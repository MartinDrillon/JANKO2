#include "velocity_engine.h"
#include "velocity_calc.h"
#include "config.h"
#include "calibration.h"
#include "midi_out.h"

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
    
    // Zero acquisition buffers (value-initialize to avoid class-memaccess warnings)
    g_acquisition = AcquisitionData{};
}

void VelocityEngine::processKey(uint8_t mux, uint8_t channel, 
                               uint16_t adc_value, uint32_t timestamp_us) {
    // Bounds checking
    if (mux >= N_MUX || channel >= N_CH) {
        return;
    }
    
    g_acquisition.workingValues[mux][channel] = adc_value;
    g_acquisition.t_sample_us[mux][channel] = timestamp_us;
    
    KeyData& key = g_keys[mux][channel];
    //KeyState oldState = key.state; // unused in release
    
    // Preserve previous measurement for edge detection
    const uint16_t prev_adc = key.last_adc;
    const uint32_t prev_t_us = key.last_sample_us;
    
    // State machine dispatch
    uint16_t thLow  = calibLow(mux, channel);
    uint16_t thHigh = calibHigh(mux, channel);
    uint16_t thRel  = calibRelease(mux, channel);
    // Determine polarity sign based on thresholds
    const int s = ((int)thHigh >= (int)thLow) ? +1 : -1; // +1 normal (press increases), -1 inverted (press decreases)
    auto sCmp = [s](int lhs_minus_rhs) -> int { return s * lhs_minus_rhs; };
    // Use config-defined hysteresis and stability for re-press rising detection
    constexpr uint16_t kRepressHystCfg = kRepressHyst;
    constexpr uint8_t  kRepressStableCountCfg = kRepressStableCount;
    constexpr float    kRepressMinReturnPctCfg = kRepressMinReturnPct;

    switch (key.state) {
        case KeyState::IDLE:
            // Start when crossing ThresholdLow in the press direction (true edge)
            if (sCmp((int)adc_value - (int)thLow) >= 0 && sCmp((int)prev_adc - (int)thLow) < 0) {
                key.state = KeyState::TRACKING;
                key.adc_start = adc_value;
                key.t_start_us = timestamp_us;
                key.current_velocity = 0;
                key.peak_adc = adc_value; // nouveau champ (sera ajouté dans struct)
            }
            break;
        case KeyState::TRACKING: {
            // Abort if we returned past ThresholdLow opposite the press direction
            if (sCmp((int)adc_value - (int)thLow) < 0) {
                resetKey(key);
                break;
            }
            // Update peak along the press direction
            if (sCmp((int)adc_value - (int)key.peak_adc) > 0) key.peak_adc = adc_value;
            // Trigger when crossing ThresholdHigh in the press direction
            if (sCmp((int)adc_value - (int)thHigh) >= 0) {
                int8_t note = effectiveNote(mux, channel);
                if (note == DISABLED) { resetKey(key); break; }
                int delta_s = sCmp((int)adc_value - (int)key.adc_start);
                uint16_t delta_adc = (delta_s > 0) ? (uint16_t)delta_s : 1;
                uint32_t dt = (timestamp_us > key.t_start_us) ? (timestamp_us - key.t_start_us) : 1;
                uint8_t velocity = computeVelocity(delta_adc, dt);
                sendNoteOn((uint8_t)note, velocity, mux, channel);
                key.note_on_sent = true;
                key.current_note = (uint8_t)note;
                key.current_velocity = velocity;
                key.state = KeyState::HELD;
                key.total_triggers++;
            }
            break; }
        case KeyState::HELD:
            if (sCmp((int)adc_value - (int)key.peak_adc) > 0) key.peak_adc = adc_value;
            // Release when crossing release threshold opposite the press direction
            if (sCmp((int)adc_value - (int)thRel) < 0) {
                // Note considered released (hysteresis), transition to REARMED
                sendNoteOff(key.current_note, mux, channel);
                key.note_on_sent = false;
                // Update adaptive High peak per key
                updateHighAfterNote(mux, channel, key.peak_adc);
                key.state = KeyState::REARMED;
                // Initialize re-press valley tracking (ThresholdMed)
                key.rearm_min_adc = adc_value;
                key.rearm_min_t_us = timestamp_us;
                key.stable_up_count = 0;
                key.stable_down_count = 0;
            }
            break;
        case KeyState::REARMED: {
            // Track extremum after release in the release direction (min for s=+1, max for s=-1)
            if (sCmp((int)adc_value - (int)key.rearm_min_adc) < 0) {
                key.rearm_min_adc = adc_value;
                key.rearm_min_t_us = timestamp_us;
                key.stable_up_count = 0; // reset rising stability when still moving away from press direction
            }

            // If fully released deeply (moved 10 counts opposite the press direction), go back to IDLE
            if (sCmp((int)adc_value - (int)thLow) <= -10) {
                key.state = KeyState::IDLE;
                break;
            }

            // Detect re-press before returning to ThresholdLow:
            // Requires two conditions:
            // 1. Valley must have returned at least kRepressMinReturnPct% of total swing (|High-Low|)
            // 2. Current value rising above valley + hysteresis
            int32_t swing = abs((int)thHigh - (int)thLow);
            int32_t valley_return = abs((int)key.rearm_min_adc - (int)thHigh); // Distance from High to valley
            int32_t min_return_required = (int32_t)(kRepressMinReturnPctCfg * (float)swing);
            
            if (valley_return >= min_return_required && 
                sCmp((int)adc_value - (int)key.rearm_min_adc) > (int)kRepressHystCfg) {
                key.stable_up_count++;
                if (key.stable_up_count >= kRepressStableCountCfg) {
                    key.state = KeyState::TRACKING;
                    key.adc_start = key.rearm_min_adc;        // start from valley (ThresholdMed)
                    key.t_start_us = key.rearm_min_t_us;
                    key.current_velocity = 0;
                    key.peak_adc = adc_value;
                    key.stable_up_count = 0;
                    key.stable_down_count = 0;
                }
            } else {
                // Not yet rising stably or insufficient return
                if (key.stable_up_count > 0) key.stable_up_count--;
            }
            break; }
    }
    // Update last measurement after processing
    key.last_adc = adc_value;
    key.last_sample_us = timestamp_us;

    // (Debug logging removed)
}

