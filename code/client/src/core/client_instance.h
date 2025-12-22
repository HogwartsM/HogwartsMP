#pragma once

#include <memory>
#include <string>
#include <cstdint>

// Forward declarations
namespace HogwartsMP::Networking {
    class NetworkClient;
}

namespace HogwartsMP::Core {

struct ClientOptions {
    std::string gameName = "HogwartsMP";
    std::string gameVersion = "2.0.0";
    std::string serverHost = "127.0.0.1";
    uint16_t serverPort = 27015;
};

class ClientInstance {
protected:
    ClientOptions _options;
    bool _initialized = false;

    // Core subsystems
    std::unique_ptr<Networking::NetworkClient> _networkClient;

public:
    ClientInstance();
    virtual ~ClientInstance();

    // Main lifecycle
    virtual bool Init(ClientOptions opts);
    virtual void Update();
    virtual void Shutdown();

    // Accessors
    Networking::NetworkClient* GetNetworkClient() const { return _networkClient.get(); }
    const ClientOptions& GetOptions() const { return _options; }
    bool IsInitialized() const { return _initialized; }
};

} // namespace HogwartsMP::Core
