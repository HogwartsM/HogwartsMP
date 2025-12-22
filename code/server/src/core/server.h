#pragma once

#include "server_instance.h"

namespace HogwartsMP {
    class Server: public Core::ServerInstance {
      private:
        void InitNetworkingMessages();

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
