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
    
    // Zero acquisition buffers
    memset(&g_acquisition, 0, sizeof(g_acquisition));
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
    KeyState oldState = key.state;
    
    // Update last measurement
    key.last_adc = adc_value;
    key.last_sample_us = timestamp_us;
    
    // State machine dispatch
    uint16_t thLow  = calibLow(mux, channel);
    uint16_t thHigh = calibHigh(mux, channel);
    uint16_t thRel  = calibRelease(mux, channel);

    switch (key.state) {
        case KeyState::IDLE:
            if (adc_value >= thLow) {
                key.state = KeyState::TRACKING;
                key.adc_start = adc_value;
                key.t_start_us = timestamp_us;
                key.current_velocity = 0;
                key.peak_adc = adc_value; // nouveau champ (sera ajouté dans struct)
            }
            break;
        case KeyState::TRACKING: {
            if (adc_value < thLow) {
                resetKey(key);
                break;
            }
            if (adc_value > key.peak_adc) key.peak_adc = adc_value;
            if (adc_value >= thHigh) {
                int8_t note = effectiveNote(mux, channel);
                if (note == DISABLED) { resetKey(key); break; }
                uint16_t delta_adc = (adc_value > key.adc_start) ? (adc_value - key.adc_start) : 1;
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
            if (adc_value > key.peak_adc) key.peak_adc = adc_value;
            if (adc_value < thRel) {
                sendNoteOff(key.current_note, mux, channel);
                key.note_on_sent = false;
                // mise à jour adaptative du High
                updateHighAfterNote(mux, channel, key.peak_adc);
                key.state = KeyState::REARMED;
            }
            break;
        case KeyState::REARMED:
            if (adc_value < (thLow - 10)) {
                key.state = KeyState::IDLE; // fully released deep
            } else if (adc_value >= thLow) {
                key.state = KeyState::TRACKING;
                key.adc_start = adc_value;
                key.t_start_us = timestamp_us;
                key.peak_adc = adc_value;
            }
            break;
    }
    
    // Log state changes for debugging
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
}

void VelocityEngine::printKeyStats(uint8_t, uint8_t) {}
void VelocityEngine::printAllActiveKeys() {}
