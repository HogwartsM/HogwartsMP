#pragma once

#ifndef _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#endif

#include <string>
#include <memory>

#if __has_include(<filesystem>)
    #include <filesystem>
    #if defined(__cpp_lib_filesystem) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
        namespace fs = std::filesystem;
    #else
        #include <experimental/filesystem>
        namespace fs = std::experimental::filesystem;
    #endif
#elif __has_include(<experimental/filesystem>)
    #include <experimental/filesystem>
    namespace fs = std::experimental::filesystem;
#else
    #error "Missing filesystem support"
#endif

namespace HogwartsMP::Logging {

enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Critical
};

class Logger {
public:
    static void Initialize(const std::string& logDir = "logs", LogLevel level = LogLevel::Info);
    static void Shutdown();

    // Logging methods
    static void Trace(const std::string& message, const char* file = nullptr, int line = 0);
    static void Debug(const std::string& message, const char* file = nullptr, int line = 0);
    static void Info(const std::string& message, const char* file = nullptr, int line = 0);
    static void Warning(const std::string& message, const char* file = nullptr, int line = 0);
    static void Error(const std::string& message, const char* file = nullptr, int line = 0);
    static void Critical(const std::string& message, const char* file = nullptr, int line = 0);

    // Formatted logging (printf-style)
    template<typename... Args>
    static void TraceF(const char* format, Args&&... args) {
        Log(LogLevel::Trace, Format(format, std::forward<Args>(args)...));
    }

    template<typename... Args>
    static void DebugF(const char* format, Args&&... args) {
        Log(LogLevel::Debug, Format(format, std::forward<Args>(args)...));
    }

    template<typename... Args>
    static void InfoF(const char* format, Args&&... args) {
        Log(LogLevel::Info, Format(format, std::forward<Args>(args)...));
    }

    template<typename... Args>
    static void WarningF(const char* format, Args&&... args) {
        Log(LogLevel::Warning, Format(format, std::forward<Args>(args)...));
    }

    template<typename... Args>
    static void ErrorF(const char* format, Args&&... args) {
        Log(LogLevel::Error, Format(format, std::forward<Args>(args)...));
    }

    template<typename... Args>
    static void CriticalF(const char* format, Args&&... args) {
        Log(LogLevel::Critical, Format(format, std::forward<Args>(args)...));
    }

    static void SetLevel(LogLevel level);
    static LogLevel GetLevel();

private:
    static void Log(LogLevel level, const std::string& message, const char* file = nullptr, int line = 0);
    static void RotateIfNeeded();
    static void LogSystemInfo();
    static std::string GetTimestamp();
    static const size_t MAX_LOG_SIZE = 5 * 1024 * 1024; // 5 MB

    template<typename... Args>
    static std::string Format(const char* format, Args&&... args) {
        int size = std::snprintf(nullptr, 0, format, std::forward<Args>(args)...) + 1;
        if (size <= 0) return "";

        std::unique_ptr<char[]> buf(new char[size]);
        std::snprintf(buf.get(), size, format, std::forward<Args>(args)...);
        return std::string(buf.get(), buf.get() + size - 1);
    }

    static bool _initialized;
    static LogLevel _level;
    static fs::path _logFile;
};

// Convenience macros
#define LOG_TRACE(msg)    HogwartsMP::Logging::Logger::Trace(msg, __FILE__, __LINE__)
#define LOG_DEBUG(msg)    HogwartsMP::Logging::Logger::Debug(msg, __FILE__, __LINE__)
#define LOG_INFO(msg)     HogwartsMP::Logging::Logger::Info(msg, __FILE__, __LINE__)
#define LOG_WARNING(msg)  HogwartsMP::Logging::Logger::Warning(msg, __FILE__, __LINE__)
#define LOG_ERROR(msg)    HogwartsMP::Logging::Logger::Error(msg, __FILE__, __LINE__)
#define LOG_CRITICAL(msg) HogwartsMP::Logging::Logger::Critical(msg, __FILE__, __LINE__)

#define LOG_TRACE_F(fmt, ...)    HogwartsMP::Logging::Logger::TraceF(fmt, __VA_ARGS__)
#define LOG_DEBUG_F(fmt, ...)    HogwartsMP::Logging::Logger::DebugF(fmt, __VA_ARGS__)
#define LOG_INFO_F(fmt, ...)     HogwartsMP::Logging::Logger::InfoF(fmt, __VA_ARGS__)
#define LOG_WARNING_F(fmt, ...)  HogwartsMP::Logging::Logger::WarningF(fmt, __VA_ARGS__)
#define LOG_ERROR_F(fmt, ...)    HogwartsMP::Logging::Logger::ErrorF(fmt, __VA_ARGS__)
#define LOG_CRITICAL_F(fmt, ...) HogwartsMP::Logging::Logger::CriticalF(fmt, __VA_ARGS__)

} // namespace HogwartsMP::Logging
