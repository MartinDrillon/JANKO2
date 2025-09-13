#pragma once
#include <Arduino.h>

// === Note Mapping Configuration ===
static constexpr uint8_t kMaxMux = 1;        // Currently using 1 MUX
static constexpr uint8_t kMaxChannels = 16;  // 16 channels per MUX// === MIDI Note Mapping Table ===
// [mux][channel] → MIDI note number
// For now: all point to C4 (middle C = 60), customize later for Janko layout
static constexpr uint8_t kNoteMap[kMaxMux][kMaxChannels] = {
    // MUX 0 - Test mapping with different notes for debugging
    {60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75}
    // C4, C#4, D4, D#4, E4, F4, F#4, G4, G#4, A4, A#4, B4, C5, C#5, D5, D#5
};

// === Utility Functions ===
inline uint8_t getMidiNote(uint8_t mux, uint8_t channel) {
    if (mux >= kMaxMux || channel >= kMaxChannels) {
        return 60; // Default to C4 if out of bounds
    }
    return kNoteMap[mux][channel];
}
