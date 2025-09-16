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
#include <imxrt.h>  // pour DWT cycle counter (Teensy 4.x)
#if DEBUG_ADC_MONITOR
#include "adc_monitor.h"
#endif

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
// Frame rate instrumentation
#if DEBUG_FRAME_RATE
static uint32_t gFrameCount = 0;
static uint32_t gLastFrameReportMs = 0;
#endif
// Profiling instrumentation
#if DEBUG_PROFILE_SCAN
static uint64_t gAccumChannelTimeUs = 0;   // somme durées individuelles channels
static uint32_t gChannelSamples = 0;       // nombre de channels mesurés
static uint32_t gFrameAccumTimeUs = 0;     // somme durées frames
static uint32_t gFrameSamples = 0;         // nombre de frames mesurées
static uint32_t gFrameStartUs = 0;         // début frame courante
static uint32_t gChannelMaxUs = 0;         // max channel observé sur intervalle
#endif
// Duplicate detection instrumentation
#if DEBUG_DUPLICATE_DETECT
static uint32_t gDuplicatePairs = 0;          // total paires détectées identiques
static uint32_t gDuplicateFrames = 0;         // frames contenant au moins 1 duplication
static uint32_t gFramesSinceDupPrint = 0;     // compteur de frames pour périodicité
#endif



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

// --- Cycle counter init (DWT) ---
static inline void enableCycleCounter() {
    // Teensy 4.x (IMXRT) expose ARM DWT registers via imxrt.h macros
    // ARM_DEMCR_TRCENA enables trace (needed for DWT cycle counter)
    ARM_DEMCR |= ARM_DEMCR_TRCENA;    // enable trace
    ARM_DWT_CYCCNT = 0;               // reset cycle counter
    ARM_DWT_CTRL |= 1;                // enable CYCCNT (bit 0)
}
static inline uint32_t readCycleCounter() { return ARM_DWT_CYCCNT; }

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
            // Pause optionnelle entre paires pour éviter des lectures clonées.
            if (kPerPairDelayMicros > 0) {
                delayMicroseconds(kPerPairDelayMicros);
            } else if (kPerPairDelayCycles > 0) {
                // Attente fine en cycles CPU
                uint32_t start = readCycleCounter();
                while ((readCycleCounter() - start) < kPerPairDelayCycles) { __asm__ volatile("nop"); }
            }
        }

    // Process all 8 keys (always active; no calibration phase)
    for (uint8_t mux = 0; mux < 8; mux++) {
        VelocityEngine::processKey(mux, channel, values[mux], timestamp_us);
#if DEBUG_ADC_MONITOR
        // Met à jour le moniteur si c'est la combinaison surveillée
        AdcMonitor::updateIfMatch(mux, channel, values[mux], timestamp_us);
#endif
    }

#if DEBUG_DUPLICATE_DETECT
    // Détection duplication: compare valeurs[0..3] vs valeurs[4..7]
    bool frameDup = false; // dans ce channel
    for (int i=0;i<4;i++) {
        int16_t d = (int16_t)values[i] - (int16_t)values[4+i];
        if (d < 0) d = -d;
        if (d <= (int16_t)kDuplicateTolerance) {
            gDuplicatePairs++;
            frameDup = true;
        }
    }
    if (frameDup) gDuplicateFrames++;
#endif

    // Optional debug print
    // Value debug removed
}


