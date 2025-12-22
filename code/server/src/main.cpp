#include "core/server.h"
#include "logging/logger.h"
#include <thread>
#include <chrono>

int main(int argc, char **argv) {
    // Initialize logging
    HogwartsMP::Logging::Logger::Initialize("logs", HogwartsMP::Logging::LogLevel::Info);

    // Setup server options
    HogwartsMP::Core::ServerOptions opts;
    opts.serverName = "HogwartsMP Server";
    opts.port = 27015;
    opts.webPort = 27016;
    opts.maxPlayers = 512;

    // Create and initialize server
    HogwartsMP::Server server;
    if (!server.Init(opts)) {
        LOG_ERROR("Failed to initialize server");
        return 1;
    }

    LOG_INFO("HogwartsMP Server started successfully");

    // Main server loop
    while (true) {
        server.Update();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    server.Shutdown();
    return 0;
}
