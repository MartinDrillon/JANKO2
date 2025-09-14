#pragma once
#include <Arduino.h>
#include "config.h"

// === ADC Configuration ===
// Référence ADC : 10 bits = 0..1023, Vref = 3.3V

// === Seuils globaux ===
// ThresholdHigh = ADC(3.2V) - 30 → 3.2/3.3*1023 ≈ 993, donc ≈ 963
static constexpr uint16_t kThresholdHigh = 880;

// ThresholdRelease = ThresholdHigh - 50 → ≈ 913
static constexpr uint16_t kThresholdRelease = 850;

// === Seuils par touche ===
// Calibration par touche - histogrammes pour médiane (256 Ko RAM)
extern uint16_t gHistLow[N_MUX][N_CH][1024];   // Histogrammes par touche
extern uint16_t gTotalLow[N_MUX][N_CH];        // Total échantillons par touche

// Seuils runtime par touche (updated by calibration)
extern uint16_t gThresholdLowPerKey[N_MUX][N_CH];

// Getters pour seuils par touche
inline uint16_t getLow(uint8_t mux, uint8_t channel) {
    if (mux >= N_MUX || channel >= N_CH) return 650; // fallback
    return gThresholdLowPerKey[mux][channel];
}

inline uint16_t getHigh(uint8_t mux, uint8_t channel) {
    (void)mux; (void)channel; // unused for now
    return kThresholdHigh;
}

inline uint16_t getRelease(uint8_t mux, uint8_t channel) {
    (void)mux; (void)channel; // unused for now  
    return kThresholdRelease;
}

// Setters pour calibration
inline void setLow(uint8_t mux, uint8_t channel, uint16_t value) {
    if (mux >= N_MUX || channel >= N_CH) return;
    // Clamp to valid ADC range
    gThresholdLowPerKey[mux][channel] = (value > 1023) ? 1023 : value;
}

// Legacy getter for compatibility (deprecated)
inline uint16_t getThresholdLow() {
    return getLow(0, 0); // fallback to first key
}

// === Fonctions de calibration ===
void initializeCalibration();
void clearCalibrationData();
uint16_t calculateMedianForKey(uint8_t mux, uint8_t channel);

// === Hystérésis et filtres ===
// Nombre d'échantillons consécutifs pour valider un franchissement
static constexpr uint8_t kStableCount = 1;

// Delta ADC minimum pour ignorer micro-tremblements
static constexpr uint16_t kMinDeltaADC = 5;

// === Vélocité (mapping vers MIDI 1..127) ===
// Vitesse minimum en counts/s (sera mappée à vélocité MIDI 1)
static constexpr float kSpeedMin = 300.0f;

// Vitesse maximum en counts/s (sera mappée à vélocité MIDI 127)
static constexpr float kSpeedMax = 30000.0f;

// === Timing ===
// Timeout TRACKING maximum en microsecondes (1000ms)
static constexpr uint32_t kTrackingTimeoutUs = 1000000;

// === Fonctions utilitaires ===
inline uint8_t calculateVelocity(float speed) {
    // Mapping linéaire speed → [1..127]
    if (speed <= kSpeedMin) return 1;
    if (speed >= kSpeedMax) return 127;
    
    float normalized = (speed - kSpeedMin) / (kSpeedMax - kSpeedMin);
    return 1 + static_cast<uint8_t>(normalized * 126.0f);
}

inline bool isValidDelta(uint16_t delta_adc) {
    return delta_adc >= kMinDeltaADC;
}
static constexpr uint16_t kTargetSampleRateHz = 1000; // per key
