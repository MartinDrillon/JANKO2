// JANKO2 - 8x16 MUX Controller with dual-ADC parallel scanning
// True parallel scanning: ADC0 + ADC1 simultaneous reads

#include <Arduino.h>
#include <ADC.h>
#include "config.h"
#include "simple_leds.h"
#include "velocity_engine.h"
#include "calibration.h"
#include "note_map.h"
#include "key_state.h"

// === ADC Instance ===
static ADC gAdc;

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

// === Calibration Removed ===
// Dynamic calibration logic has been fully removed in the simplified velocity design.
// Thresholds are now fixed constants defined in calibration.h. All related state,
// histograms, and timing have been stripped to reduce latency.

// --- Scanning State ---
uint8_t currentChannel = 0;
uint32_t lastScanUs = 0;



// --- Function Declarations ---
// (Calibration functions removed)

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
    
    // === ADC Library Configuration ===
    // Calibrate both ADCs
    gAdc.adc0->calibrate();
    gAdc.adc1->calibrate();
    
    // Set to no averaging for maximum speed
    gAdc.adc0->setAveraging(1);
    gAdc.adc1->setAveraging(1);
    
    // Set maximum conversion speed
    gAdc.adc0->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_HIGH_SPEED);
    gAdc.adc1->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_HIGH_SPEED);
    
    // Set maximum sampling speed
    gAdc.adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::VERY_HIGH_SPEED);
    gAdc.adc1->setSamplingSpeed(ADC_SAMPLING_SPEED::VERY_HIGH_SPEED);
    
    // Serial prints removed for performance
}

// --- Optimized Scanning with LUT and Synchronized ADC ---
void scanChannelDualADC(uint8_t channel) {
    // Optional debug channel freeze
    #if (DEBUG_FREEZE_CHANNEL >= 0)
    channel = DEBUG_FREEZE_CHANNEL % N_CH;
    #endif

    // Set MUX channel with ultra-fast LUT (both groups simultaneously)
    setMuxChannel(channel);

    // Allow MUX outputs + sample/hold buffers to settle
    delayMicroseconds(kSettleMicros);

    // Timestamp as close as possible to first acquisition (pre-loop)
    uint32_t timestamp_us = micros();

        // Perform four synchronized dual-ADC reads (must do all 4 to avoid cloned values)
        // EXACT SPECIFIED IMPLEMENTATION with selectable order and per-pair debug
        uint16_t values[8];
    for (int i = 0; i < 4; ++i) {
            #if ADC_SYNC_ORDER == 0
                // Ordre (ADC0, ADC1) — souvent attendu par pedvide sur T4.1
                const int pin_adc0 = MUX_ADC_PINS[4 + i];  // 20,21,22,23
                const int pin_adc1 = MUX_ADC_PINS[i];      // 41,40,39,38
                gAdc.startSynchronizedSingleRead(pin_adc0, pin_adc1);
                ADC::Sync_result r = gAdc.readSynchronizedSingle();
                values[i]     = r.result_adc1;  // MUX i     → ADC1 → pins 41,40,39,38
                values[4 + i] = r.result_adc0;  // MUX 4+i   → ADC0 → pins 20,21,22,23
            #else
                // Ordre (ADC1, ADC0)
                const int pin_adc1 = MUX_ADC_PINS[i];      // 41,40,39,38
                const int pin_adc0 = MUX_ADC_PINS[4 + i];  // 20,21,22,23
                gAdc.startSynchronizedSingleRead(pin_adc1, pin_adc0);
                ADC::Sync_result r = gAdc.readSynchronizedSingle();
                values[i]     = r.result_adc1;  // MUX i     → ADC1
                values[4 + i] = r.result_adc0;  // MUX 4+i   → ADC0
            #endif

            #if DEBUG_PRINT_PAIRS
                // Pair debug removed
            #endif
            // Micro pause to allow next pair stabilization (avoid cloned samples)
            delayMicroseconds(1);
        }

    // Process all 8 keys (always active; no calibration phase)
    for (uint8_t mux = 0; mux < 8; mux++) {
        VelocityEngine::processKey(mux, channel, values[mux], timestamp_us);
    }

    // Optional debug print
    // Value debug removed
}


void setup() {
    // Serial interface removed
    
    // Initialize dual MUX hardware
    initializeDualMux();
    setMuxChannel(currentChannel);
    lastScanUs = micros();
    
    // Initialize LEDs
    simpleLedsInit();
    
    // Calibration LEDs no longer used (ensure off)
    setCalibrationLeds(false);
    
    // Initialize velocity engine
    VelocityEngine::initialize();
    
    // Ready banner removed
    
        // === ADC Pin Mapping Sanity Check (dynamic) ===
        // Pin mapping sanity print removed
    
    // Show current note mapping
    // printNoteMap removed (Serial use)
}

void loop() {
    const uint32_t nowUs = micros();
    // (Calibration state management removed)

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

// (Calibration helper functions removed)


                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        