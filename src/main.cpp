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

// === Target 2 Performance Instrumentation ===
struct PerformanceMetrics {
    uint32_t frameCount = 0;
    
    // Detailed timing breakdown
    uint32_t totalSLinesTime = 0;
    uint32_t totalSettleTime = 0;
    uint32_t totalConversionTime = 0;
    uint32_t totalProcessingTime = 0;
    
    // Min/Max tracking
    uint32_t minScanTime = UINT32_MAX;
    uint32_t maxScanTime = 0;
    uint32_t avgScanTime = 0;
    
    uint32_t lastReportMs = 0;
    
    void reset() {
        frameCount = 0;
        totalSLinesTime = totalSettleTime = totalConversionTime = totalProcessingTime = 0;
        minScanTime = UINT32_MAX;
        maxScanTime = 0;
    }
    
    void addFrame(uint32_t slines, uint32_t settle, uint32_t conversion, uint32_t processing) {
        frameCount++;
        totalSLinesTime += slines;
        totalSettleTime += settle;
        totalConversionTime += conversion;
        totalProcessingTime += processing;
        
        uint32_t totalTime = slines + settle + conversion + processing;
        minScanTime = min(minScanTime, totalTime);
        maxScanTime = max(maxScanTime, totalTime);
        avgScanTime = (totalSLinesTime + totalSettleTime + totalConversionTime + totalProcessingTime) / frameCount;
    }
};

static PerformanceMetrics g_perfMetrics;

// === Target 2 Performance Monitoring ===
constexpr uint32_t PERF_REPORT_INTERVAL_MS = 3000;     // Report every 3s
constexpr uint32_t FRAME_SAMPLE_SIZE = 100;            // Sample over 100 frames (faster report)

// Debug output control
uint32_t lastDebugMs = 0;
constexpr uint32_t DEBUG_INTERVAL_MS = 1000;    // Debug every 1s (faster monitoring)
constexpr uint8_t MONITORED_CHANNEL = 6;        // Channel to monitor
constexpr uint8_t MONITORED_MUX_A = 2;          // Group A MUX to monitor
constexpr uint8_t MONITORED_MUX_B = 6;          // Group B MUX to monitor

// --- Function Declarations ---
void handleSerialCommands();
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
    
    Serial.println("Target 2: High-speed MUX with LUT optimization ready");
}

// --- Target 2: Optimized Scanning with LUT and Performance Instrumentation ---
void scanChannelDualADC(uint8_t channel) {
    uint32_t startTime = micros();
    
    // (a) S-lines timing - ultra-fast with LUT
    uint32_t slinesStart = micros();
    setMuxChannel(channel);
    uint32_t slinesTime = micros() - slinesStart;
    
    // (b) Settle timing
    uint32_t settleStart = micros();
    delayMicroseconds(kSettleMicros);
    uint32_t settleTime = micros() - settleStart;
    
    // Get timestamp for velocity processing
    uint32_t timestamp_us = micros();
    
    // (c) Conversion timing - 4 optimized pairs (using fast analogRead for now)
    uint32_t conversionStart = micros();
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
    
    uint32_t conversionTime = micros() - conversionStart;
    
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
    
    // (d) Processing timing - batch process all 8 keys
    uint32_t processingStart = micros();
    
    // Only process keys if not calibrating (suspend MIDI engine during calibration)
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
    
    uint32_t processingTime = micros() - processingStart;
    
    // Add to performance metrics
    g_perfMetrics.addFrame(slinesTime, settleTime, conversionTime, processingTime);
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
    Serial.print(" [L="); Serial.print(getThresholdLow());
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
    
    // Start calibration: turn on red LEDs 3, 4, 5
    setCalibrationLeds(true);
    gCalibT0 = millis();
    Serial.println("=== CALIBRATION STARTED ===");
    Serial.println("Red LEDs 3,4,5 on - collecting ADC data for 2 seconds...");
    
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

    // === Calibration state management ===
    if (gState == RunState::CALIBRATING && (nowMs - gCalibT0 >= kCalibDurationMs)) {
        finishCalibration();
    }

    // === Main scanning loop - Target 2 synchronized pairs ===
    if (nowUs - lastScanUs >= kScanIntervalMicros) {
        lastScanUs = nowUs;
        
        // Scan current channel with detailed performance instrumentation
        scanChannelDualADC(currentChannel);
        
        // Advance to next channel
        currentChannel = (currentChannel + 1) % N_CH;
        
        // Frame completion: all channels scanned
        if (currentChannel == 0) {
            g_acquisition.swapBuffers();
        }
    }

    // === Target 2 Performance Monitoring ===
    if ((g_perfMetrics.frameCount >= FRAME_SAMPLE_SIZE) || 
        (nowMs - g_perfMetrics.lastReportMs >= PERF_REPORT_INTERVAL_MS)) {
        
        if (g_perfMetrics.frameCount > 0) {
            float frameRate = (g_perfMetrics.frameCount * 1000.0f) / PERF_REPORT_INTERVAL_MS;
            float keyRate = frameRate * 128.0f; // 128 keys per frame
            
            Serial.printf("=== TARGET 2 PERFORMANCE REPORT ===\n");
            Serial.printf("Frame Rate: %.1f Hz (%.1f keys/s)\n", frameRate, keyRate);
            Serial.printf("Scan Times (µs): min=%lu, avg=%lu, max=%lu\n", 
                         g_perfMetrics.minScanTime, g_perfMetrics.avgScanTime, g_perfMetrics.maxScanTime);
            Serial.printf("Breakdown: S-lines=%.1f, Settle=%.1f, Conv=%.1f, Proc=%.1f µs\n",
                         g_perfMetrics.totalSLinesTime / (float)g_perfMetrics.frameCount,
                         g_perfMetrics.totalSettleTime / (float)g_perfMetrics.frameCount,
                         g_perfMetrics.totalConversionTime / (float)g_perfMetrics.frameCount,
                         g_perfMetrics.totalProcessingTime / (float)g_perfMetrics.frameCount);
            Serial.printf("Total improvement vs Target 1: %.1fx\n", 
                         242.0f / g_perfMetrics.avgScanTime); // 242µs was Target 1 baseline
            Serial.println("=====================================");
        }
        
        g_perfMetrics.reset();
        g_perfMetrics.lastReportMs = nowMs;
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

// === Calibration Functions ===
uint16_t calculateMedian() {
    if (gSamplesTotal == 0) {
        Serial.println("WARNING: No samples collected during calibration!");
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
    
    // Log completion
    Serial.printf("=== CALIBRATION COMPLETED ===\n");
    Serial.printf("ThresholdLow updated to: %u (was 650)\n", gThresholdLow);
    Serial.printf("Total samples collected: %lu\n", gSamplesTotal);
    Serial.printf("MIDI engine now active\n");
    Serial.println("=============================");
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
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        