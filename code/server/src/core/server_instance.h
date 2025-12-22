#pragma once

#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <flecs.h>

namespace HogwartsMP::Networking {
    class NetworkServer;
}

namespace HogwartsMP::Core {

struct ServerOptions {
    std::string serverName = "HogwartsMP Server";
    std::string gameVersion = "2.0.0";
    uint16_t port = 27015;
    uint16_t webPort = 27016;
    size_t maxPlayers = 512;
    std::string resourcesPath = "resources";
    bool enableConsole = true;
};

class ServerInstance {
protected:
    ServerOptions _options;
    bool _initialized = false;

    // Core subsystems
    std::unique_ptr<flecs::world> _world;
    std::unique_ptr<Networking::NetworkServer> _networkServer;

public:
    ServerInstance(); // Defined in .cpp to allow forward declaration
    virtual ~ServerInstance(); // Defined in .cpp to allow forward declaration

    // Main lifecycle
    virtual bool Init(ServerOptions opts);
    virtual void Update();
    virtual void Shutdown();

    // Hooks for derived classes
    virtual bool PostInit() = 0;
    virtual void PostUpdate() = 0;
    virtual bool PreShutdown() = 0;

    // Accessors
    flecs::world* GetWorld() const { return _world.get(); }
    Networking::NetworkServer* GetNetworkServer() const { return _networkServer.get(); }
    const ServerOptions& GetOptions() const { return _options; }
    bool IsInitialized() const { return _initialized; }

    uint16_t GetPort() const { return _options.port; }
    uint16_t GetWebPort() const { return _options.webPort; }
    size_t GetMaxPlayers() const { return _options.maxPlayers; }
};

} // namespace HogwartsMP::Core
