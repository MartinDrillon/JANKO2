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
    {    64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79},
    
    // MUX2 (Group A, ADC1): notes 80-95 (G#5 to B6)
    // CH:  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
    {    80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95},
    
    // MUX3 (Group A, ADC1): notes 96-111 (C7 to D#8)
    // CH:  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
    {    96, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111},
    
    // MUX4 (Group B, ADC0): notes 112-127 (E8 to G9)
    // CH:  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
    {   112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127},
    
    // MUX5 (Group B, ADC0): Example with some disabled keys
    // CH:  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
    {    48, 50, 52, 53, 55, 57, 59, 60, 62, 64, 65, 67, 69, 71, 72, 74},
    
    // MUX6 (Group B, ADC0): Example chromatic from C4
    // CH:  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
    {    60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75},
    
    // MUX7 (Group B, ADC0): Example with disabled keys at edges
    // CH:  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
    {    -1, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, -1}
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