#pragma once

#include <string>
#include <memory>
#include <filesystem>

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
    static void Trace(const std::string& message);
    static void Debug(const std::string& message);
    static void Info(const std::string& message);
    static void Warning(const std::string& message);
    static void Error(const std::string& message);
    static void Critical(const std::string& message);

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
    static void Log(LogLevel level, const std::string& message);

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
    static std::filesystem::path _logFile;
};

// Convenience macros
#define LOG_TRACE(msg)    HogwartsMP::Logging::Logger::Trace(msg)
#define LOG_DEBUG(msg)    HogwartsMP::Logging::Logger::Debug(msg)
#define LOG_INFO(msg)     HogwartsMP::Logging::Logger::Info(msg)
#define LOG_WARNING(msg)  HogwartsMP::Logging::Logger::Warning(msg)
#define LOG_ERROR(msg)    HogwartsMP::Logging::Logger::Error(msg)
#define LOG_CRITICAL(msg) HogwartsMP::Logging::Logger::Critical(msg)

#define LOG_TRACE_F(fmt, ...)    HogwartsMP::Logging::Logger::TraceF(fmt, __VA_ARGS__)
#define LOG_DEBUG_F(fmt, ...)    HogwartsMP::Logging::Logger::DebugF(fmt, __VA_ARGS__)
#define LOG_INFO_F(fmt, ...)     HogwartsMP::Logging::Logger::InfoF(fmt, __VA_ARGS__)
#define LOG_WARNING_F(fmt, ...)  HogwartsMP::Logging::Logger::WarningF(fmt, __VA_ARGS__)
#define LOG_ERROR_F(fmt, ...)    HogwartsMP::Logging::Logger::ErrorF(fmt, __VA_ARGS__)
#define LOG_CRITICAL_F(fmt, ...) HogwartsMP::Logging::Logger::CriticalF(fmt, __VA_ARGS__)

} // namespace HogwartsMP::Logging
