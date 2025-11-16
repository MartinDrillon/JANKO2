#pragma once
#include <Arduino.h>
#include "config.h"

// === MIDI Note Mapping for 8x16 MUX Configuration ===

//=============================================================================
// PHYSICAL LAYOUT CONFIGURATION (QMK-style)
//=============================================================================

/**
 * Hardware position encoding: MC(mux, channel)
 * Encodes (mux, channel) as single byte for compact storage
 */
#define MC(m, c) ((uint8_t)((m) << 4) | (c))
#define GET_MUX(mc) ((mc) >> 4)
#define GET_CH(mc) ((mc) & 0x0F)

/**
 * Key definition: MIDI note + hardware position
 */
struct KeyDef {
    int8_t note;    // MIDI note number (0-127, or -1 for disabled)
    uint8_t hw;     // Hardware position: MC(mux, channel)
};

/**
 * Physical keyboard layout (4 rows × 30 columns)
 * Edit this array to match your physical keyboard layout
 * Format: {note, MC(mux, channel)}
 * 
 * Example Janko layout (adjust to your actual wiring):
 */
constexpr KeyDef kPhysicalLayout[4][30] = {
    // ROW 4 - Top row
    // L1  L2  L3  L4  L5  L6  L7  L8  L9  L10 L11 L12 L13 L14 L15 L16 L17 L18 L19 L20 L21 L22 L23 L24 L25 L26 L27 L28 L29 L30
    // 28  30  32  34  36  38  40  42  44  46  48  50  52  54  56  58  60  62  64  66  68  70  72  74  76  78  80  82  84  86
    {
                        {28, MC(0,5)},  {30, MC(0,2)},  {32, MC(0,1)}, 
        {34, MC(1,6)},  {36, MC(1,5)},  {38, MC(1,2)},  {40, MC(1,1)}, 
        {42, MC(2,6)},  {44, MC(2,5)},  {46, MC(2,2)},  {48, MC(2,1)}, 
        {50, MC(3,6)},  {52, MC(3,5)},  {54, MC(3,2)},  {56, MC(3,1)}, 
        {58, MC(4,6)},  {60, MC(4,5)},  {62, MC(4,2)},  {64, MC(4,1)}, 
        {66, MC(5,6)},  {68, MC(5,5)},  {70, MC(5,2)},  {72, MC(5,1)}, 
        {74, MC(6,6)},  {76, MC(6,5)},  {78, MC(6,2)},  {80, MC(6,1)}, 
        {82, MC(7,6)},  {84, MC(7,5)},  {86, MC(7,2)}
    },
    
    // ROW 3
    // L1  L2  L3  L4  L5  L6  L7  L8  L9  L10 L11 L12 L13 L14 L15 L16 L17 L18 L19 L20 L21 L22 L23 L24 L25 L26 L27 L28 L29 L30
    // 29  31  33  35  37  39  41  43  45  47  49  51  53  55  57  59  61  63  65  67  69  71  73  75  77  79  81  83  85  87
    {
                        {29, MC(0,7)},  {31, MC(0,0)},  {33, MC(0,3)}, 
        {35, MC(1,4)},  {37, MC(1,7)},  {39, MC(1,0)},  {41, MC(1,3)}, 
        {43, MC(2,4)},  {45, MC(2,7)},  {47, MC(2,0)},  {49, MC(2,3)}, 
        {51, MC(3,4)},  {53, MC(3,7)},  {55, MC(3,0)},  {57, MC(3,3)}, 
        {59, MC(4,4)},  {61, MC(4,7)},  {63, MC(4,0)},  {65, MC(4,3)}, 
        {67, MC(5,4)},  {69, MC(5,7)},  {71, MC(5,0)},  {73, MC(5,3)}, 
        {75, MC(6,4)},  {77, MC(6,7)},  {79, MC(6,0)},  {81, MC(6,3)}, 
        {83, MC(7,4)},  {85, MC(7,7)},  {87, MC(7,0)}
    },
    
    // ROW 2
    // L1  L2  L3  L4  L5  L6  L7  L8  L9  L10 L11 L12 L13 L14 L15 L16 L17 L18 L19 L20 L21 L22 L23 L24 L25 L26 L27 L28 L29 L30
    // 28  30  32  34  36  38  40  42  44  46  48  50  52  54  56  58  60  62  64  66  68  70  72  74  76  78  80  82  84  86
    {
                         {28, MC(0,8)},  {30, MC(0,15)},  {32, MC(0,12)}, 
        {34, MC(1,11)},  {36, MC(1,8)},  {38, MC(1,15)},  {40, MC(1,12)}, 
        {42, MC(2,11)},  {44, MC(2,8)},  {46, MC(2,15)},  {48, MC(2,12)}, 
        {50, MC(3,11)},  {52, MC(3,8)},  {54, MC(3,15)},  {56, MC(3,12)}, 
        {58, MC(4,11)},  {60, MC(4,8)},  {62, MC(4,15)},  {64, MC(4,12)}, 
        {66, MC(5,11)},  {68, MC(5,8)},  {70, MC(5,15)},  {72, MC(5,12)}, 
        {74, MC(6,11)},  {76, MC(6,8)},  {78, MC(6,15)},  {80, MC(6,12)}, 
        {82, MC(7,11)},  {84, MC(7,8)},  {86, MC(7,15)}
    },
    
    // ROW 1 - Bottom row
    // L1  L2  L3  L4  L5  L6  L7  L8  L9  L10 L11 L12 L13 L14 L15 L16 L17 L18 L19 L20 L21 L22 L23 L24 L25 L26 L27 L28 L29 L30
    // 29  31  33  35  37  39  41  43  45  47  49  51  53  55  57  59  61  63  65  67  69  71  73  75  77  79  81  83  85  87
    {
                        {29, MC(0,10)},  {31, MC(0,13)},  {33, MC(0,14)}, 
        {35, MC(1,9)},  {37, MC(1,10)},  {39, MC(1,13)},  {41, MC(1,14)}, 
        {43, MC(2,9)},  {45, MC(2,10)},  {47, MC(2,13)},  {49, MC(2,14)}, 
        {51, MC(3,9)},  {53, MC(3,10)},  {55, MC(3,13)},  {57, MC(3,14)}, 
        {59, MC(4,9)},  {61, MC(4,10)},  {63, MC(4,13)},  {65, MC(4,14)}, 
        {67, MC(5,9)},  {69, MC(5,10)},  {71, MC(5,13)},  {73, MC(5,14)}, 
        {75, MC(6,9)},  {77, MC(6,10)},  {79, MC(6,13)},  {81, MC(6,14)}, 
        {83, MC(7,9)},  {85, MC(7,10)},  {87, MC(7,13)}
    }
};

