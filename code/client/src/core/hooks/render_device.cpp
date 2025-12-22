#include <utils/safe_win32.h>
#include <MinHook.h>
#include <logging/logger.h>
#include <utils/hooking/hook_function.h>
#include <utils/hooking/hooking.h>

#include "../application.h"
#include "dx12_pointer_grab.cpp"
#include "../../Overlay/overlay_manager.h"

#include <imgui.h>
#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_win32.h>
#include <d3d12.h>
#include <dxgi1_4.h>

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace HogwartsMP::Core::Hooks {

    // Global DX12 State for ImGui
    struct DX12FrameContext {
        ID3D12CommandAllocator* CommandAllocator;
        uint64_t FenceValue;
    };

    static bool g_ImGuiInitialized = false;
    static ID3D12Device* g_Device = nullptr;
    static ID3D12DescriptorHeap* g_SrvDescHeap = nullptr;
    static ID3D12DescriptorHeap* g_RtvHeap = nullptr;
    static ID3D12CommandQueue* g_CommandQueue = nullptr;
    static ID3D12GraphicsCommandList* g_CommandList = nullptr;
    static DX12FrameContext* g_FrameContext = nullptr;
    static UINT g_FrameBufferCount = 0;
    static ID3D12Resource** g_MainRenderTargetResource = nullptr;
    static D3D12_CPU_DESCRIPTOR_HANDLE* g_MainRenderTargetDescriptor = nullptr;

    // Hook Function Pointers
    typedef long(__fastcall* IDXGISwapChain3__Present_t) (IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT Flags);
    static IDXGISwapChain3__Present_t IDXGISwapChain3__Present_original = nullptr;

    typedef long(__fastcall* IDXGISwapChain3__ResizeBuffers_t)(IDXGISwapChain3* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
    static IDXGISwapChain3__ResizeBuffers_t IDXGISwapChain3__ResizeBuffers_original = nullptr;

    typedef void(*ID3D12CommandQueue__ExecuteCommandLists_t)(ID3D12CommandQueue* queue, UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists);
    static ID3D12CommandQueue__ExecuteCommandLists_t ID3D12CommandQueue__ExecuteCommandLists_original = nullptr;

    typedef void(__fastcall *FWindowsApplication__ProcessMessage_t)(void *, HWND hwnd, uint32_t msg, WPARAM wParam, LPARAM lParam);
    static FWindowsApplication__ProcessMessage_t FWindowsApplication__ProcessMessage_original = nullptr;

    // Helper functions
    void CleanupDeviceD3D12() {
        if (g_FrameContext) {
            for (UINT i = 0; i < g_FrameBufferCount; i++) {
                if (g_FrameContext[i].CommandAllocator) g_FrameContext[i].CommandAllocator->Release();
            }
            delete[] g_FrameContext;
            g_FrameContext = nullptr;
        }
        if (g_CommandList) { g_CommandList->Release(); g_CommandList = nullptr; }
        if (g_SrvDescHeap) { g_SrvDescHeap->Release(); g_SrvDescHeap = nullptr; }
        if (g_RtvHeap) { g_RtvHeap->Release(); g_RtvHeap = nullptr; }
        if (g_MainRenderTargetResource) {
            for (UINT i = 0; i < g_FrameBufferCount; i++) {
                if (g_MainRenderTargetResource[i]) g_MainRenderTargetResource[i]->Release();
            }
            delete[] g_MainRenderTargetResource;
            g_MainRenderTargetResource = nullptr;
        }
        if (g_MainRenderTargetDescriptor) {
            delete[] g_MainRenderTargetDescriptor;
            g_MainRenderTargetDescriptor = nullptr;
        }
    }

    void SetupRenderTarget(IDXGISwapChain3* pSwapChain) {
        for (UINT i = 0; i < g_FrameBufferCount; i++) {
            ID3D12Resource* pBackBuffer = nullptr;
            pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
            g_Device->CreateRenderTargetView(pBackBuffer, nullptr, g_MainRenderTargetDescriptor[i]);
            g_MainRenderTargetResource[i] = pBackBuffer;
        }
    }

    // Hook Implementations

    void ID3D12CommandQueue__ExecuteCommandLists_Hook(ID3D12CommandQueue* queue, UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists) {
        if (!g_CommandQueue) {
            g_CommandQueue = queue;
        }
        ID3D12CommandQueue__ExecuteCommandLists_original(queue, NumCommandLists, ppCommandLists);
    }

    long __fastcall IDXGISwapChain3__Present_Hook(IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT Flags) {
        if (!g_ImGuiInitialized && g_CommandQueue) {
            if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D12Device), (void**)&g_Device))) {
                DXGI_SWAP_CHAIN_DESC desc;
                pSwapChain->GetDesc(&desc);
                
                g_FrameBufferCount = desc.BufferCount;
                g_FrameContext = new DX12FrameContext[g_FrameBufferCount];
                g_MainRenderTargetResource = new ID3D12Resource*[g_FrameBufferCount];
                g_MainRenderTargetDescriptor = new D3D12_CPU_DESCRIPTOR_HANDLE[g_FrameBufferCount];

                // Create SRV Descriptor Heap
                D3D12_DESCRIPTOR_HEAP_DESC srvDesc = {};
                srvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
                srvDesc.NumDescriptors = 1;
                srvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
                if (FAILED(g_Device->CreateDescriptorHeap(&srvDesc, IID_PPV_ARGS(&g_SrvDescHeap)))) {
                    return IDXGISwapChain3__Present_original(pSwapChain, SyncInterval, Flags);
                }

                for (UINT i = 0; i < g_FrameBufferCount; i++) {
                    g_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_FrameContext[i].CommandAllocator));
                }
                
                g_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_FrameContext[0].CommandAllocator, nullptr, IID_PPV_ARGS(&g_CommandList));
                g_CommandList->Close(); // Start closed

                // Setup RTVs
                // We need a persistent RTV heap.
                D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {};
                rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
                rtvDesc.NumDescriptors = g_FrameBufferCount;
                rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
                if (FAILED(g_Device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&g_RtvHeap)))) {
                     return IDXGISwapChain3__Present_original(pSwapChain, SyncInterval, Flags);
                }
                
                SIZE_T rtvDescriptorSize = g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
                D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_RtvHeap->GetCPUDescriptorHandleForHeapStart();
                for (UINT i = 0; i < g_FrameBufferCount; i++) {
                    g_MainRenderTargetDescriptor[i] = rtvHandle;
                    rtvHandle.ptr += rtvDescriptorSize;
                }
                
                SetupRenderTarget(pSwapChain);
                
                // Init ImGui
                ImGui::CreateContext();
                ImGuiIO& io = ImGui::GetIO();
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
                
                ImGui_ImplWin32_Init(desc.OutputWindow);
                ImGui_ImplDX12_Init(g_Device, g_FrameBufferCount,
                    desc.BufferDesc.Format, g_SrvDescHeap,
                    g_SrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
                    g_SrvDescHeap->GetGPUDescriptorHandleForHeapStart());

                // Initialize Overlay Manager (style etc) now that ImGui context is ready
                HogwartsMP::Overlay::OverlayManager::Get().Init();

                g_ImGuiInitialized = true;
            }
        }

        if (g_ImGuiInitialized) {
            // New Frame
            ImGui_ImplDX12_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            // Application Overlay Render
            auto& app = HogwartsMP::Core::gApplication;
            if (app) {
                app->RenderOverlay();
            }

            // Render
            ImGui::Render();

            // Record command list
            UINT backBufferIdx = pSwapChain->GetCurrentBackBufferIndex();
            ID3D12CommandAllocator* allocator = g_FrameContext[backBufferIdx].CommandAllocator;
            allocator->Reset();
            
            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = g_MainRenderTargetResource[backBufferIdx];
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

            g_CommandList->Reset(allocator, nullptr);
            g_CommandList->ResourceBarrier(1, &barrier);
            
            g_CommandList->OMSetRenderTargets(1, &g_MainRenderTargetDescriptor[backBufferIdx], FALSE, nullptr);
            g_CommandList->SetDescriptorHeaps(1, &g_SrvDescHeap);
            
            ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_CommandList);
            
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
            g_CommandList->ResourceBarrier(1, &barrier);
            g_CommandList->Close();

            ID3D12CommandList* ppCommandLists[] = { g_CommandList };
            g_CommandQueue->ExecuteCommandLists(1, ppCommandLists);
        }

        return IDXGISwapChain3__Present_original(pSwapChain, SyncInterval, Flags);
    }

    long __fastcall IDXGISwapChain3__ResizeBuffers_Hook(IDXGISwapChain3* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
        if (g_ImGuiInitialized) {
            CleanupDeviceD3D12();
            ImGui_ImplDX12_InvalidateDeviceObjects();
            g_ImGuiInitialized = false;
        }
        return IDXGISwapChain3__ResizeBuffers_original(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
    }

    void FWindowsApplication__ProcessMessage_Hook(void* pThis, HWND hwnd, uint32_t msg, WPARAM wParam, LPARAM lParam) {
        if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)) {
             // If ImGui handled it and we want to block input
             auto& app = HogwartsMP::Core::gApplication;
             if (app && app->AreControlsLocked()) {
                 return; 
             }
        }
        FWindowsApplication__ProcessMessage_original(pThis, hwnd, msg, wParam, lParam);
    }

} // namespace HogwartsMP::Core::Hooks
