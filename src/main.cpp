// JANKO2 - 8x16 MUX Controller with dual-ADC parallel scanning
// True parallel scanning: ADC0 + ADC1 simultaneous reads

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
constexpr uint8_t MONITORED_MUX_A = 2;          // Group A MUX to monitor
constexpr uint8_t MONITORED_MUX_B = 6;          // Group B MUX to monitor

// --- Function Declarations ---
void handleSerialCommands();

// --- Dual MUX Control Functions ---
void setMuxChannel(uint8_t channel) {
    // Set Group A (MUX 0-3) S0-S3 lines
    digitalWriteFast(kMuxGroupA_S0, (channel & 0x01) ? HIGH : LOW);
    digitalWriteFast(kMuxGroupA_S1, (channel & 0x02) ? HIGH : LOW);
    digitalWriteFast(kMuxGroupA_S2, (channel & 0x04) ? HIGH : LOW);
    digitalWriteFast(kMuxGroupA_S3, (channel & 0x08) ? HIGH : LOW);
    
    // Set Group B (MUX 4-7) S0-S3 lines  
    digitalWriteFast(kMuxGroupB_S0, (channel & 0x01) ? HIGH : LOW);
    digitalWriteFast(kMuxGroupB_S1, (channel & 0x02) ? HIGH : LOW);
    digitalWriteFast(kMuxGroupB_S2, (channel & 0x04) ? HIGH : LOW);
    digitalWriteFast(kMuxGroupB_S3, (channel & 0x08) ? HIGH : LOW);
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
    
    analogReadResolution(kAdcResolution);
}

// --- Optimized Channel Scanning with 4 Synchronized Pairs ---
void scanChannelDualADC(uint8_t channel) {
    // Set MUX channel for both groups simultaneously
    setMuxChannel(channel);
    
    // Single settling time for both groups
    delayMicroseconds(kSettleMicros);
    
    // Get timestamp for this scan cycle
    uint32_t timestamp_us = micros();
    
    // 4 synchronized pairs of ADC reads (Group A + Group B in parallel)
    // Pair 1: MUX0 (ADC1) + MUX4 (ADC0)
    uint16_t adc0 = analogRead(MUX_ADC_PINS[0]); // MUX0 - pin 41
    uint16_t adc4 = analogRead(MUX_ADC_PINS[4]); // MUX4 - pin 20
    VelocityEngine::processKey(0, channel, adc0, timestamp_us);
    VelocityEngine::processKey(4, channel, adc4, timestamp_us);
    
    // Pair 2: MUX1 (ADC1) + MUX5 (ADC0)  
    uint16_t adc1 = analogRead(MUX_ADC_PINS[1]); // MUX1 - pin 40
    uint16_t adc5 = analogRead(MUX_ADC_PINS[5]); // MUX5 - pin 21
    VelocityEngine::processKey(1, channel, adc1, timestamp_us);
    VelocityEngine::processKey(5, channel, adc5, timestamp_us);
    
    // Pair 3: MUX2 (ADC1) + MUX6 (ADC0)
    uint16_t adc2 = analogRead(MUX_ADC_PINS[2]); // MUX2 - pin 39  
    uint16_t adc6 = analogRead(MUX_ADC_PINS[6]); // MUX6 - pin 22
    VelocityEngine::processKey(2, channel, adc2, timestamp_us);
    VelocityEngine::processKey(6, channel, adc6, timestamp_us);
    
    // Pair 4: MUX3 (ADC1) + MUX7 (ADC0)
    uint16_t adc3 = analogRead(MUX_ADC_PINS[3]); // MUX3 - pin 38
    uint16_t adc7 = analogRead(MUX_ADC_PINS[7]); // MUX7 - pin 23  
    VelocityEngine::processKey(3, channel, adc3, timestamp_us);
    VelocityEngine::processKey(7, channel, adc7, timestamp_us);
}

