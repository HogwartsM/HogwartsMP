#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <optional>

// Pattern scanning utilities for finding code patterns in memory
namespace hook {
    // Get base address (defined in main.cpp)
    extern uintptr_t g_base;
    void set_base(uintptr_t base);
    uintptr_t get_base();

    // Simple pattern matching result
    class pattern_match {
    public:
        pattern_match(uintptr_t address) : _address(address) {}

        uintptr_t get_first() const {
            return _address;
        }

        operator uintptr_t() const {
            return _address;
        }

    private:
        uintptr_t _address;
    };

    // Pattern scanner
    inline pattern_match pattern(const char* pattern_string) {
        // TODO: Implement actual pattern scanning
        // For now, return 0 to allow compilation
        // This will need to be implemented for actual hooking to work
        return pattern_match(0);
    }

    // Get address from opcode pattern with wildcards
    inline uintptr_t get_opcode_address(const char* pattern_string) {
        // TODO: Implement actual pattern scanning with opcode resolution
        // For now, return 0 to allow compilation
        return 0;
    }
}
