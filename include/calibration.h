#pragma once
#include <Arduino.h>

// === ADC Configuration ===
// Référence ADC : 10 bits = 0..1023, Vref = 3.3V

// === Seuils fixes simplifiés (plus de calibration dynamique) ===
static constexpr uint16_t kThresholdLow     = 745;  // activation / début suivi
static constexpr uint16_t kThresholdHigh    = 915;  // déclenchement Note On
static constexpr uint16_t kThresholdRelease = 900;  // retour Note Off

// Compat shim: certaines parties du code appellent encore getThresholdLow()
inline uint16_t getThresholdLow() { return kThresholdLow; }

