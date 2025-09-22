#pragma once
#include <Arduino.h>
#include "config.h"
// Removed calibration include (thresholds not needed for state struct definitions)

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

    // Peak tracking pour adaptation High dynamique
    uint16_t peak_adc = 0;

    // Re-press valley tracking (ThresholdMed):
    // When key transitions HELD → REARMED (released but not fully), we start tracking
    // the lowest ADC reached before the next press. If the next press occurs before
    // returning to ThresholdLow, velocity timing will start from this valley so that
    // dv/dt remains consistent with a full stroke at the same physical speed.
    uint16_t rearm_min_adc = 0;     // minimum ADC after release before re-press
    uint32_t rearm_min_t_us = 0;    // timestamp when that minimum was observed
    
    // Debug/monitoring
    uint32_t total_triggers = 0;
    uint32_t false_starts = 0;
};

// === Global Key State Array ===
// [mux][channel] → KeyData using config.h constants (8x16 = 128 keys)
extern KeyData g_keys[N_MUX][N_CH];

// === Acquisition Buffers ===
struct AcquisitionData {
    // Current working values (being written during scan)
    uint16_t workingValues[N_MUX][N_CH];
    uint32_t t_sample_us[N_MUX][N_CH];
    
    // Snapshot values (stable for debug/logging)
    uint16_t snapshotValues[N_MUX][N_CH];
    uint32_t snapshotTimestamp_us;
    
    // Frame management
    volatile bool frameReady = false;
    uint32_t frameCounter = 0;
    
    void swapBuffers() {
        // Copy working → snapshot
        memcpy(snapshotValues, workingValues, sizeof(snapshotValues));
        snapshotTimestamp_us = micros();
        frameReady = true;
        frameCounter++;
    }
};

extern AcquisitionData g_acquisition;

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
