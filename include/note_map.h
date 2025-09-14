#pragma once
#include <Arduino.h>
#include "config.h"

// === MIDI Note Mapping for 4x16 MUX Configuration ===

// Get MIDI note number for given MUX and channel
// Temporary mapping: all notes start from C4 (note 60)
inline uint8_t getMidiNote(uint8_t mux, uint8_t channel) {
    // Bounds checking
    if (mux >= N_MUX || channel >= N_CH) {
        return 60; // Default to C4
    }
    
    // Simple linear mapping: MUX0-CH0 = note 60 (C4)
    // Each MUX gets 16 consecutive notes
    return 60 + (mux * N_CH) + channel;
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

// === Temporary Note Layout for Testing ===
// MUX0: notes 60-75  (C4 to D#5)
// MUX1: notes 76-91  (E5 to G6) 
// MUX2: notes 92-107 (G#6 to B7)
// MUX3: notes 108-123 (C8 to D#9)
// Total: 64 notes covering 5+ octaves
