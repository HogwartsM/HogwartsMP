#pragma once

#include "core/server.h"
#include <flecs.h>

namespace HogwartsMP::Networking {
    class NetworkServer;
}

namespace HogwartsMP::Core::Modules {
    class Human {
    public:
        Human(flecs::world &world);

        static void Create(HogwartsMP::Networking::NetworkServer *net, flecs::entity e);

        static void SetupMessages(flecs::world* world, HogwartsMP::Networking::NetworkServer *net);
    };
} // namespace HogwartsMP::Core::Modules
