// JANKO2 - 8x16 MUX Controller with dual-ADC parallel scanning
// True parallel scanning: ADC0 + ADC1 simultaneous reads

#include <Arduino.h>
#include "config.h"
#include "simple_leds.h"
#include "velocity_engine.h"
#include "calibration.h"
#include "note_map.h"
#include "key_state.h"

// === Fast MUX Channel Switching LUT ===
// Pre-calculated GPIO states for each channel (0-15)
struct ChannelGPIO {
    uint8_t groupA_S0, groupA_S1, groupA_S2, groupA_S3;
    uint8_t groupB_S0, groupB_S1, groupB_S2, groupB_S3;
};

// LUT for fast channel switching (avoids bit operations in loop)
static constexpr ChannelGPIO CHANNEL_LUT[16] = {
    // Ch0: 0000
    {LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW},
    // Ch1: 0001  
    {HIGH, LOW, LOW, LOW, HIGH, LOW, LOW, LOW},
    // Ch2: 0010
    {LOW, HIGH, LOW, LOW, LOW, HIGH, LOW, LOW},
    // Ch3: 0011
    {HIGH, HIGH, LOW, LOW, HIGH, HIGH, LOW, LOW},
    // Ch4: 0100
    {LOW, LOW, HIGH, LOW, LOW, LOW, HIGH, LOW},
    // Ch5: 0101
    {HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW},
    // Ch6: 0110
    {LOW, HIGH, HIGH, LOW, LOW, HIGH, HIGH, LOW},
    // Ch7: 0111
    {HIGH, HIGH, HIGH, LOW, HIGH, HIGH, HIGH, LOW},
    // Ch8: 1000
    {LOW, LOW, LOW, HIGH, LOW, LOW, LOW, HIGH},
    // Ch9: 1001
    {HIGH, LOW, LOW, HIGH, HIGH, LOW, LOW, HIGH},
    // Ch10: 1010
    {LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH},
    // Ch11: 1011
    {HIGH, HIGH, LOW, HIGH, HIGH, HIGH, LOW, HIGH},
    // Ch12: 1100
    {LOW, LOW, HIGH, HIGH, LOW, LOW, HIGH, HIGH},
    // Ch13: 1101
    {HIGH, LOW, HIGH, HIGH, HIGH, LOW, HIGH, HIGH},
    // Ch14: 1110
    {LOW, HIGH, HIGH, HIGH, LOW, HIGH, HIGH, HIGH},
    // Ch15: 1111
    {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH}
};

// === USB MIDI is built-in on Teensy ===
// No additional setup required

// === Calibration State ===
enum class RunState { CALIBRATING, RUN };
static RunState gState = RunState::CALIBRATING;
constexpr uint32_t kCalibDurationMs = 2000;
static uint32_t gCalibT0 = 0;

// Calibration histogram for median calculation (10-bit ADC = 1024 values)
static uint32_t gHist[1024] = {0};
static uint32_t gSamplesTotal = 0;

// --- Scanning State ---
uint8_t currentChannel = 0;
uint32_t lastScanUs = 0;



// --- Function Declarations ---
uint16_t calculateMedian();
void finishCalibration();

// --- Dual MUX Control Functions ---
// --- Ultra-Fast MUX Channel Setting with LUT ---
void setMuxChannel(uint8_t channel) {
    const ChannelGPIO& gpio = CHANNEL_LUT[channel];
    
    // Set Group A (S0-S3) with pre-calculated values
    digitalWriteFast(kMuxGroupA_S0, gpio.groupA_S0);
    digitalWriteFast(kMuxGroupA_S1, gpio.groupA_S1);
    digitalWriteFast(kMuxGroupA_S2, gpio.groupA_S2);
    digitalWriteFast(kMuxGroupA_S3, gpio.groupA_S3);
    
    // Set Group B (S0-S3) with pre-calculated values
    digitalWriteFast(kMuxGroupB_S0, gpio.groupB_S0);
    digitalWriteFast(kMuxGroupB_S1, gpio.groupB_S1);
    digitalWriteFast(kMuxGroupB_S2, gpio.groupB_S2);
    digitalWriteFast(kMuxGroupB_S3, gpio.groupB_S3);
}

void initializeDualMux() {
    // Initialize Group A control pins
    pinMode(kMuxGroupA_S0, OUTPUT);
    pinMode(kMuxGroupA_S1, OUTPUT);
    pinMode(kMuxGroupA_S2, OUTPUT);
    pinMode(kMuxGroupA_S3, OUTPUT);
    
    // Initialize Group B control pins
    pinMode(kMuxGroupB_S0, OUTPUT);
    pinMode(kMuxGroupB_S1, OUTPUT);
    pinMode(kMuxGroupB_S2, OUTPUT);
    pinMode(kMuxGroupB_S3, OUTPUT);
    
    // Standard ADC configuration for now (we'll optimize with ADC lib later)
    analogReadResolution(kAdcResolution);
}

