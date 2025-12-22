#pragma once

#include "server_instance.h"
#include "cli/cli.h"
#include <memory>

namespace HogwartsMP {
    class Server: public Core::ServerInstance {
      private:
        void InitNetworkingMessages();
        void InitCommands();
        
        std::unique_ptr<Core::CLI> _cli;
        
        uint64_t _lastCoordinateRequestTime = 0;

      public:
        Server();
        ~Server() override;

        bool PostInit() override;

        void PostUpdate() override;

        bool PreShutdown() override;

        void BroadcastChatMessage(const std::string &msg);

        void InitRPCs();

        static inline Server *_serverRef = nullptr;
    };
} // namespace HogwartsMP
