#pragma once
#include <Arduino.h>

// Simple base4-style velocity computation
// delta_adc: difference between thresholdHigh and starting ADC value (>=1)
// dt_us: time in microseconds between start and trigger
// Returns MIDI velocity 1..127
inline uint8_t computeVelocity(uint16_t delta_adc, uint32_t dt_us) {
    if (dt_us == 0) return 127; // extreme edge case
    // Convert to speed (ADC counts per microsecond)
    float speed = static_cast<float>(delta_adc) / static_cast<float>(dt_us);
    // Empirical min/max (same spirit as base4 prototype)
    constexpr float kMinSpeed = 0.001f;  // very slow
    constexpr float kMaxSpeed = 0.05f;   // very fast
    float norm = (speed - kMinSpeed) / (kMaxSpeed - kMinSpeed);
    if (norm < 0.f) norm = 0.f;
    if (norm > 1.f) norm = 1.f;
    uint8_t vel = 1 + static_cast<uint8_t>(norm * 126.0f);
    if (vel < 1) vel = 1;
    if (vel > 127) vel = 127;
    return vel;
}