/**
 * Compile-time conversion: physical layout → hardware lookup
 * Searches kPhysicalLayout to find the note for a given (mux, channel)
 * Returns -1 (DISABLED) if not found
 */
constexpr int8_t generateNoteMap(uint8_t mux, uint8_t channel) {
    for (uint8_t row = 0; row < 4; row++) {
        for (uint8_t col = 0; col < 30; col++) {
            const KeyDef& key = kPhysicalLayout[row][col];
            if (GET_MUX(key.hw) == mux && GET_CH(key.hw) == channel) {
                return key.note;
            }
        }
    }
    return -1; // DISABLED - not found in physical layout
}

//=============================================================================
// PUBLIC API
//=============================================================================

// Sentinel value for disabled keys
constexpr int8_t DISABLED = -1;

// Note mapping table [mux][channel] -> MIDI note (0-127) or DISABLED
// Generated from kPhysicalLayout at compile time
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
        "do", "do#", "re", "re#", "mi", "fa", "fa#", "sol", "sol#", "la", "la#", "si"
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


//	L1	L2	L3	L4	L5	L6	L7	L8	L9	L10	L11	L12	L13	L14	L15	L16	L17	L18	L19	L20	L21	L22	L23	L24	L25	L26	L27	L28	L29	L30
//R4	28	30	32	34	36	38	40	42	44	46	48	50	52	54	56	58	60	62	64	66	68	70	72	74	76	78	80	82	84	86
//R3	29	31	33	35	37	39	41	43	45	47	49	51	53	55	57	59	61	63	65	67	69	71	73	75	77	79	81	83	85	87
//R2	28	30	32	34	36	38	40	42	44	46	48	50	52	54	56	58	60	62	64	66	68	70	72	74	76	78	80	82	84	86
//R1	29	31	33	35	37	39	41	43	45	47	49	51	53	55	57	59	61	63	65	67	69	71	73	75	77	79	81	83	85	87