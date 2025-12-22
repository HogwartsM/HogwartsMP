#pragma once

#include <memory>
#include <string>
#include <cstdint>
#include <flecs.h>

// Forward declarations
struct ID3D12Device;
struct ID3D12CommandQueue;
struct IDXGISwapChain3;
struct HWND__;
typedef HWND__* HWND;

namespace HogwartsMP::Networking {
    class NetworkClient;
}

namespace HogwartsMP::Core {

enum class RendererBackend {
    D3D12,
    Vulkan
};

enum class PlatformBackend {
    Win32,
    SDL2
};

struct ClientOptions {
    std::string gameName = "HogwartsMP";
    std::string gameVersion = "2.0.0";
    uint64_t discordAppId = 0;
    bool useRenderer = true;
    bool useImGUI = true;

    struct {
        RendererBackend backend = RendererBackend::D3D12;
        PlatformBackend platform = PlatformBackend::Win32;

        struct {
            ID3D12Device* device = nullptr;
            ID3D12CommandQueue* commandQueue = nullptr;
            IDXGISwapChain3* swapchain = nullptr;
        } d3d12;

        HWND windowHandle = nullptr;
    } renderer;
};

class ClientInstance {
protected:
    ClientOptions _options;
    bool _initialized = false;

    // Core subsystems
    std::unique_ptr<flecs::world> _world;
    std::unique_ptr<Networking::NetworkClient> _networkClient;

public:
    ClientInstance(); // Defined in .cpp to allow forward declaration
    virtual ~ClientInstance(); // Defined in .cpp to allow forward declaration

    // Main lifecycle
    virtual bool Init(ClientOptions opts);
    virtual void Update();
    virtual void Shutdown();

    // Hooks for derived classes (called by Init/Update/Shutdown)
    virtual bool PostInit() = 0;
    virtual void PostUpdate() = 0;
    virtual void PostRender() = 0;
    virtual bool PreShutdown() = 0;

    // Accessors
    flecs::world* GetWorld() const { return _world.get(); }
    Networking::NetworkClient* GetNetworkClient() const { return _networkClient.get(); }
    const ClientOptions& GetOptions() const { return _options; }
    bool IsInitialized() const { return _initialized; }

    // Renderer accessors
    ID3D12Device* GetD3D12Device() const { return _options.renderer.d3d12.device; }
    ID3D12CommandQueue* GetCommandQueue() const { return _options.renderer.d3d12.commandQueue; }
    IDXGISwapChain3* GetSwapChain() const { return _options.renderer.d3d12.swapchain; }
    HWND GetWindowHandle() const { return _options.renderer.windowHandle; }
};

} // namespace HogwartsMP::Core
