#include "logger.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <chrono>
#include <windows.h>

namespace HogwartsMP {
namespace Logging {

bool Logger::_initialized = false;
LogLevel Logger::_level = LogLevel::Info;
fs::path Logger::_logFile;

static std::ofstream logFileStream;
static std::mutex logMutex;
static HANDLE g_hConsole = INVALID_HANDLE_VALUE;

void Logger::Initialize(const std::string& logDir, LogLevel level) {
    if (_initialized) {
        return;
    }

    _level = level;

    // Create logs directory
    if (!fs::exists(logDir)) {
        fs::create_directories(logDir);
    }

    // Create log file with timestamp
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &in_time_t);

    std::ostringstream filename;
    filename << logDir << "/hogwartsmp_"
             << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".log";

    _logFile = filename.str();

    // Open log file
    logFileStream.open(_logFile, std::ios::out | std::ios::app);

    if (!logFileStream.is_open()) {
        std::cerr << "[Logger] Failed to open log file: " << _logFile << std::endl;
    }

    // Initialize Console Handle explicitly
    // We open "CONOUT$" to ensure we have a direct handle to the console buffer,
    // which works even if stdout is redirected to NUL by the host process.
    g_hConsole = CreateFileA("CONOUT$", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    
    if (g_hConsole == INVALID_HANDLE_VALUE) {
        // Fallback to standard handle if CreateFile fails (e.g. no console attached yet)
        g_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    _initialized = true;

    LogSystemInfo();
    Info("=== HogwartsMP Logger Initialized ===");
}

void Logger::Shutdown() {
    if (!_initialized) {
        return;
    }

    Info("=== HogwartsMP Logger Shutdown ===");

    if (logFileStream.is_open()) {
        logFileStream.close();
    }

    if (g_hConsole != INVALID_HANDLE_VALUE && g_hConsole != GetStdHandle(STD_OUTPUT_HANDLE)) {
        CloseHandle(g_hConsole);
    }
    g_hConsole = INVALID_HANDLE_VALUE;

    _initialized = false;
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

void Logger::RotateIfNeeded() {
    if (logFileStream.tellp() > MAX_LOG_SIZE) {
        logFileStream.close();

        // Rename current file to .old
        fs::path oldPath = _logFile;
        oldPath += ".old";
        
        // Remove existing .old if it exists
        if (fs::exists(oldPath)) {
            fs::remove(oldPath);
        }
        
        fs::rename(_logFile, oldPath);

        // Open new file
        logFileStream.open(_logFile, std::ios::out | std::ios::app);
        if (logFileStream.is_open()) {
            logFileStream << "=== Log Rotated ===" << std::endl;
        }
    }
}

void Logger::LogSystemInfo() {
    if (!logFileStream.is_open()) return;

    std::lock_guard<std::mutex> lock(logMutex);

    logFileStream << "=== System Information ===" << std::endl;
    logFileStream << "Started at: " << GetTimestamp() << std::endl;

    // Memory Info
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    
    logFileStream << "Total Physical Memory: " << memInfo.ullTotalPhys / (1024 * 1024) << " MB" << std::endl;
    logFileStream << "Available Physical Memory: " << memInfo.ullAvailPhys / (1024 * 1024) << " MB" << std::endl;

    // CPU Info
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    logFileStream << "Processor Architecture: " << sysInfo.wProcessorArchitecture << std::endl;
    logFileStream << "Number of Processors: " << sysInfo.dwNumberOfProcessors << std::endl;
    
    logFileStream << "==========================" << std::endl;
}

const char* LevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Trace:    return "TRACE";
        case LogLevel::Debug:    return "DEBUG";
        case LogLevel::Info:     return "INFO";
        case LogLevel::Warning:  return "WARN";
        case LogLevel::Error:    return "ERROR";
        case LogLevel::Critical: return "CRIT";
        default:                 return "UNKNOWN";
    }
}

void Logger::Log(LogLevel level, const std::string& message, const char* file, int line) {
    if (!_initialized || level < _level) {
        return;
    }

    std::lock_guard<std::mutex> lock(logMutex);

    std::string timestamp = GetTimestamp();
    const char* levelStr = LevelToString(level);

    // Format: [YYYY-MM-DD HH:MM:SS.mmm] [LEVEL] [file:line] Message
    std::ostringstream formatted;
    formatted << "[" << timestamp << "] [" << levelStr << "] ";
    
    if (file) {
        formatted << "[" << fs::path(file).filename().string() << ":" << line << "] ";
    }
    
    formatted << message;

    std::string logLine = formatted.str();

    // Output to console using the dedicated handle
    if (g_hConsole != INVALID_HANDLE_VALUE) {
        std::string finalLog = logLine + "\n";
        WriteConsoleA(g_hConsole, finalLog.c_str(), static_cast<DWORD>(finalLog.length()), nullptr, nullptr);
    }

    // Output to file
    if (logFileStream.is_open()) {
        RotateIfNeeded();
        logFileStream << logLine << std::endl;
        logFileStream.flush(); // Ensure immediate write
    }
}

void Logger::Trace(const std::string& message, const char* file, int line) {
    Log(LogLevel::Trace, message, file, line);
}

void Logger::Debug(const std::string& message, const char* file, int line) {
    Log(LogLevel::Debug, message, file, line);
}

void Logger::Info(const std::string& message, const char* file, int line) {
    Log(LogLevel::Info, message, file, line);
}

void Logger::Warning(const std::string& message, const char* file, int line) {
    Log(LogLevel::Warning, message, file, line);
}

void Logger::Error(const std::string& message, const char* file, int line) {
    Log(LogLevel::Error, message, file, line);
}

void Logger::Critical(const std::string& message, const char* file, int line) {
    Log(LogLevel::Critical, message, file, line);
}

} // namespace Logging
} // namespace HogwartsMP
