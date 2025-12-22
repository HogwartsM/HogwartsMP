#pragma once

#include <functional>
#include <vector>
#include <string>

// InitFunction system for registering hooks to be initialized later
class InitFunction {
public:
    using InitFunc = std::function<void()>;

    InitFunction(InitFunc func, const char* name = "") {
        GetInitFunctions().push_back({func, name});
    }

    static void RunAll() {
        for (auto& [func, name] : GetInitFunctions()) {
            func();
        }
    }

private:
    struct FunctionEntry {
        InitFunc func;
        std::string name;
    };

    static std::vector<FunctionEntry>& GetInitFunctions() {
        static std::vector<FunctionEntry> initFunctions;
        return initFunctions;
    }
};
