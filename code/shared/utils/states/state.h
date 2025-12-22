#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>

// Simple state machine system for client states
namespace Framework::Utils::States {
    class Machine; // Forward declaration

    // Base interface for all states
    class IState {
    public:
        virtual ~IState() = default;

        virtual const char* GetName() const = 0;
        virtual int32_t GetId() const = 0;

        virtual bool OnEnter(Machine* machine) = 0;
        virtual bool OnExit(Machine* machine) = 0;
        virtual bool OnUpdate(Machine* machine) = 0;
    };

    // State machine that manages state transitions
    class Machine {
    public:
        Machine() : _currentState(nullptr) {}

        void RegisterState(int32_t id, std::shared_ptr<IState> state) {
            _states[id] = state;
        }

        bool TransitionTo(int32_t stateId) {
            auto it = _states.find(stateId);
            if (it == _states.end()) {
                return false;
            }

            if (_currentState) {
                _currentState->OnExit(this);
            }

            _currentState = it->second;
            return _currentState->OnEnter(this);
        }

        void Update() {
            if (_currentState) {
                _currentState->OnUpdate(this);
            }
        }

        IState* GetCurrentState() const {
            return _currentState.get();
        }

    private:
        std::shared_ptr<IState> _currentState;
        std::unordered_map<int32_t, std::shared_ptr<IState>> _states;
    };
}
