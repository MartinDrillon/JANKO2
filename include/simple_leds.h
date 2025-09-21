#pragma once
#include <Arduino.h>

// Initialise la LED simple (pin 12) et le strip (pin 10)
void simpleLedsInit();

// (Ancien) Met à jour immédiatement et envoie strip.show() (à éviter maintenant)
void simpleLedsTask();

// Nouveau mode performant :
// 1) simpleLedsUpdateInputState() lit l'entrée et met en cache la couleur voulue sans appeler show()
// 2) simpleLedsFrameFlush() en fin de frame pousse réellement les données si changement.
void simpleLedsUpdateInputState();
void simpleLedsFrameFlush();

// Calibration LED functions
void setCalibrationLeds(bool enabled);  // Turn red LEDs 3,4,5 on/off

// Optional: feed rocker status (so this module doesn't read pins)
void simpleLedsSetRocker(bool pin4High, bool pin5High);

// Optional: feed button24 state (LOW = pressed) to light LED 1 white
void simpleLedsSetButton24(bool isLowPressed);

// Calibration override modes for the strip (override all other indicators)
enum class LedOverride : uint8_t { None = 0, CalibBlinkRed, CalibBlueSolid };
void simpleLedsSetOverride(LedOverride mode);
