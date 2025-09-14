#pragma once
// Configuration constants for JANKO2 4-MUX controller

#include <Arduino.h>

// === Hardware Configuration ===
// 4 MUX configuration
static constexpr int N_MUX = 4;     // Number of multiplexers 
static constexpr int N_CH = 16;     // Channels per multiplexer
static constexpr uint16_t kTotalKeys = N_MUX * N_CH; // 64 keys total

// ADC pins for the 4 MUX outputs (Teensy 4.1)
static constexpr int ADC_PINS[N_MUX] = {
    41,  // MUX1 → pin 41 (A22)
    40,  // MUX2 → pin 40 (A21) 
    39,  // MUX3 → pin 39 (A20)
    38   // MUX4 → pin 38 (A19)
};
// Safety check: ensure array size matches N_MUX
static_assert(N_MUX == sizeof(ADC_PINS)/sizeof(ADC_PINS[0]), "ADC_PINS array size must match N_MUX");

// CD74HC4067 (16-channel analog/digital multiplexer) control pins
// Shared S0-S3 lines for all 4 MUX chips
static constexpr uint8_t kMuxPinS0 = 35; // S0
static constexpr uint8_t kMuxPinS1 = 34; // S1
static constexpr uint8_t kMuxPinS2 = 36; // S2
static constexpr uint8_t kMuxPinS3 = 37; // S3
// Optional enable (/EN) pin could be added later if needed (active LOW)
// static constexpr uint8_t kMuxEnablePin =  ;

// ADC Configuration
static constexpr uint8_t kAdcResolution = 10; // 10-bit ADC (0-1023)

// MIDI Configuration
static constexpr uint8_t kMidiChannel = 1; // MIDI channel (1-16)

// Scanning Configuration
static constexpr uint32_t kScanIntervalMicros = 100; // Fast scan: 100µs per channel
static constexpr uint32_t kSettleMicros = 10;       // MUX settling time 

