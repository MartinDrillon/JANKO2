#pragma once
// Configuration for JANKO2 8-MUX dual-ADC controller

#include <Arduino.h>

// === ADC Synchronized Read Order Selector ===
// 0 = startSynchronizedSingleRead(ADC0, ADC1)
// 1 = startSynchronizedSingleRead(ADC1, ADC0)
#ifndef ADC_SYNC_ORDER
#define ADC_SYNC_ORDER 0
#endif

// Temporary debug: print per synchronized pair results
#ifndef DEBUG_PRINT_PAIRS
#define DEBUG_PRINT_PAIRS 1
#endif

// === Hardware Configuration ===
// 8 MUX configuration with 2 ADC groups for true parallel scanning
//
// Wiring Recap (Source of Truth) — Teensy 4.1
// Group A (ADC1, MUX0..MUX3):
//   Select Lines (shared by MUX0..3): S0=35 (LSB), S1=34, S2=36, S3=37 (MSB)
//   COM Outputs → ADC1 inputs:
//     MUX0 COM → 41 (A22 / ADC1)
//     MUX1 COM → 40 (A21 / ADC1)
//     MUX2 COM → 39 (A20 / ADC1)
//     MUX3 COM → 38 (A19 / ADC1)
// Group B (ADC0, MUX4..MUX7):
//   Select Lines (shared by MUX4..7): S0=16 (LSB), S1=17, S2=15, S3=14 (MSB)
//   COM Outputs → ADC0 inputs:
//     MUX4 COM → 20 (A6  / ADC0)
//     MUX5 COM → 21 (A7  / ADC0)
//     MUX6 COM → 22 (A8  / ADC0)
//     MUX7 COM → 23 (A9  / ADC0)
// Channel index (0..15) encoded as (S3 S2 S1 S0) with S0 = LSB for BOTH groups.
// IMPORTANT: Use only MUX_ADC_PINS[] inside scanning (no PAIR* arrays) to avoid logic errors.
static constexpr int N_MUX = 8;        // Total number of multiplexers 
static constexpr int N_CH = 16;        // Channels per multiplexer
static constexpr uint16_t kTotalKeys = N_MUX * N_CH; // 128 keys total

// ADC pins for the 8 MUX outputs - optimized for Teensy 4.1 dual ADC
// Group A (ADC1): MUX 0-3 
// Group B (ADC0): MUX 4-7
static constexpr int MUX_ADC_PINS[N_MUX] = {
    // Group A - ADC1 (pins 38-41)
    41,  // MUX0 → pin 41 (ADC1_0 / A22)
    40,  // MUX1 → pin 40 (ADC1_1 / A21) 
    39,  // MUX2 → pin 39 (ADC1_2 / A20)
    38,  // MUX3 → pin 38 (ADC1_3 / A19)
    // Group B - ADC0 (pins 20-23)
    20,  // MUX4 → pin 20 (ADC0_6 / A6)
    21,  // MUX5 → pin 21 (ADC0_7 / A7)
    22,  // MUX6 → pin 22 (ADC0_8 / A8) 
    23   // MUX7 → pin 23 (ADC0_9 / A9)
};

// Synchronized ADC pairs for parallel reading (Target 2 optimization)
// (Legacy helper arrays kept only for potential diagnostics; NOT to be used in scanChannelDualADC)
static constexpr int PAIR_ADC1_PINS[4] = {41, 40, 39, 38}; // DO NOT USE in scanning logic
static constexpr int PAIR_ADC0_PINS[4] = {20, 21, 22, 23}; // DO NOT USE in scanning logic

// Group configuration for dual ADC scanning
static constexpr int N_GROUPS = 2;
static constexpr int MUX_PER_GROUP = 4;

// CD74HC4067 MUX control pins - separate for each group
// Group A (MUX 0-3) - S0-S3 lines
static constexpr uint8_t kMuxGroupA_S0 = 35; // S0 for group A
static constexpr uint8_t kMuxGroupA_S1 = 34; // S1 for group A  
static constexpr uint8_t kMuxGroupA_S2 = 36; // S2 for group A
static constexpr uint8_t kMuxGroupA_S3 = 37; // S3 for group A

// Group B (MUX 4-7) - S0-S3 lines  
static constexpr uint8_t kMuxGroupB_S0 = 16; // S0 for group B
static constexpr uint8_t kMuxGroupB_S1 = 17; // S1 for group B
static constexpr uint8_t kMuxGroupB_S2 = 15; // S2 for group B
static constexpr uint8_t kMuxGroupB_S3 = 14; // S3 for group B

// Safety check: ensure array size matches N_MUX
static_assert(N_MUX == sizeof(MUX_ADC_PINS)/sizeof(MUX_ADC_PINS[0]), "MUX_ADC_PINS array size must match N_MUX");

// ADC Configuration
static constexpr uint8_t kAdcResolution = 10; // 10-bit ADC (0-1023)

// MIDI Configuration
static constexpr uint8_t kMidiChannel = 1; // MIDI channel (1-16)

// Scanning Configuration - Target 2 synchronized pairs optimization
static constexpr uint32_t kScanIntervalMicros = 3;  // 3µs per channel (optimized for speed)
static constexpr uint32_t kSettleMicros = 1;         // 1µs settling for op-amp followers (reduced)
static constexpr uint32_t kFrameTargetHz = 3125;     // 3.125kHz frame rate target (optimized) 

// === Debug Options ===
// Freeze scanning to a fixed logical channel (0..15). Set to -1 for normal operation.
// Set to 6 to freeze scanning on logical channel 6 for debug logging
#ifndef DEBUG_FREEZE_CHANNEL
#define DEBUG_FREEZE_CHANNEL -1  // Reset to -1 to scan all channels
#endif

// Enable printing of per-channel values after each scan if non-zero.
// Enable verbose per-scan value printing
#ifndef DEBUG_PRINT_VALUES
#define DEBUG_PRINT_VALUES 1
#endif

