#pragma once
#include <Arduino.h>
#include "key_state.h"
#include "calibration.h"
#include "note_map.h"

// === Velocity Calculation Engine ===
class VelocityEngine {
public:
    // Initialize the velocity engine
    static void initialize();
    
    // Process single key measurement - MAIN ENTRY POINT
    static void processKey(uint8_t mux, uint8_t channel, 
                          uint16_t adc_value, uint32_t timestamp_us);
    
    // Debug/monitoring functions
    static void printKeyStats(uint8_t mux, uint8_t channel);
    static void printAllActiveKeys();
    
private:
    // Simplified inline state machine in processKey; legacy handlers removed.
    static void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t mux, uint8_t channel);
    static void sendNoteOff(uint8_t note, uint8_t mux, uint8_t channel);
    static void resetKey(KeyData& key);
    // Debug helpers removed (no Serial output allowed)
};