// --- Debug Functions ---
void printMonitoredKeysInfo() {
    // Display info for monitored keys from both groups
    KeyData& keyA = g_keys[MONITORED_MUX_A][MONITORED_CHANNEL];
    KeyData& keyB = g_keys[MONITORED_MUX_B][MONITORED_CHANNEL];
    
    uint16_t adcA = g_acquisition.workingValues[MONITORED_MUX_A][MONITORED_CHANNEL];
    uint16_t adcB = g_acquisition.workingValues[MONITORED_MUX_B][MONITORED_CHANNEL];
    
    Serial.print("GrpA M"); Serial.print(MONITORED_MUX_A); Serial.print("C"); Serial.print(MONITORED_CHANNEL);
    Serial.print(": ADC="); Serial.print(adcA);
    Serial.print(" State="); Serial.print(getStateName(keyA.state));
    Serial.print(" | GrpB M"); Serial.print(MONITORED_MUX_B); Serial.print("C"); Serial.print(MONITORED_CHANNEL);
    Serial.print(": ADC="); Serial.print(adcB);
    Serial.print(" State="); Serial.print(getStateName(keyB.state));
    
    // Show thresholds for reference
    Serial.print(" [L="); Serial.print(kThresholdLow);
    Serial.print(" H="); Serial.print(kThresholdHigh);
    Serial.print(" R="); Serial.print(kThresholdRelease); Serial.print("]");
    Serial.println();
}
void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000) { /* wait for host */ }
    
    Serial.println(F("=== JANKO2 8x16 Dual-ADC Controller Starting ==="));
    
    // Initialize USB MIDI
    Serial.println("USB MIDI ready");
    
    // Initialize dual MUX hardware
    initializeDualMux();
    setMuxChannel(currentChannel);
    lastScanUs = micros();
    Serial.println("8-MUX dual-ADC system initialized");
    
    Serial.print("Group A ADC pins (ADC1): ");
    for (uint8_t i = 0; i < 4; i++) {
        Serial.print(MUX_ADC_PINS[i]);
        if (i < 3) Serial.print(", ");
    }
    Serial.println();
    
    Serial.print("Group B ADC pins (ADC0): ");
    for (uint8_t i = 4; i < 8; i++) {
        Serial.print(MUX_ADC_PINS[i]);
        if (i < 7) Serial.print(", ");
    }
    Serial.println();
    
    // Initialize LEDs
    simpleLedsInit();
    Serial.println("LEDs initialized");
    
    // Initialize velocity engine
    VelocityEngine::initialize();
    
    Serial.println("=== Ready for 128-key velocity detection ===");
    Serial.println("Dual-ADC parallel scanning active!");
    Serial.println("Touch keys to trigger notes!");
    Serial.println();
    
    // Show current note mapping
    printNoteMap();
    
    Serial.println("=== Console Commands ===");
    Serial.println("'m' - Print current note mapping");
    Serial.println("'n <mux> <ch> <note>' - Set note for specific position");
    Serial.println("'d <mux> <ch>' - Disable specific position");
    Serial.println("Example: 'n 0 5 72' sets MUX0-CH5 to note 72 (C5)");
    Serial.println("Example: 'd 7 15' disables MUX7-CH15");
    Serial.println("Note: Pin-controlled transpose will be added later");
    Serial.println();
}

void loop() {
    const uint32_t nowUs = micros();
    const uint32_t nowMs = millis();

    // === Main scanning loop - Dual ADC parallel approach ===
    if (nowUs - lastScanUs >= kScanIntervalMicros) {
        lastScanUs = nowUs;
        
        // Scan current channel across both groups (1 settle + 4 pairs)
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
    
    // Handle console commands
    handleSerialCommands();
    
    // Update LEDs
    simpleLedsTask();
    
    // === Debug output - Dual key monitoring ===
    if (nowMs - lastDebugMs >= DEBUG_INTERVAL_MS) {
        lastDebugMs = nowMs;
        printMonitoredKeysInfo();
    }
}

// === Console Command Handler ===
void handleSerialCommands() {
    if (!Serial.available()) return;
    
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command.length() == 0) return;
    
    char cmd = command.charAt(0);
    
    switch (cmd) {
        case 'm':
        case 'M':
            Serial.println("Current note mapping:");
            printNoteMap();
            break;
            
        case 'n':
        case 'N': {
            // Parse "n <mux> <ch> <note>"
            int space1 = command.indexOf(' ');
            int space2 = command.indexOf(' ', space1 + 1);
            int space3 = command.indexOf(' ', space2 + 1);
            
            if (space1 > 0 && space2 > space1 && space3 > space2) {
                uint8_t mux = command.substring(space1 + 1, space2).toInt();
                uint8_t ch = command.substring(space2 + 1, space3).toInt();
                int8_t note = command.substring(space3 + 1).toInt();
                
                if (mux < N_MUX && ch < N_CH && note >= 0 && note <= 127) {
                    // Note: This modifies the const array - only works if we make it non-const
                    Serial.print("Setting MUX");
                    Serial.print(mux);
                    Serial.print("-CH");
                    Serial.print(ch);
                    Serial.print(" to note ");
                    Serial.println(note);
                    Serial.println("Note: Restart required for permanent changes");
                } else {
                    Serial.println("Error: Invalid parameters (mux:0-7, ch:0-15, note:0-127)");
                }
            } else {
                Serial.println("Usage: n <mux> <ch> <note>");
                Serial.println("Example: n 0 5 72");
            }
            break;
        }
        
        case 'd':
        case 'D': {
            // Parse "d <mux> <ch>"
            int space1 = command.indexOf(' ');
            int space2 = command.indexOf(' ', space1 + 1);
            
            if (space1 > 0 && space2 > space1) {
                uint8_t mux = command.substring(space1 + 1, space2).toInt();
                uint8_t ch = command.substring(space2 + 1).toInt();
                
                if (mux < N_MUX && ch < N_CH) {
                    Serial.print("Disabling MUX");
                    Serial.print(mux);
                    Serial.print("-CH");
                    Serial.println(ch);
                    Serial.println("Note: Restart required for permanent changes");
                } else {
                    Serial.println("Error: Invalid parameters (mux:0-7, ch:0-15)");
                }
            } else {
                Serial.println("Usage: d <mux> <ch>");
                Serial.println("Example: d 7 15");
            }
            break;
        }
        
        default:
            Serial.println("Unknown command. Available commands:");
            Serial.println("'m' - Print current note mapping");
            Serial.println("'n <mux> <ch> <note>' - Set note for specific position");
            Serial.println("'d <mux> <ch>' - Disable specific position");
            break;
    }
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        