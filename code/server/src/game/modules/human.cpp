#include "human.h"
#include "../../../../shared/logging/logger.h"

namespace HogwartsMP::Core::Modules {

    Human::Human(flecs::world &world) {
        world.module<Human>();
        Logging::Logger::Info("Human module registered");
    }

    void Human::Create(HogwartsMP::Networking::NetworkServer *net, flecs::entity e) {
        // TODO: Create human entity with components
        // TODO: Setup spawn/despawn/update callbacks
        Logging::Logger::InfoF("Human entity created: %llu", e.id());
    }

    void Human::SetupMessages(flecs::world* world, HogwartsMP::Networking::NetworkServer *net) {
        // TODO: Register human spawn/despawn/update message handlers
        // TODO: Setup synchronization callbacks
        Logging::Logger::Info("Human messages setup (placeholder)");
    }

} // namespace HogwartsMP::Core::Modules
