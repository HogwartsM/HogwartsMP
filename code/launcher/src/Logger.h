#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <filesystem>
#include <sstream>

enum class LogLevel {
    INFO,
    WARNING,
    ERR // Avoid collision with windows.h ERROR macro
};

class Logger {
public:
    static void Init();
    static void Shutdown();
    static void Log(LogLevel level, const std::string& message, const char* file, int line);
    static void Log(LogLevel level, const std::wstring& message, const char* file, int line);

private:
    static void RotateIfNeeded();
    static std::string GetTimestamp();
    static std::string LevelToString(LogLevel level);
    static void LogSystemInfo();
    static std::string WStringToString(const std::wstring& wstr);

    static std::ofstream m_logFile;
    static std::mutex m_mutex;
    static std::filesystem::path m_logFilePath;
    static const size_t MAX_LOG_SIZE = 5 * 1024 * 1024; // 5 MB
};

// Macros for easy usage
// Usage: LOG_INFO("Message " << value);
#define LOG_INFO(msg) { std::ostringstream oss; oss << msg; Logger::Log(LogLevel::INFO, oss.str(), __FILE__, __LINE__); }
#define LOG_WARN(msg) { std::ostringstream oss; oss << msg; Logger::Log(LogLevel::WARNING, oss.str(), __FILE__, __LINE__); }
#define LOG_ERROR(msg) { std::ostringstream oss; oss << msg; Logger::Log(LogLevel::ERR, oss.str(), __FILE__, __LINE__); }

// Wide string support macros
#define LOG_INFO_W(msg) { std::wostringstream oss; oss << msg; Logger::Log(LogLevel::INFO, oss.str(), __FILE__, __LINE__); }
#define LOG_WARN_W(msg) { std::wostringstream oss; oss << msg; Logger::Log(LogLevel::WARNING, oss.str(), __FILE__, __LINE__); }
#define LOG_ERROR_W(msg) { std::wostringstream oss; oss << msg; Logger::Log(LogLevel::ERR, oss.str(), __FILE__, __LINE__); }
