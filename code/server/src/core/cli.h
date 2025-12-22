#pragma once

#include <string>
#include <functional>
#include <map>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

namespace HogwartsMP::Core {

using CommandCallback = std::function<void(const std::vector<std::string>& args)>;

class CLI {
public:
    CLI();
    ~CLI();

    void Start();
    void Stop();

    void RegisterCommand(const std::string& name, CommandCallback callback, const std::string& description = "");
    void ExecuteCommand(const std::string& line);

private:
    void InputLoop();

    std::map<std::string, std::pair<CommandCallback, std::string>> _commands;
    std::thread _inputThread;
    std::atomic<bool> _running;
    std::mutex _mutex;
};

} // namespace HogwartsMP::Core
