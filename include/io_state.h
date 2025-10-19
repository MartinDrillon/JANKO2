#pragma once
#include <Arduino.h>

namespace IoState {
    // Rocker pins
    constexpr uint8_t kPinRocker4 = 4;
    constexpr uint8_t kPinRocker5 = 5;
    // Button pin
    constexpr uint8_t kPinButton24 = 24;
    // Encoder pins - Quadrature A/B
    constexpr uint8_t kPinEncA = 26; // channel A
    constexpr uint8_t kPinEncB = 28; // channel B

    // Button 24 click detection
    enum class Btn24Click { None, Short, Double, Triple };

    struct RockerStatus {
        bool pin4High;
        bool pin5High;
        int8_t transpose; // -12, 0, +12
        bool button24Low; // true when pressed (LOW)
        int8_t encDelta;  // -n..+n steps since last poll (CCW negative, CW positive)
        Btn24Click btn24Click; // None, Short (<300ms), Double, Triple
    };

    // Initialize pins (INPUT_PULLUP). Call once in setup.
    void init();

    // Non-blocking, rate-limited polling (~100 Hz). Returns true if state changed and fills outStatus.
    bool update(uint32_t nowMs, RockerStatus& outStatus);
}
