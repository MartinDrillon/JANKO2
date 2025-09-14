#include "velocity_engine.h"

// === Global State Arrays Definition ===
KeyData g_keys[N_MUX][N_CH];
AcquisitionData g_acquisition;

// === VelocityEngine Implementation ===

void VelocityEngine::initialize() {
    // Initialize all keys to IDLE state
    for (uint8_t mux = 0; mux < N_MUX; mux++) {
        for (uint8_t channel = 0; channel < N_CH; channel++) {
            resetKey(g_keys[mux][channel]);
        }
    }
    
    // Initialize acquisition buffers
    memset(&g_acquisition, 0, sizeof(g_acquisition));
    
    Serial.println("VelocityEngine: Initialized for 4x16 MUX configuration");
    Serial.print("Thresholds: Low="); Serial.print(kThresholdLow);
    Serial.print(", High="); Serial.print(kThresholdHigh);
    Serial.print(", Release="); Serial.println(kThresholdRelease);
}

void VelocityEngine::processKey(uint8_t mux, uint8_t channel, 
                               uint16_t adc_value, uint32_t timestamp_us) {
    // Bounds checking
    if (mux >= N_MUX || channel >= N_CH) {
        return;
    }
    
    // Store in acquisition buffer
    g_acquisition.workingValues[mux][channel] = adc_value;
    g_acquisition.t_sample_us[mux][channel] = timestamp_us;
    
    KeyData& key = g_keys[mux][channel];
    KeyState oldState = key.state;
    
    // Update last measurement
    key.last_adc = adc_value;
    key.last_sample_us = timestamp_us;
    
    // State machine dispatch
    switch (key.state) {
        case KeyState::IDLE:
            handleIdle(key, adc_value, timestamp_us, mux, channel);
            break;
            
        case KeyState::TRACKING:
            handleTracking(key, adc_value, timestamp_us, mux, channel);
            break;
            
        case KeyState::HELD:
            handleHeld(key, adc_value, timestamp_us, mux, channel);
            break;
            
        case KeyState::REARMED:
            handleRearmed(key, adc_value, timestamp_us, mux, channel);
            break;
    }
    
    // Log state changes for debugging
    if (oldState != key.state) {
        logStateChange(mux, channel, oldState, key.state, adc_value, key.current_velocity);
    }
}

void VelocityEngine::handleIdle(KeyData& key, uint16_t adc, uint32_t t_us, 
                               uint8_t mux, uint8_t channel) {
    if (adc > kThresholdLow) {
        key.stable_up_count++;
        if (key.stable_up_count >= kStableCount) {
            // START TRACKING
            key.state = KeyState::TRACKING;
            key.adc_start = adc;
            key.t_start_us = t_us;
            key.stable_up_count = 0;
            key.stable_down_count = 0;
        }
    } else {
        key.stable_up_count = 0; // reset counter
    }
}

void VelocityEngine::handleTracking(KeyData& key, uint16_t adc, uint32_t t_us, 
                                   uint8_t mux, uint8_t channel) {
    // Check timeout
    if (t_us - key.t_start_us > kTrackingTimeoutUs) {
        key.false_starts++;
        resetKey(key); // too slow, abort
        return;
    }
    
    if (adc < kThresholdLow) {
        // FALSE START - fell below threshold before hitting high
        key.false_starts++;
        resetKey(key);
        return;
    }
    
    if (adc >= kThresholdHigh) {
        key.stable_up_count++;
        if (key.stable_up_count >= kStableCount) {
            // NOTE TRIGGERED!
            uint16_t delta_adc = max(adc - key.adc_start, kMinDeltaADC);
            uint32_t delta_t = t_us - key.t_start_us;
            uint8_t velocity = calculateVelocity(delta_adc, delta_t);
            uint8_t note = getMidiNote(mux, channel);
            
            sendNoteOn(note, velocity, mux, channel);
            key.note_on_sent = true;
            key.current_note = note;
            key.current_velocity = velocity;
            key.state = KeyState::HELD;
            key.stable_up_count = 0;
            key.total_triggers++;
        }
    } else {
        key.stable_up_count = 0;
    }
}

void VelocityEngine::handleHeld(KeyData& key, uint16_t adc, uint32_t t_us,
                               uint8_t mux, uint8_t channel) {
    if (adc < kThresholdRelease) {
        key.stable_down_count++;
        if (key.stable_down_count >= kStableCount) {
            // NOTE RELEASED
            sendNoteOff(key.current_note, mux, channel);
            key.note_on_sent = false;
            key.state = KeyState::REARMED; // Ready for re-trigger
            key.stable_down_count = 0;
        }
    } else {
        key.stable_down_count = 0;
    }
    
    // Optional: aftertouch/pressure sensing could be added here
}