// Removed advanced handlers & velocity math (simplified inline in processKey switch)

void VelocityEngine::sendNoteOn(uint8_t note, uint8_t velocity, uint8_t mux, uint8_t channel) {
    MidiOut::Event ev{MidiOut::Kind::NoteOn, kMidiChannel, note, velocity};
    if (!MidiOut::enqueue(ev)) {
        // Fallback to immediate send to avoid missed notes if queue is saturated
        usbMIDI.sendNoteOn(note, velocity, kMidiChannel);
        usbMIDI.send_now();
    }
}

void VelocityEngine::sendNoteOff(uint8_t note, uint8_t mux, uint8_t channel) {
    MidiOut::Event ev{MidiOut::Kind::NoteOff, kMidiChannel, note, 0};
    if (!MidiOut::enqueue(ev)) {
        // Fallback to immediate send to avoid stuck notes
        usbMIDI.sendNoteOff(note, 0, kMidiChannel);
        usbMIDI.send_now();
    }
}

void VelocityEngine::resetKey(KeyData& key) {
    key.state = KeyState::IDLE;
    key.adc_start = 0;
    key.t_start_us = 0;
    key.note_on_sent = false;
    key.current_note = 0;
    key.current_velocity = 0;
    key.peak_adc = 0;
    key.rearm_min_adc = 0;
    key.rearm_min_t_us = 0;
}

void VelocityEngine::printKeyStats(uint8_t, uint8_t) {}
void VelocityEngine::printAllActiveKeys() {}
