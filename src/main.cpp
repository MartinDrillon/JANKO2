// JANKO2 - 4x16 MUX Controller with velocity detection
// Canal-major scanning: 1 settle + 4 ADC reads per channel

#include <Arduino.h>
#include "config.h"
#include "simple_leds.h"
#include "velocity_engine.h"
#include "calibration.h"
#include "note_map.h"
#include "key_state.h"

// === USB MIDI is built-in on Teensy ===
// No additional setup required

// --- Scanning State ---
uint8_t currentChannel = 0;
uint32_t lastScanUs = 0;

// Debug output control
uint32_t lastDebugMs = 0;
constexpr uint32_t DEBUG_INTERVAL_MS = 100;     // Debug every 100ms
constexpr uint8_t MONITORED_CHANNEL = 6;        // Channel to monitor
constexpr uint8_t MONITORED_MUX = 2;            // MUX to monitor

// --- MUX Control Functions ---
void muxSelect(uint8_t channel) {
    digitalWriteFast(kMuxPinS0, (channel & 0x01) ? HIGH : LOW);
    digitalWriteFast(kMuxPinS1, (channel & 0x02) ? HIGH : LOW);
    digitalWriteFast(kMuxPinS2, (channel & 0x04) ? HIGH : LOW);
    digitalWriteFast(kMuxPinS3, (channel & 0x08) ? HIGH : LOW);
}

void initializeMux() {
    pinMode(kMuxPinS0, OUTPUT);
    pinMode(kMuxPinS1, OUTPUT);
    pinMode(kMuxPinS2, OUTPUT);
    pinMode(kMuxPinS3, OUTPUT);
    analogReadResolution(kAdcResolution);
}

// --- Channel Scanning Function ---
void scanChannelAllMux(uint8_t channel) {
    // Set MUX channel for all 4 MUX chips (shared S0-S3 lines)
    muxSelect(channel);
    
    // Wait for MUX settling
    delayMicroseconds(kSettleMicros);
    
    // Get timestamp for this scan cycle
    uint32_t timestamp_us = micros();
    
    // Read all 4 ADC inputs sequentially (quasi-simultaneous)
    for (uint8_t mux = 0; mux < N_MUX; mux++) {
        uint16_t adc_value = analogRead(ADC_PINS[mux]);
        
        // IMMEDIATE processing - don't wait for frame completion
        VelocityEngine::processKey(mux, channel, adc_value, timestamp_us);
    }
}

// --- Debug Functions ---
void printMonitoredKeyInfo() {
    // Display info for monitored key
    KeyData& key = g_keys[MONITORED_MUX][MONITORED_CHANNEL];
    uint16_t adc = g_acquisition.workingValues[MONITORED_MUX][MONITORED_CHANNEL];
    
    Serial.print("M"); Serial.print(MONITORED_MUX);
    Serial.print("C"); Serial.print(MONITORED_CHANNEL);
    Serial.print(": ADC="); Serial.print(adc);
    Serial.print(" State="); Serial.print(getStateName(key.state));
    Serial.print(" Triggers="); Serial.print(key.total_triggers);
    Serial.print(" Note="); Serial.print(key.current_note);
    
    // Show thresholds for reference
    Serial.print(" [L="); Serial.print(kThresholdLow);
    Serial.print(" H="); Serial.print(kThresholdHigh);
    Serial.print(" R="); Serial.print(kThresholdRelease); Serial.print("]");
    Serial.println();
}
void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000) { /* wait for host */ }
    
    Serial.println(F("=== JANKO2 4x16 MUX Controller Starting ==="));
    
    // Initialize USB MIDI
    Serial.println("USB MIDI ready");
    
    // Initialize MUX hardware
    initializeMux();
    muxSelect(currentChannel);
    lastScanUs = micros();
    Serial.println("4-MUX system initialized");
    Serial.print("ADC pins: ");
    for (uint8_t i = 0; i < N_MUX; i++) {
        Serial.print(ADC_PINS[i]);
        if (i < N_MUX - 1) Serial.print(", ");
    }
    Serial.println();
    
    // Initialize LEDs
    simpleLedsInit();
    Serial.println("LEDs initialized");
    
    // Initialize velocity engine
    VelocityEngine::initialize();
    
    Serial.println("=== Ready for 64-key velocity detection ===");
    Serial.println("Touch keys to trigger notes!");
    Serial.println();
}

void loop() {
    const uint32_t nowUs = micros();
    const uint32_t nowMs = millis();

    // === Main scanning loop - Canal-major approach ===
    if (nowUs - lastScanUs >= kScanIntervalMicros) {
        lastScanUs = nowUs;
        
        // Scan current channel across all 4 MUX (1 settle + 4 reads)
        scanChannelAllMux(currentChannel);
        
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
    
    // === Debug output - Single key monitoring ===
    if (nowMs - lastDebugMs >= DEBUG_INTERVAL_MS) {
        lastDebugMs = nowMs;
        printMonitoredKeyInfo();
    }
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        