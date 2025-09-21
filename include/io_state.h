#pragma once
#include <Arduino.h>

namespace IoState {
    // Rocker pins
    constexpr uint8_t kPinRocker4 = 4;
    constexpr uint8_t kPinRocker5 = 5;

    struct RockerStatus {
        bool pin4High;
        bool pin5High;
        int8_t transpose; // -12, 0, +12
    };

    // Initialize pins (INPUT_PULLUP). Call once in setup.
    void init();

    // Non-blocking, rate-limited polling (~100 Hz). Returns true if state changed and fills outStatus.
    bool update(uint32_t nowMs, RockerStatus& outStatus);
}
