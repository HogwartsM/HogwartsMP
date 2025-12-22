#include "Logger.h"
#include <windows.h>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>

std::ofstream Logger::m_logFile;
std::mutex Logger::m_mutex;
std::filesystem::path Logger::m_logFilePath;

void Logger::Init() {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Create logs directory if it doesn't exist
    std::filesystem::path logDir = "logs";
    if (!std::filesystem::exists(logDir)) {
        std::filesystem::create_directory(logDir);
    }

    // Generate filename with timestamp
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm buf;
    localtime_s(&buf, &in_time_t);

    std::ostringstream filename;
    filename << "launcher_logs_" 
             << std::put_time(&buf, "%Y-%m-%d_%H-%M-%S") 
             << ".txt";

    m_logFilePath = logDir / filename.str();

    m_logFile.open(m_logFilePath, std::ios::out | std::ios::app);
    
    if (m_logFile.is_open()) {
        LogSystemInfo();
    } else {
        std::cerr << "Failed to open log file: " << m_logFilePath << std::endl;
    }
    
    // Also print system info to console
    std::cout << "=== System Information ===" << std::endl;
    std::cout << "Logs will be written to: " << m_logFilePath.string() << std::endl;
}

void Logger::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_logFile.is_open()) {
        m_logFile << "=== Logger Shutdown ===" << std::endl;
        m_logFile.close();
    }
}

void Logger::Log(LogLevel level, const std::string& message, const char* file, int line) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Construct the log entry
    std::ostringstream logEntry;
    logEntry << "[" << GetTimestamp() << "] "
             << "[" << LevelToString(level) << "] "
             << "[" << std::filesystem::path(file).filename().string() << ":" << line << "] "
             << message;
    
    std::string logStr = logEntry.str();

    // Write to file
    if (m_logFile.is_open()) {
        RotateIfNeeded();
        m_logFile << logStr << std::endl;
        m_logFile.flush();
    }

    // Write to console
    if (level == LogLevel::ERR) {
        std::cerr << logStr << std::endl;
    } else {
        std::cout << logStr << std::endl;
    }
}

void Logger::Log(LogLevel level, const std::wstring& message, const char* file, int line) {
    Log(level, WStringToString(message), file, line);
}

void Logger::RotateIfNeeded() {
    if (m_logFile.tellp() > MAX_LOG_SIZE) {
        m_logFile.close();

        // Rename current file to .old
        std::filesystem::path oldPath = m_logFilePath;
        oldPath += ".old";
        
        // Remove existing .old if it exists
        if (std::filesystem::exists(oldPath)) {
            std::filesystem::remove(oldPath);
        }
        
        std::filesystem::rename(m_logFilePath, oldPath);

        // Open new file
        m_logFile.open(m_logFilePath, std::ios::out | std::ios::app);
        if (m_logFile.is_open()) {
            m_logFile << "=== Log Rotated ===" << std::endl;
        }
    }
}

std::string Logger::GetTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm buf;
    localtime_s(&buf, &in_time_t);

    std::ostringstream ss;
    ss << std::put_time(&buf, "%Y-%m-%d %H:%M:%S") << "." << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string Logger::LevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void Logger::LogSystemInfo() {
    if (!m_logFile.is_open()) return;

    m_logFile << "=== System Information ===" << std::endl;
    m_logFile << "Launcher Started at: " << GetTimestamp() << std::endl;

    // Get OS Version
    OSVERSIONINFOEXA osInfo;
    ZeroMemory(&osInfo, sizeof(OSVERSIONINFOEXA));
    osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    // Note: GetVersionEx is deprecated but useful for simple logging, 
    // for exact version in Win10/11 usually need RtlGetVersion or manifest.
    // Using simple approach for now.
    
    // Memory Info
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    
    m_logFile << "Total Physical Memory: " << memInfo.ullTotalPhys / (1024 * 1024) << " MB" << std::endl;
    m_logFile << "Available Physical Memory: " << memInfo.ullAvailPhys / (1024 * 1024) << " MB" << std::endl;

    // CPU Info
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    m_logFile << "Processor Architecture: " << sysInfo.wProcessorArchitecture << std::endl;
    m_logFile << "Number of Processors: " << sysInfo.dwNumberOfProcessors << std::endl;
    
    m_logFile << "==========================" << std::endl;
}

std::string Logger::WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}
