#include "simple_leds.h"
#include <Adafruit_NeoPixel.h>

// Pins (adapter si besoin)
static constexpr uint8_t PIN_SINGLE = 12;   // LED NeoPixel (SK6812 mini e) unique
static constexpr uint8_t PIN_STRIP  = 10;   // Strip NeoPixel
static constexpr uint8_t PIN_INPUT  = 3;    // Entrée à surveiller

// Strip de 5 LEDs sur PIN_STRIP; on utilisera l'index 1 (2ème LED) pour l'état du pin 3.
static Adafruit_NeoPixel strip(5, PIN_STRIP, NEO_GRB + NEO_KHZ800);

// LED simple sur pin 12 sera traitée en sortie numérique + couleur fixe verte par R/G/B discret (utiliser RGB discret ou NeoPixel ?)
// Supposons LED unique standard: ON = vert -> si LED RGB séparée faudrait lib; ici on simplifie: on allume HIGH.

void simpleLedsInit() {
    // Pin 3 surveillé (laisser au hardware externe le soin de tirer HIGH/LOW). Si flottant, mettre un pullup/pulldown adapté.
    pinMode(PIN_INPUT, INPUT);

        // NeoPixel unique sur pin 12 : on l'initialise séparément (format GRB)
        static Adafruit_NeoPixel single(1, PIN_SINGLE, NEO_GRB + NEO_KHZ800);
        single.begin();
        single.setPixelColor(0, single.Color(0, 50, 0)); // vert
        single.show();

    strip.begin();
    strip.clear();
    strip.show();
}

void simpleLedsTask() {
    int state = digitalRead(PIN_INPUT);
    // Nettoie uniquement la LED utilisée (évite de modifier les autres si tu veux les gérer ailleurs plus tard)
    if (state == HIGH) {
        strip.setPixelColor(1, strip.Color(0,0,80));
    } else {
        strip.setPixelColor(1, 0);
    }
    strip.show();
}

void setCalibrationLeds(bool enabled) {
    if (enabled) {
        // Turn on LEDs 2, 3, 4 in red (indices 2, 3, 4 - last 3 LEDs of the 5-LED strip)
        strip.setPixelColor(2, strip.Color(255, 0, 0)); // Red
        strip.setPixelColor(3, strip.Color(255, 0, 0)); // Red
        strip.setPixelColor(4, strip.Color(255, 0, 0)); // Red
    } else {
        // Turn off calibration LEDs
        strip.setPixelColor(2, 0);
        strip.setPixelColor(3, 0);
        strip.setPixelColor(4, 0);
    }
    strip.show();
}
