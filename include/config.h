#pragma once
// Minimal configuration constants (expand later as modules are added)

#include <Arduino.h>

static constexpr uint16_t kTotalKeys = 120;
static constexpr uint8_t  kMatrixRows = 10;  // placeholder
static constexpr uint8_t  kMatrixCols = 12;  // placeholder

// CD74HC4067 (16-channel analog/digital multiplexer) control pins
// User-specified mapping
static constexpr uint8_t kMuxPinS0 = 35; // S0
static constexpr uint8_t kMuxPinS1 = 34; // S1
static constexpr uint8_t kMuxPinS2 = 36; // S2
static constexpr uint8_t kMuxPinS3 = 37; // S3
// Optional enable (/EN) pin could be added later if needed (active LOW)
// static constexpr uint8_t kMuxEnablePin =  ;

// MIDI Configuration
static constexpr uint8_t kMidiChannel = 1; // MIDI channel (1-16)

// Debounce Configuration
static constexpr uint32_t kDebounceMs = 20; // Debounce time in milliseconds

// Scanning Configuration
static constexpr uint32_t kScanIntervalMicros = 1000; // Scan interval in microseconds 

