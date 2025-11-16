#include "note_map.h"

// Global transpose value (default: no transpose)
int8_t gTranspose = 0;

static inline int8_t clampTranspose(int8_t s) {
    if (s < -24) return -24;
    if (s >  24) return  24;
    return s;
}

void noteMapSetTranspose(int8_t semitones) {
    gTranspose = clampTranspose(semitones);
}

/**
 * Note mapping table [mux][channel] -> MIDI note
 * Generated from kPhysicalLayout (defined in note_map.h) at compile time
 * Uses constexpr to compute all values during compilation (zero runtime cost)
 */
const int8_t kNoteMap[8][16] = {
    // MUX 0
    {
        generateNoteMap(0,0), generateNoteMap(0,1), generateNoteMap(0,2), generateNoteMap(0,3),
        generateNoteMap(0,4), generateNoteMap(0,5), generateNoteMap(0,6), generateNoteMap(0,7),
        generateNoteMap(0,8), generateNoteMap(0,9), generateNoteMap(0,10), generateNoteMap(0,11),
        generateNoteMap(0,12), generateNoteMap(0,13), generateNoteMap(0,14), generateNoteMap(0,15)
    },
    // MUX 1
    {
        generateNoteMap(1,0), generateNoteMap(1,1), generateNoteMap(1,2), generateNoteMap(1,3),
        generateNoteMap(1,4), generateNoteMap(1,5), generateNoteMap(1,6), generateNoteMap(1,7),
        generateNoteMap(1,8), generateNoteMap(1,9), generateNoteMap(1,10), generateNoteMap(1,11),
        generateNoteMap(1,12), generateNoteMap(1,13), generateNoteMap(1,14), generateNoteMap(1,15)
    },
    // MUX 2
    {
        generateNoteMap(2,0), generateNoteMap(2,1), generateNoteMap(2,2), generateNoteMap(2,3),
        generateNoteMap(2,4), generateNoteMap(2,5), generateNoteMap(2,6), generateNoteMap(2,7),
        generateNoteMap(2,8), generateNoteMap(2,9), generateNoteMap(2,10), generateNoteMap(2,11),
        generateNoteMap(2,12), generateNoteMap(2,13), generateNoteMap(2,14), generateNoteMap(2,15)
    },
    // MUX 3
    {
        generateNoteMap(3,0), generateNoteMap(3,1), generateNoteMap(3,2), generateNoteMap(3,3),
        generateNoteMap(3,4), generateNoteMap(3,5), generateNoteMap(3,6), generateNoteMap(3,7),
        generateNoteMap(3,8), generateNoteMap(3,9), generateNoteMap(3,10), generateNoteMap(3,11),
        generateNoteMap(3,12), generateNoteMap(3,13), generateNoteMap(3,14), generateNoteMap(3,15)
    },
    // MUX 4
    {
        generateNoteMap(4,0), generateNoteMap(4,1), generateNoteMap(4,2), generateNoteMap(4,3),
        generateNoteMap(4,4), generateNoteMap(4,5), generateNoteMap(4,6), generateNoteMap(4,7),
        generateNoteMap(4,8), generateNoteMap(4,9), generateNoteMap(4,10), generateNoteMap(4,11),
        generateNoteMap(4,12), generateNoteMap(4,13), generateNoteMap(4,14), generateNoteMap(4,15)
    },
    // MUX 5
    {
        generateNoteMap(5,0), generateNoteMap(5,1), generateNoteMap(5,2), generateNoteMap(5,3),
        generateNoteMap(5,4), generateNoteMap(5,5), generateNoteMap(5,6), generateNoteMap(5,7),
        generateNoteMap(5,8), generateNoteMap(5,9), generateNoteMap(5,10), generateNoteMap(5,11),
        generateNoteMap(5,12), generateNoteMap(5,13), generateNoteMap(5,14), generateNoteMap(5,15)
    },
    // MUX 6
    {
        generateNoteMap(6,0), generateNoteMap(6,1), generateNoteMap(6,2), generateNoteMap(6,3),
        generateNoteMap(6,4), generateNoteMap(6,5), generateNoteMap(6,6), generateNoteMap(6,7),
        generateNoteMap(6,8), generateNoteMap(6,9), generateNoteMap(6,10), generateNoteMap(6,11),
        generateNoteMap(6,12), generateNoteMap(6,13), generateNoteMap(6,14), generateNoteMap(6,15)
    },
    // MUX 7
    {
        generateNoteMap(7,0), generateNoteMap(7,1), generateNoteMap(7,2), generateNoteMap(7,3),
        generateNoteMap(7,4), generateNoteMap(7,5), generateNoteMap(7,6), generateNoteMap(7,7),
        generateNoteMap(7,8), generateNoteMap(7,9), generateNoteMap(7,10), generateNoteMap(7,11),
        generateNoteMap(7,12), generateNoteMap(7,13), generateNoteMap(7,14), generateNoteMap(7,15)
    }
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
    // Apply current global transpose (updated by IoState via noteMapSetTranspose)
    int16_t transposed = (int16_t)mappedNote + (int16_t)gTranspose;
    if (transposed < 0 || transposed > 127) {
        return DISABLED;
    }
    return (int8_t)transposed;
}

void printNoteMap() {
    // Mapping print removed (no Serial in performance build)
}

// Pin polling moved to IoState