void setup() {
    // Serial interface removed
    
    // Initialize dual MUX hardware
    initializeDualMux();
    setMuxChannel(currentChannel);
    lastScanUs = micros();
#if DEBUG_PROFILE_SCAN
    gFrameStartUs = lastScanUs;
#endif
    
    // Initialize LEDs
    simpleLedsInit();
    
    // Calibration LEDs no longer used (ensure off)
    setCalibrationLeds(false);

#if DEBUG_ADC_MONITOR
    AdcMonitor::begin();
#endif

#if DEBUG_FRAME_RATE && DEBUG_FRAME_RATE_TOGGLE_LED
    pinMode(13, OUTPUT); // LED intégrée Teensy
    digitalWriteFast(13, LOW);
#endif
    
    // Initialize velocity engine
    VelocityEngine::initialize();
    enableCycleCounter();
    
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
    // Prépare éventuel changement LED (sans show())
    simpleLedsUpdateInputState();
    if (kContinuousScan) {
        // Pas d'attente: enchaîne directement
        uint32_t t0 = micros();
        scanChannelDualADC(currentChannel);
        uint32_t t1 = micros();
#if DEBUG_PROFILE_SCAN
        uint32_t chDur = t1 - t0;
        gAccumChannelTimeUs += chDur;
        gChannelSamples++;
        if (chDur > gChannelMaxUs) gChannelMaxUs = chDur;
#endif
        currentChannel = (currentChannel + 1) % N_CH;
        if (currentChannel == 0) {
            g_acquisition.swapBuffers();
#if DEBUG_PROFILE_SCAN
            uint32_t nowUsFrame = micros();
            uint32_t frameDur = nowUsFrame - gFrameStartUs;
            gFrameStartUs = nowUsFrame;
            gFrameAccumTimeUs += frameDur;
            gFrameSamples++;
            if (gFrameSamples >= DEBUG_PROFILE_INTERVAL_FRAMES) {
                // Calculs moyens
                uint32_t avgFrameUs = gFrameAccumTimeUs / gFrameSamples;
                uint32_t avgChannelUs = gChannelSamples ? (uint32_t)(gAccumChannelTimeUs / gChannelSamples) : 0;
                Serial.printf("[PROFILE] frames=%lu avgFrame=%luus avgCh=%luus maxCh=%luus fps_est=%lu\n",
                              (unsigned long)gFrameSamples,
                              (unsigned long)avgFrameUs,
                              (unsigned long)avgChannelUs,
                              (unsigned long)gChannelMaxUs,
                              (unsigned long)(avgFrameUs ? (1000000UL / avgFrameUs) : 0));
                // reset interval
                gFrameAccumTimeUs = 0;
                gFrameSamples = 0;
                gAccumChannelTimeUs = 0;
                gChannelSamples = 0;
                gChannelMaxUs = 0;
            }
#endif
#if DEBUG_FRAME_RATE
            gFrameCount++;
#if DEBUG_FRAME_RATE_TOGGLE_LED
            digitalToggleFast(13);
#endif
            uint32_t nowMs = millis();
            if (nowMs - gLastFrameReportMs >= DEBUG_FRAME_RATE_INTERVAL_MS) {
                // Impression minimaliste (peut être filtrée si besoin)
                Serial.printf("FrameRate=%lu fps\n", gFrameCount * 1000UL / (nowMs - gLastFrameReportMs));
                gLastFrameReportMs = nowMs;
                gFrameCount = 0;
            }
#endif
    // Flush LED strip une seule fois par frame (si changement)
    simpleLedsFrameFlush();
#if DEBUG_DUPLICATE_DETECT
    gFramesSinceDupPrint++;
    if (gFramesSinceDupPrint >= DEBUG_DUPLICATE_PRINT_INTERVAL_FRAMES) {
    Serial.printf("[DUP] frames=%lu dupFrames=%lu dupPairs=%lu ratioFrames=%.4f ratioPairs=%.4f\n",
        (unsigned long)gFramesSinceDupPrint,
        (unsigned long)gDuplicateFrames,
        (unsigned long)gDuplicatePairs,
        gFramesSinceDupPrint ? (double)gDuplicateFrames / (double)gFramesSinceDupPrint : 0.0,
    gFramesSinceDupPrint ? (double)gDuplicatePairs / (double)(gFramesSinceDupPrint*4) : 0.0);
        gFramesSinceDupPrint = 0;
        gDuplicateFrames = 0;
        gDuplicatePairs = 0;
    }
#endif
        }
    } else if (nowUs - lastScanUs >= kScanIntervalMicros) {
        lastScanUs = nowUs;
        uint32_t t0 = micros();
        scanChannelDualADC(currentChannel);
        uint32_t t1 = micros();
#if DEBUG_PROFILE_SCAN
        uint32_t chDur = t1 - t0;
        gAccumChannelTimeUs += chDur;
        gChannelSamples++;
        if (chDur > gChannelMaxUs) gChannelMaxUs = chDur;
#endif
        currentChannel = (currentChannel + 1) % N_CH;
        if (currentChannel == 0) {
            g_acquisition.swapBuffers();
#if DEBUG_PROFILE_SCAN
            uint32_t nowUsFrame = micros();
            uint32_t frameDur = nowUsFrame - gFrameStartUs;
            gFrameStartUs = nowUsFrame;
            gFrameAccumTimeUs += frameDur;
            gFrameSamples++;
            if (gFrameSamples >= DEBUG_PROFILE_INTERVAL_FRAMES) {
                uint32_t avgFrameUs = gFrameAccumTimeUs / gFrameSamples;
                uint32_t avgChannelUs = gChannelSamples ? (uint32_t)(gAccumChannelTimeUs / gChannelSamples) : 0;
                Serial.printf("[PROFILE] frames=%lu avgFrame=%luus avgCh=%luus maxCh=%luus fps_est=%lu\n",
                              (unsigned long)gFrameSamples,
                              (unsigned long)avgFrameUs,
                              (unsigned long)avgChannelUs,
                              (unsigned long)gChannelMaxUs,
                              (unsigned long)(avgFrameUs ? (1000000UL / avgFrameUs) : 0));
                gFrameAccumTimeUs = 0;
                gFrameSamples = 0;
                gAccumChannelTimeUs = 0;
                gChannelSamples = 0;
                gChannelMaxUs = 0;
            }
#endif
#if DEBUG_FRAME_RATE
            gFrameCount++;
#if DEBUG_FRAME_RATE_TOGGLE_LED
            digitalToggleFast(13);
#endif
            uint32_t nowMs = millis();
            if (nowMs - gLastFrameReportMs >= DEBUG_FRAME_RATE_INTERVAL_MS) {
                Serial.printf("FrameRate=%lu fps\n", gFrameCount * 1000UL / (nowMs - gLastFrameReportMs));
                gLastFrameReportMs = nowMs;
                gFrameCount = 0;
            }
#endif
    // Flush LED strip une seule fois par frame (si changement)
    simpleLedsFrameFlush();
#if DEBUG_DUPLICATE_DETECT
    gFramesSinceDupPrint++;
    if (gFramesSinceDupPrint >= DEBUG_DUPLICATE_PRINT_INTERVAL_FRAMES) {
    Serial.printf("[DUP] frames=%lu dupFrames=%lu dupPairs=%lu ratioFrames=%.4f ratioPairs=%.4f\n",
        (unsigned long)gFramesSinceDupPrint,
        (unsigned long)gDuplicateFrames,
        (unsigned long)gDuplicatePairs,
        gFramesSinceDupPrint ? (double)gDuplicateFrames / (double)gFramesSinceDupPrint : 0.0,
    gFramesSinceDupPrint ? (double)gDuplicatePairs / (double)(gFramesSinceDupPrint*4) : 0.0);
        gFramesSinceDupPrint = 0;
        gDuplicateFrames = 0;
        gDuplicatePairs = 0;
    }
#endif
        }
    }

    // === Handle other tasks ===
    // USB MIDI is handled automatically
    // (LEDs déjà flush en fin de frame si nécessaire)

#if DEBUG_ADC_MONITOR
    AdcMonitor::printPeriodic();
#endif
}

// (Calibration helper functions removed)


                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        