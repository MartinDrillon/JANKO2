#pragma once
#include <Arduino.h>
#include "calibration.h"
#include "note_map.h"

// === Key State Machine ===
enum class KeyState : uint8_t {
    IDLE,        // at rest, waiting for trigger
    TRACKING,    // rising, measuring velocity  
    HELD,        // note active, waiting for release
    REARMED      // released but ready for re-trigger
};

// === Per-Key State Data ===
struct KeyData {
    KeyState state = KeyState::IDLE;
    
    // Velocity tracking
    uint16_t adc_start = 0;
    uint32_t t_start_us = 0;
    
    // MIDI state
    bool note_on_sent = false;
    uint8_t current_note = 0;
    uint8_t current_velocity = 0;
    
    // Anti-bounce counters
    uint8_t stable_up_count = 0;
    uint8_t stable_down_count = 0;
    
    // Last measurement
    uint16_t last_adc = 0;
    uint32_t last_sample_us = 0;
    
    // Debug/monitoring
    uint32_t total_triggers = 0;
    uint32_t false_starts = 0;
};

// === Global Key State Array ===
// [mux][channel] → KeyData
extern KeyData g_keys[kMaxMux][kMaxChannels];

// === State Names for Debug ===
inline const char* getStateName(KeyState state) {
    switch (state) {
        case KeyState::IDLE: return "IDLE";
        case KeyState::TRACKING: return "TRACK";
        case KeyState::HELD: return "HELD";
        case KeyState::REARMED: return "REARM";
        default: return "UNKNOWN";
    }
}
