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
    // State machine handlers
    static void handleIdle(KeyData& key, uint16_t adc, uint32_t t_us, 
                          uint8_t mux, uint8_t channel);
    static void handleTracking(KeyData& key, uint16_t adc, uint32_t t_us, 
                              uint8_t mux, uint8_t channel);
    static void handleHeld(KeyData& key, uint16_t adc, uint32_t t_us,
                          uint8_t mux, uint8_t channel);
    static void handleRearmed(KeyData& key, uint16_t adc, uint32_t t_us,
                             uint8_t mux, uint8_t channel);
    
    // Utility functions
    static uint8_t calculateVelocity(uint16_t delta_adc, uint32_t delta_t_us);
    static bool isStableTransition(KeyData& key, uint16_t adc, bool rising);
    static void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t mux, uint8_t channel);
    static void sendNoteOff(uint8_t note, uint8_t mux, uint8_t channel);
    static void resetKey(KeyData& key);
    
    // Debug helpers
    static void logStateChange(uint8_t mux, uint8_t channel, 
                              KeyState old_state, KeyState new_state, 
                              uint16_t adc, uint8_t velocity = 0);
};