void VelocityEngine::handleRearmed(KeyData& key, uint16_t adc, uint32_t t_us,
                                  uint8_t mux, uint8_t channel) {
    // Ready for re-trigger without going back to ThresholdLow
    if (adc > kThresholdLow) {
        key.stable_up_count++;
        if (key.stable_up_count >= kStableCount) {
            // Re-trigger detected
            key.state = KeyState::TRACKING;
            key.adc_start = adc;
            key.t_start_us = t_us;
            key.stable_up_count = 0;
        }
    } else {
        key.stable_up_count = 0;
        if (adc < kThresholdLow - 20) { // small hysteresis
            // Full release - back to IDLE
            key.state = KeyState::IDLE;
        }
    }
}

uint8_t VelocityEngine::calculateVelocity(uint16_t delta_adc, uint32_t delta_t_us) {
    // Convert to speed (counts/second)
    float speed = (float)delta_adc / ((float)delta_t_us / 1000000.0f);
    
    // Linear mapping to MIDI range [1-127]
    float normalized = (speed - kSpeedMin) / (kSpeedMax - kSpeedMin);
    normalized = constrain(normalized, 0.0f, 1.0f);
    
    uint8_t velocity = 1 + (uint8_t)(normalized * 126.0f);
    return constrain(velocity, 1, 127);
}

void VelocityEngine::sendNoteOn(uint8_t note, uint8_t velocity, uint8_t mux, uint8_t channel) {
    usbMIDI.sendNoteOn(note, velocity, 1); // Channel 1
    
    // Debug output uniquement pour canal 6
    if (channel == 6) {
        Serial.print("NOTE ON: CH"); Serial.print(channel);
        Serial.print(" Note="); Serial.print(note);
        Serial.print(" Vel="); Serial.println(velocity);
    }
}

void VelocityEngine::sendNoteOff(uint8_t note, uint8_t mux, uint8_t channel) {
    usbMIDI.sendNoteOff(note, 0, 1); // Channel 1
    
    // Debug output uniquement pour canal 6
    if (channel == 6) {
        Serial.print("NOTE OFF: CH"); Serial.print(channel);
        Serial.print(" Note="); Serial.println(note);
    }
}

void VelocityEngine::resetKey(KeyData& key) {
    key.state = KeyState::IDLE;
    key.adc_start = 0;
    key.t_start_us = 0;
    key.note_on_sent = false;
    key.current_note = 0;
    key.current_velocity = 0;
    key.stable_up_count = 0;
    key.stable_down_count = 0;
}

void VelocityEngine::logStateChange(uint8_t mux, uint8_t channel, 
                                   KeyState old_state, KeyState new_state, 
                                   uint16_t adc, uint8_t velocity) {
    // Debug uniquement pour le canal surveillé
    if (channel == 6) {  // Canal 6 seulement
        Serial.print("CH"); Serial.print(channel);
        Serial.print(": "); Serial.print(getStateName(old_state));
        Serial.print("→"); Serial.print(getStateName(new_state));
        Serial.print(" ADC="); Serial.print(adc);
        if (velocity > 0) {
            Serial.print(" Vel="); Serial.print(velocity);
        }
        Serial.println();
    }
}

void VelocityEngine::printKeyStats(uint8_t mux, uint8_t channel) {
    if (mux >= N_MUX || channel >= N_CH) return;
    
    KeyData& key = g_keys[mux][channel];
    Serial.print("CH"); Serial.print(channel);
    Serial.print(": State="); Serial.print(getStateName(key.state));
    Serial.print(" ADC="); Serial.print(key.last_adc);
    Serial.print(" Triggers="); Serial.print(key.total_triggers);
    Serial.print(" FalseStarts="); Serial.println(key.false_starts);
}

void VelocityEngine::printAllActiveKeys() {
    bool found_active = false;
    for (uint8_t mux = 0; mux < N_MUX; mux++) {
        for (uint8_t channel = 0; channel < N_CH; channel++) {
            KeyData& key = g_keys[mux][channel];
            if (key.state != KeyState::IDLE || key.total_triggers > 0) {
                if (!found_active) {
                    Serial.println("=== Active Keys ===");
                    found_active = true;
                }
                printKeyStats(mux, channel);
            }
        }
    }
    if (!found_active) {
        Serial.println("No active keys");
    }
}
