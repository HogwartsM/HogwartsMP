#include "manual_map.h"
#include <iostream>
#include <fstream>
#include <vector>

// Stub function that runs in target process
DWORD WINAPI LoaderStub(LPVOID pData) {
    ManualMapData* data = (ManualMapData*)pData;

    BYTE* pBase = (BYTE*)data->imageBase;
    auto* pDosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(pBase);
    auto* pNTHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(pBase + pDosHeader->e_lfanew);
    auto* pOpt = &pNTHeaders->OptionalHeader;

    // Resolve imports
    auto* pImportDescr = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(pBase + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
    while (pImportDescr->Name) {
        char* szMod = reinterpret_cast<char*>(pBase + pImportDescr->Name);
        HMODULE hDll = data->fnLoadLibraryA(szMod);

        auto* pThunkRef = reinterpret_cast<ULONG_PTR*>(pBase + pImportDescr->OriginalFirstThunk);
        auto* pFuncRef = reinterpret_cast<ULONG_PTR*>(pBase + pImportDescr->FirstThunk);

        if (!pThunkRef)
            pThunkRef = pFuncRef;

        for (; *pThunkRef; ++pThunkRef, ++pFuncRef) {
            if (IMAGE_SNAP_BY_ORDINAL(*pThunkRef)) {
                *pFuncRef = (ULONG_PTR)data->fnGetProcAddress(hDll, reinterpret_cast<char*>(*pThunkRef & 0xFFFF));
            } else {
                auto* pImport = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(pBase + (*pThunkRef));
                *pFuncRef = (ULONG_PTR)data->fnGetProcAddress(hDll, pImport->Name);
            }
        }
        ++pImportDescr;
    }

    // Execute TLS callbacks
    if (pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size) {
        auto* pTLS = reinterpret_cast<IMAGE_TLS_DIRECTORY*>(pBase + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress);
        auto* pCallback = reinterpret_cast<PIMAGE_TLS_CALLBACK*>(pTLS->AddressOfCallBacks);
        for (; pCallback && *pCallback; ++pCallback)
            (*pCallback)(pBase, DLL_PROCESS_ATTACH, nullptr);
    }

    // Call DllMain (EntryPoint)
    auto DllMain = reinterpret_cast<BOOL(WINAPI*)(HMODULE, DWORD, LPVOID)>(pBase + pOpt->AddressOfEntryPoint);
    DllMain((HMODULE)pBase, DLL_PROCESS_ATTACH, nullptr);

    return 0;
}

// Dummy function to mark end of LoaderStub
DWORD WINAPI LoaderStubEnd() { return 0; }

bool ManualMapInject(HANDLE hProcess, const std::wstring& dllPath) {
    std::wcout << L"Starting Manual Map injection..." << std::endl;

    // Read DLL file
    std::ifstream File(dllPath.c_str(), std::ios::binary | std::ios::ate);
    if (!File.is_open()) {
        std::wcerr << L"Failed to open DLL file: " << dllPath << std::endl;
        return false;
    }

    auto FileSize = File.tellg();
    std::vector<BYTE> srcData(FileSize);
    File.seekg(0, std::ios::beg);
    File.read(reinterpret_cast<char*>(srcData.data()), FileSize);
    File.close();

    // Validate PE headers
    auto* pDosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(srcData.data());
    if (pDosHeader->e_magic != 0x5A4D) { // "MZ"
        std::wcerr << L"Invalid PE file (bad DOS header)" << std::endl;
        return false;
    }

    auto* pNTHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(srcData.data() + pDosHeader->e_lfanew);
    if (pNTHeaders->Signature != IMAGE_NT_SIGNATURE) {
        std::wcerr << L"Invalid PE file (bad NT signature)" << std::endl;
        return false;
    }

    auto& OptionalHeader = pNTHeaders->OptionalHeader;
    auto* pSectionHeader = IMAGE_FIRST_SECTION(pNTHeaders);

    std::wcout << L"DLL image size: 0x" << std::hex << OptionalHeader.SizeOfImage << std::dec << std::endl;

    // Allocate memory in target process
    void* pTargetBase = VirtualAllocEx(hProcess, nullptr, OptionalHeader.SizeOfImage,
                                       MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!pTargetBase) {
        std::wcerr << L"Failed to allocate memory in target process: " << GetLastError() << std::endl;
        return false;
    }

    std::wcout << L"Allocated memory at: 0x" << std::hex << pTargetBase << std::dec << std::endl;

    // Map headers
    if (!WriteProcessMemory(hProcess, pTargetBase, srcData.data(), OptionalHeader.SizeOfHeaders, nullptr)) {
        std::wcerr << L"Failed to write headers: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pTargetBase, 0, MEM_RELEASE);
        return false;
    }

    // Map sections
    for (UINT i = 0; i < pNTHeaders->FileHeader.NumberOfSections; ++i, ++pSectionHeader) {
        if (pSectionHeader->SizeOfRawData == 0)
            continue;

        void* pSectionDest = (BYTE*)pTargetBase + pSectionHeader->VirtualAddress;
        if (!WriteProcessMemory(hProcess, pSectionDest,
                               srcData.data() + pSectionHeader->PointerToRawData,
                               pSectionHeader->SizeOfRawData, nullptr)) {
            std::wcerr << L"Failed to write section " << i << L": " << GetLastError() << std::endl;
            VirtualFreeEx(hProcess, pTargetBase, 0, MEM_RELEASE);
            return false;
        }
    }

    std::wcout << L"Sections mapped successfully" << std::endl;

    // Prepare loader data
    ManualMapData data;
    data.imageBase = pTargetBase;

    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    data.fnLoadLibraryA = (decltype(data.fnLoadLibraryA))GetProcAddress(hKernel32, "LoadLibraryA");
    data.fnGetProcAddress = (decltype(data.fnGetProcAddress))GetProcAddress(hKernel32, "GetProcAddress");

    // Allocate memory for loader data
    void* pLoaderData = VirtualAllocEx(hProcess, nullptr, sizeof(ManualMapData),
                                       MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!pLoaderData) {
        std::wcerr << L"Failed to allocate loader data: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pTargetBase, 0, MEM_RELEASE);
        return false;
    }

    if (!WriteProcessMemory(hProcess, pLoaderData, &data, sizeof(ManualMapData), nullptr)) {
        std::wcerr << L"Failed to write loader data: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pTargetBase, 0, MEM_RELEASE);
        VirtualFreeEx(hProcess, pLoaderData, 0, MEM_RELEASE);
        return false;
    }

    // Copy loader stub to target
    SIZE_T stubSize = reinterpret_cast<SIZE_T>(&LoaderStubEnd) - reinterpret_cast<SIZE_T>(&LoaderStub);
    void* pLoaderStub = VirtualAllocEx(hProcess, nullptr, stubSize,
                                       MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!pLoaderStub) {
        std::wcerr << L"Failed to allocate loader stub: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pTargetBase, 0, MEM_RELEASE);
        VirtualFreeEx(hProcess, pLoaderData, 0, MEM_RELEASE);
        return false;
    }

    if (!WriteProcessMemory(hProcess, pLoaderStub, (void*)&LoaderStub, stubSize, nullptr)) {
        std::wcerr << L"Failed to write loader stub: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pTargetBase, 0, MEM_RELEASE);
        VirtualFreeEx(hProcess, pLoaderData, 0, MEM_RELEASE);
        VirtualFreeEx(hProcess, pLoaderStub, 0, MEM_RELEASE);
        return false;
    }

    std::wcout << L"Executing loader stub..." << std::endl;

    // Execute loader stub
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
                                       (LPTHREAD_START_ROUTINE)pLoaderStub,
                                       pLoaderData, 0, nullptr);
    if (!hThread) {
        std::wcerr << L"Failed to create remote thread: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pTargetBase, 0, MEM_RELEASE);
        VirtualFreeEx(hProcess, pLoaderData, 0, MEM_RELEASE);
        VirtualFreeEx(hProcess, pLoaderStub, 0, MEM_RELEASE);
        return false;
    }

    // Wait for loader to complete
    DWORD waitResult = WaitForSingleObject(hThread, 10000);
    if (waitResult == WAIT_TIMEOUT) {
        std::wcerr << L"Loader stub timeout" << std::endl;
        CloseHandle(hThread);
        return false;
    }

    DWORD exitCode;
    GetExitCodeThread(hThread, &exitCode);
    CloseHandle(hThread);

    // Cleanup
    VirtualFreeEx(hProcess, pLoaderData, 0, MEM_RELEASE);
    VirtualFreeEx(hProcess, pLoaderStub, 0, MEM_RELEASE);

    if (exitCode != 0) {
        std::wcerr << L"Loader stub failed with exit code: " << exitCode << std::endl;
        VirtualFreeEx(hProcess, pTargetBase, 0, MEM_RELEASE);
        return false;
    }

    std::wcout << L"Manual Map injection successful!" << std::endl;
    std::wcout << L"DLL base address: 0x" << std::hex << pTargetBase << std::dec << std::endl;

    return true;
}
