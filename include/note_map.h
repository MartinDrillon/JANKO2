#pragma once
#include <Arduino.h>
#include "config.h"

// === MIDI Note Mapping for 8x16 MUX Configuration ===

// Sentinel value for disabled keys
constexpr int8_t DISABLED = -1;

// Note mapping table [mux][channel] -> MIDI note (0-127) or DISABLED
extern const int8_t kNoteMap[8][16];

// Global transpose value (semitones) applied by effectiveNote
extern int8_t gTranspose;

// Setter to update global transpose (from IoState)
void noteMapSetTranspose(int8_t semitones);

// Get effective MIDI note for given MUX and channel
// Returns DISABLED if key is disabled or note out of range
// Note: Transpose will be applied later when controlled by hardware pin
int8_t effectiveNote(uint8_t mux, uint8_t channel);

// Legacy function for compatibility (will be removed)
inline uint8_t getMidiNote(uint8_t mux, uint8_t channel) {
    int8_t note = effectiveNote(mux, channel);
    return (note >= 0) ? (uint8_t)note : 60; // Default to C4 for disabled keys
}

// === Debug Helpers ===
inline const char* getNoteNameC(uint8_t midiNote) {
    static const char* noteNames[] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    return noteNames[midiNote % 12];
}

inline uint8_t getNoteOctave(uint8_t midiNote) {
    return (midiNote / 12) - 1;
}

// Debug function to print current note mapping
void printNoteMap();

// Note: hardware pin polling is centralized in IoState (io_state.*)

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
