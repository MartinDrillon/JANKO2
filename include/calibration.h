#pragma once
#include <Arduino.h>

// === ADC Configuration ===
// Référence ADC : 10 bits = 0..1023, Vref = 3.3V

// === Seuils globaux ===
// ThresholdHigh = ADC(3.2V) - 30 → 3.2/3.3*1023 ≈ 993, donc ≈ 963
static constexpr uint16_t kThresholdHigh = 900;

// ThresholdLow = ADC(2.3V) + 30 → 2.3/3.3*1023 ≈ 713, donc ≈ 743  
static constexpr uint16_t kThresholdLow = 650;

// ThresholdRelease = ThresholdHigh - 50 → ≈ 913
static constexpr uint16_t kThresholdRelease = 850;

// === Hystérésis et filtres ===
// Nombre d'échantillons consécutifs pour valider un franchissement
static constexpr uint8_t kStableCount = 2;

// Delta ADC minimum pour ignorer micro-tremblements
static constexpr uint16_t kMinDeltaADC = 15;

// === Vélocité (mapping vers MIDI 1..127) ===
// Vitesse minimum en counts/s (sera mappée à vélocité MIDI 1)
static constexpr float kSpeedMin = 100.0f;

// Vitesse maximum en counts/s (sera mappée à vélocité MIDI 127)
static constexpr float kSpeedMax = 5000.0f;

// === Timing ===
// Timeout TRACKING maximum en microsecondes (100ms)
static constexpr uint32_t kTrackingTimeoutUs = 100000;

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
