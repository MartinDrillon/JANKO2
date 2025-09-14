#pragma once
// Configuration for JANKO2 8-MUX dual-ADC controller

#include <Arduino.h>

// === Hardware Configuration ===
// 8 MUX configuration with 2 ADC groups for true parallel scanning
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
static constexpr int PAIR_ADC1_PINS[4] = {41, 40, 39, 38}; // Group A
static constexpr int PAIR_ADC0_PINS[4] = {20, 21, 22, 23}; // Group B

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

