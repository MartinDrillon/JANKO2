#include "simple_leds.h"
#include <Adafruit_NeoPixel.h>

// Pins (adapter si besoin)
static constexpr uint8_t PIN_SINGLE = 12;   // LED NeoPixel (SK6812 mini e) unique
static constexpr uint8_t PIN_STRIP  = 10;   // Strip NeoPixel
static constexpr uint8_t PIN_INPUT  = 3;    // Entrée à surveiller

// Strip de 5 LEDs sur PIN_STRIP; on utilisera l'index 1 (2ème LED) pour l'état du pin 3.
static Adafruit_NeoPixel strip(5, PIN_STRIP, NEO_GRB + NEO_KHZ800);
static Adafruit_NeoPixel single(1, PIN_SINGLE, NEO_GRB + NEO_KHZ800);

// Cache de l'état désiré pour la LED d'index 1 sur le strip.
static uint32_t g_cachedColor = 0;      // couleur souhaitée actuelle
static uint32_t g_lastPushedColor = 0;  // dernière couleur réellement envoyée
static bool g_needFlush = false;        // un flush est nécessaire

// LED simple sur pin 12 sera traitée en sortie numérique + couleur fixe verte par R/G/B discret (utiliser RGB discret ou NeoPixel ?)
// Supposons LED unique standard: ON = vert -> si LED RGB séparée faudrait lib; ici on simplifie: on allume HIGH.

void simpleLedsInit() {
    pinMode(PIN_INPUT, INPUT);
    single.begin();
    single.setPixelColor(0, single.Color(0, 50, 0)); // vert fixe
    single.show();

    strip.begin();
    strip.clear();
    strip.show();
    g_cachedColor = 0;
    g_lastPushedColor = 0;
    g_needFlush = false;
}

void simpleLedsTask() {
    // Conservé pour compat rétro; appelle la logique lazy puis flush direct.
    simpleLedsUpdateInputState();
    simpleLedsFrameFlush();
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

// --- Nouveau mode performant ---
void simpleLedsUpdateInputState() {
    int state = digitalRead(PIN_INPUT);
    uint32_t wanted = (state == HIGH) ? strip.Color(0,0,80) : 0;
    if (wanted != g_cachedColor) {
        g_cachedColor = wanted;
        g_needFlush = true;
    }
}

void simpleLedsFrameFlush() {
    if (!g_needFlush && g_cachedColor == g_lastPushedColor) {
        return; // rien à faire
    }
    strip.setPixelColor(1, g_cachedColor);
    strip.show();
    g_lastPushedColor = g_cachedColor;
    g_needFlush = false;
}
