#pragma once
#include <Arduino.h>
#include "config.h"

// === MIDI Note Mapping for 8x16 MUX Configuration ===

// Get MIDI note number for given MUX and channel
// Temporary mapping: linear progression from C3 (note 48)
inline uint8_t getMidiNote(uint8_t mux, uint8_t channel) {
    // Bounds checking
    if (mux >= N_MUX || channel >= N_CH) {
        return 60; // Default to C4
    }
    
    // Linear mapping: MUX0-CH0 = note 48 (C3)
    // Each MUX gets 16 consecutive notes
    // 128 total notes covering 10+ octaves
    return 48 + (mux * N_CH) + channel;
}

// === Debug Helper ===
inline const char* getNoteNameC(uint8_t midiNote) {
    static const char* noteNames[] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    return noteNames[midiNote % 12];
}

inline uint8_t getNoteOctave(uint8_t midiNote) {
    return (midiNote / 12) - 1;
}

// === 8-MUX Note Layout for Testing ===
// Group A (ADC1):
// MUX0: notes 48-63   (C3 to D#4)
// MUX1: notes 64-79   (E4 to G5) 
// MUX2: notes 80-95   (G#5 to B6)
// MUX3: notes 96-111  (C7 to D#8)
// 
// Group B (ADC0):
// MUX4: notes 112-127 (E8 to G9)
// MUX5: notes 128-143 (G#9 to B10) - might exceed MIDI range
// MUX6: notes 144-159 (C11 to D#12) - might exceed MIDI range
// MUX7: notes 160-175 (E12 to G13) - might exceed MIDI range
// 
// Note: MIDI standard goes up to 127, higher notes for testing only
