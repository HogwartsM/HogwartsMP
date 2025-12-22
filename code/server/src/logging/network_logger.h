#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <filesystem>
#include <functional>
#include <ctime>
#include <vector>

namespace HogwartsMP::Logging {

enum class NetworkEventType {
    CONNECT,
    DISCONNECT,
    REQUEST,
    RESPONSE,
    ERROR_EVENT
};

enum class NetworkSeverity {
    INFO_LEVEL,
    WARNING_LEVEL,
    ERROR_LEVEL
};

struct NetworkEvent {
    std::time_t timestamp;
    NetworkEventType type;
    std::string sourceIp;
    std::string destIp;
    std::string method;   // HTTP method or Packet ID
    std::string endpoint; // URL or RPC name
    int statusCode;       // HTTP status or Error code
    double processingTimeMs;
    size_t dataSize;
    NetworkSeverity severity;
};

class NetworkLogger {
public:
    static NetworkLogger& Get();

    void Initialize(const std::string& logDir);
    void Shutdown();

    void Log(const NetworkEvent& event);
    
    // Helper methods for common events
    void LogConnection(const std::string& ip, const std::string& endpoint = "/connect");
    void LogDisconnection(const std::string& ip, const std::string& reason = "unknown");
    void LogRequest(const std::string& sourceIp, const std::string& method, const std::string& endpoint, size_t size);
    void LogResponse(const std::string& destIp, const std::string& method, const std::string& endpoint, int status, double duration, size_t size);

    // Filtering
    using FilterCallback = std::function<bool(const NetworkEvent&)>;
    void SetFilter(FilterCallback filter) { _filter = filter; }

private:
    NetworkLogger() = default;
    ~NetworkLogger();

    void RotateLogFile();
    void ArchiveOldLogs();
    std::string EventTypeToString(NetworkEventType type);
    std::string SeverityToString(NetworkSeverity severity);
    std::string FormatJSON(const NetworkEvent& event);
    std::string GetCurrentDateString();

    std::string _logDir;
    std::string _currentDate;
    std::ofstream _logFile;
    std::mutex _mutex;
    bool _initialized = false;
    FilterCallback _filter;
    
    const int RETENTION_DAYS = 30;
};

} // namespace HogwartsMP::Logging