// --- Optimized Scanning with LUT ---
void scanChannelDualADC(uint8_t channel) {
    // Set MUX channel with ultra-fast LUT
    setMuxChannel(channel);
    
    // Allow MUX to settle
    delayMicroseconds(kSettleMicros);
    
    // Get timestamp for velocity processing
    uint32_t timestamp_us = micros();
    
    // Read 4 optimized pairs (using fast analogRead)
    uint16_t values[8];
    
    // Pair 1: MUX0 (pin 41) + MUX4 (pin 20)
    values[0] = analogRead(PAIR_ADC1_PINS[0]); // MUX0 
    values[4] = analogRead(PAIR_ADC0_PINS[0]); // MUX4
    
    // Pair 2: MUX1 (pin 40) + MUX5 (pin 21)
    values[1] = analogRead(PAIR_ADC1_PINS[1]); // MUX1
    values[5] = analogRead(PAIR_ADC0_PINS[1]); // MUX5
    
    // Pair 3: MUX2 (pin 39) + MUX6 (pin 22)
    values[2] = analogRead(PAIR_ADC1_PINS[2]); // MUX2
    values[6] = analogRead(PAIR_ADC0_PINS[2]); // MUX6
    
    // Pair 4: MUX3 (pin 38) + MUX7 (pin 23)
    values[3] = analogRead(PAIR_ADC1_PINS[3]); // MUX3
    values[7] = analogRead(PAIR_ADC0_PINS[3]); // MUX7
    
    // === Calibration data collection ===
    if (gState == RunState::CALIBRATING) {
        // Collect all ADC values in histogram for median calculation
        for (uint8_t mux = 0; mux < 8; mux++) {
            uint16_t val = values[mux];
            // Clamp to 10-bit range (safety check)
            if (val > 1023) val = 1023;
            gHist[val]++;
            gSamplesTotal++;
        }
    }
    
    // Process all 8 keys
    if (gState == RunState::RUN) {
        for (uint8_t mux = 0; mux < 8; mux++) {
            VelocityEngine::processKey(mux, channel, values[mux], timestamp_us);
        }
    } else {
        // During calibration, still update working values for continuity but no MIDI processing
        for (uint8_t mux = 0; mux < 8; mux++) {
            g_acquisition.workingValues[mux][channel] = values[mux];
        }
    }
}


void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000) { /* wait for host */ }
    
    Serial.println(F("=== JANKO2 8x16 Dual-ADC Controller Starting ==="));
    
    // Initialize dual MUX hardware
    initializeDualMux();
    setMuxChannel(currentChannel);
    lastScanUs = micros();
    
    // Initialize LEDs
    simpleLedsInit();
    
    // Start calibration: turn on red LEDs 3, 4, 5
    setCalibrationLeds(true);
    gCalibT0 = millis();
    Serial.println("=== CALIBRATION STARTED ===");
    
    // Initialize velocity engine
    VelocityEngine::initialize();
    
    Serial.println("=== Ready for 128-key velocity detection ===");
    
    // Show current note mapping
    printNoteMap();
}

void loop() {
    const uint32_t nowUs = micros();
    const uint32_t nowMs = millis();

    // === Calibration state management ===
    if (gState == RunState::CALIBRATING && (nowMs - gCalibT0 >= kCalibDurationMs)) {
        finishCalibration();
    }

    // === Main scanning loop ===
    if (nowUs - lastScanUs >= kScanIntervalMicros) {
        lastScanUs = nowUs;
        
        // Scan current channel
        scanChannelDualADC(currentChannel);
        
        // Advance to next channel
        currentChannel = (currentChannel + 1) % N_CH;
        
        // Frame completion: all channels scanned
        if (currentChannel == 0) {
            g_acquisition.swapBuffers();
        }
    }

    // === Handle other tasks ===
    // USB MIDI is handled automatically
    
    // Update LEDs
    simpleLedsTask();
}

// === Calibration Functions ===
uint16_t calculateMedian() {
    if (gSamplesTotal == 0) {
        return 650; // Return default value
    }
    
    uint32_t target = gSamplesTotal / 2;
    uint32_t cumSum = 0;
    
    for (uint16_t i = 0; i < 1024; i++) {
        cumSum += gHist[i];
        if (cumSum >= target) {
            return i;
        }
    }
    
    // Fallback (should not happen)
    return 650;
}

void finishCalibration() {
    // Calculate median from histogram
    uint16_t median = calculateMedian();
    
    // Update runtime threshold
    gThresholdLow = median;
    
    // Turn off calibration LEDs
    setCalibrationLeds(false);
    
    // Change state to RUN
    gState = RunState::RUN;
}


                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        