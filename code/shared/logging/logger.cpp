#include "logger.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <mutex>

namespace HogwartsMP::Logging {

bool Logger::_initialized = false;
LogLevel Logger::_level = LogLevel::Info;
std::filesystem::path Logger::_logFile;

static std::ofstream logFileStream;
static std::mutex logMutex;

void Logger::Initialize(const std::string& logDir, LogLevel level) {
    if (_initialized) {
        return;
    }

    _level = level;

    // Create logs directory
    std::filesystem::create_directories(logDir);

    // Create log file with timestamp
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);

    std::ostringstream filename;
    filename << logDir << "/hogwartsmp_"
             << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".log";

    _logFile = filename.str();

    // Open log file
    logFileStream.open(_logFile, std::ios::out | std::ios::app);

    if (!logFileStream.is_open()) {
        std::cerr << "[Logger] Failed to open log file: " << _logFile << std::endl;
    }

    _initialized = true;

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

    _initialized = false;
}

std::string GetTimestamp() {
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
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

void Logger::Log(LogLevel level, const std::string& message) {
    if (!_initialized || level < _level) {
        return;
    }

    std::lock_guard<std::mutex> lock(logMutex);

    std::string timestamp = GetTimestamp();
    const char* levelStr = LevelToString(level);

    // Format: [YYYY-MM-DD HH:MM:SS] [LEVEL] Message
    std::ostringstream formatted;
    formatted << "[" << timestamp << "] [" << levelStr << "] " << message;

    std::string logLine = formatted.str();

    // Output to console
    if (level >= LogLevel::Error) {
        std::cerr << logLine << std::endl;
    } else {
        std::cout << logLine << std::endl;
    }

    // Output to file
    if (logFileStream.is_open()) {
        logFileStream << logLine << std::endl;
        logFileStream.flush(); // Ensure immediate write
    }
}

void Logger::Trace(const std::string& message) {
    Log(LogLevel::Trace, message);
}

void Logger::Debug(const std::string& message) {
    Log(LogLevel::Debug, message);
}

void Logger::Info(const std::string& message) {
    Log(LogLevel::Info, message);
}

void Logger::Warning(const std::string& message) {
    Log(LogLevel::Warning, message);
}

void Logger::Error(const std::string& message) {
    Log(LogLevel::Error, message);
}

void Logger::Critical(const std::string& message) {
    Log(LogLevel::Critical, message);
}

void Logger::SetLevel(LogLevel level) {
    _level = level;
}

LogLevel Logger::GetLevel() {
    return _level;
}

} // namespace HogwartsMP::Logging
