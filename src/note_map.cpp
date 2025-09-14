#include "note_map.h"

// Global transpose value (default: no transpose)
int8_t gTranspose = 0;

// Note mapping table [mux][channel] -> MIDI note
// Default: linear mapping (48 + mux*16 + channel) starting from C3
// Use DISABLED (-1) to disable specific keys
const int8_t kNoteMap[8][16] = {
    // MUX0 (Group A, ADC1): notes 48-63 (C3 to D#4)
    // CH:  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
    {    35, 38, 36, 37, -1, 34, -1, 33, 32, -1, 31, -1, 36, 33, 35, 34},
    
    // MUX1 (Group A, ADC1): notes 64-79 (E4 to G5)
    // CH:  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
    {    43, 46, 44, 45, 39, 42, 40, 41, 40, 37, 39, 38, 44, 41, 43, 42},
    
    // MUX2 (Group A, ADC1): notes 80-95 (G#5 to B6)
    // CH:  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
    {    51, 54, 52, 53, 47, 50, 48, 49, 48, 45, 47, 46, 52, 49, 51, 50},
    
    // MUX3 (Group A, ADC1): notes 96-111 (C7 to D#8)
    // CH:  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
    {    59, 62, 60, 61, 55, 58, 56, 57, 56, 53, 55, 54, 60, 57, 59, 58},
    
    // MUX4 (Group B, ADC0): notes 112-127 (E8 to G9)
    // CH:  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
    {   67, 70, 68, 69, 63, 66, 64, 65, 64, 61, 63, 62, 68, 65, 67, 66},
    
    // MUX5 (Group B, ADC0): Example with some disabled keys
    // CH:  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
    {    75, 78, 76, 77, 71, 74, 72, 73, 72, 69, 71, 70, 76, 73, 75, 74},
    
    // MUX6 (Group B, ADC0): Example chromatic from C4
    // CH:  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
    {    83, 86, 84, 85, 79, 82, 80, 81, 80, 77, 79, 78, 84, 81, 83, 82},
    
    // MUX7 (Group B, ADC0): Example with disabled keys at edges
    // CH:  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
    {    91, -1, 92, -1, 87, 90, 88, 89, 88, 85, 87, 86, -1, 89, -1, 90}
};

int8_t effectiveNote(uint8_t mux, uint8_t channel) {
    // Bounds checking
    if (mux >= N_MUX || channel >= N_CH) {
        return DISABLED;
    }
    
    // Get mapped note
    int8_t mappedNote = kNoteMap[mux][channel];
    
    // Check if key is disabled
    if (mappedNote == DISABLED) {
        return DISABLED;
    }
    
    // For now, return the note without transpose
    // TODO: Add pin-controlled transpose later
    // int16_t transposedNote = mappedNote + (transposePin ? gTranspose : 0);
    
    // Validate MIDI range [0..127]
    if (mappedNote < 0 || mappedNote > 127) {
        return DISABLED;
    }
    
    return mappedNote;
}

void printNoteMap() {
    Serial.println("=== Current Note Mapping ===");
    Serial.print("Global Transpose: ");
    Serial.print(gTranspose);
    Serial.println(" (not applied automatically - will be pin-controlled later)");
    Serial.println();
    
    for (uint8_t mux = 0; mux < N_MUX; mux++) {
        Serial.print("MUX");
        Serial.print(mux);
        Serial.print(": ");
        
        for (uint8_t ch = 0; ch < N_CH; ch++) {
            int8_t note = effectiveNote(mux, ch);
            if (note == DISABLED) {
                Serial.print(" -- ");
            } else {
                if (note < 100) Serial.print(" ");
                if (note < 10) Serial.print(" ");
                Serial.print(note);
            }
            if (ch < N_CH - 1) Serial.print(",");
        }
        Serial.println();
    }
    Serial.println("============================");
}