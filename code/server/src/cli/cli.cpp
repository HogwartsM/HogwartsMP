#include "cli.h"
#include <iostream>
#include <sstream>
#include "../../../shared/logging/logger.h"

namespace HogwartsMP::Core {

CLI::CLI() : _running(false) {}

CLI::~CLI() {
    Stop();
}

void CLI::Start() {
    if (_running) return;
    _running = true;
    _inputThread = std::thread(&CLI::InputLoop, this);
}

void CLI::Stop() {
    if (!_running) return;
    _running = false;
    
    // Detach thread if it's joinable, as reading from stdin is blocking
    // and difficult to interrupt portably without platform-specific code.
    // In a real production environment, we might use Overlapped I/O on Windows
    // or select() on stdin on Linux.
    if (_inputThread.joinable()) {
        _inputThread.detach(); 
    }
}

void CLI::RegisterCommand(const std::string& name, CommandCallback callback, const std::string& description) {
    std::lock_guard<std::mutex> lock(_mutex);
    _commands[name] = {callback, description};
}

void CLI::ExecuteCommand(const std::string& line) {
    if (line.empty()) return;

    std::istringstream iss(line);
    std::string commandName;
    iss >> commandName;

    std::vector<std::string> args;
    std::string arg;
    while (iss >> arg) {
        args.push_back(arg);
    }

    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _commands.find(commandName);
    if (it != _commands.end()) {
        try {
            it->second.first(args);
        } catch (const std::exception& e) {
            Logging::Logger::ErrorF("Error executing command '%s': %s", commandName.c_str(), e.what());
        }
    } else {
        Logging::Logger::InfoF("Unknown command: %s. Type 'help' for a list of commands.", commandName.c_str());
    }
}

void CLI::InputLoop() {
    std::string line;
    while (_running) {
        // std::getline is blocking. 
        if (std::getline(std::cin, line)) {
            if (!_running) break;
            ExecuteCommand(line);
        } else {
            // EOF or error
            break;
        }
    }
}

} // namespace HogwartsMP::Core
