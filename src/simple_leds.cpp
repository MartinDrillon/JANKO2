#include "simple_leds.h"
#include <Adafruit_NeoPixel.h>

// Pins (adapter si besoin)
static constexpr uint8_t PIN_SINGLE = 12;   // LED NeoPixel (SK6812 mini e) unique
static constexpr uint8_t PIN_STRIP  = 10;   // Strip NeoPixel
static constexpr uint8_t PIN_INPUT  = 3;    // Entrée à surveiller
// Rocker switch pins (with INPUT_PULLUP)
static constexpr uint8_t PIN_ROCKER_4 = 4;  // Rocker contact 1
static constexpr uint8_t PIN_ROCKER_5 = 5;  // Rocker contact 2

// Strip de 5 LEDs sur PIN_STRIP; on utilisera l'index 1 (2ème LED) pour l'état du pin 3.
static Adafruit_NeoPixel strip(5, PIN_STRIP, NEO_GRB + NEO_KHZ800);
static Adafruit_NeoPixel single(1, PIN_SINGLE, NEO_GRB + NEO_KHZ800);

// Cache de l'état désiré pour la LED d'index 1 sur le strip.
static uint32_t g_cachedColor = 0;      // couleur souhaitée actuelle (LED index 1)
static uint32_t g_lastPushedColor = 0;  // dernière couleur réellement envoyée (LED index 1)
// Rocker cached state: 0=off, 1=LEFT(LED idx 2), 2=CENTER(LED idx 3), 3=RIGHT(LED idx 4)
static uint8_t g_cachedRockerState = 0;
static uint8_t g_lastRockerState = 0;
static bool g_needFlush = false;        // un flush est nécessaire

// LED simple sur pin 12 sera traitée en sortie numérique + couleur fixe verte par R/G/B discret (utiliser RGB discret ou NeoPixel ?)
// Supposons LED unique standard: ON = vert -> si LED RGB séparée faudrait lib; ici on simplifie: on allume HIGH.

void simpleLedsInit() {
    pinMode(PIN_INPUT, INPUT);
    // Rocker with pull-ups (required)
    pinMode(PIN_ROCKER_4, INPUT_PULLUP);
    pinMode(PIN_ROCKER_5, INPUT_PULLUP);
    single.begin();
    single.setPixelColor(0, single.Color(0, 50, 0)); // vert fixe
    single.show();

    strip.begin();
    strip.clear();
    strip.show();
    g_cachedColor = 0;
    g_lastPushedColor = 0;
    g_cachedRockerState = 0;
    g_lastRockerState = 0;
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

    // Rocker: poll at a slow interval to avoid impacting ADC scan timing
    static uint32_t lastPollMs = 0;
    const uint32_t nowMs = millis();
    if ((uint32_t)(nowMs - lastPollMs) >= 10) { // ~100 Hz polling
        lastPollMs = nowMs;
        const int r4 = digitalRead(PIN_ROCKER_4); // HIGH means that side is active
        const int r5 = digitalRead(PIN_ROCKER_5); // HIGH means that side is active
        uint8_t rockerState = 0; // default: off
        if (r4 == HIGH && r5 == HIGH) {
            rockerState = 2; // center → LED index 3 (1-based 4)
        } else if (r4 == HIGH) {
            rockerState = 3; // pin 4 HIGH → LED index 4 (1-based 5)
        } else if (r5 == HIGH) {
            rockerState = 1; // pin 5 HIGH → LED index 2 (1-based 3)
        } else {
            rockerState = 0; // both LOW → none
        }
        if (rockerState != g_cachedRockerState) {
            g_cachedRockerState = rockerState;
            g_needFlush = true;
        }
    }
}

void simpleLedsFrameFlush() {
    if (!g_needFlush && g_cachedColor == g_lastPushedColor && g_cachedRockerState == g_lastRockerState) {
        return; // rien à faire
    }
    // Update pin-3 status LED (index 1)
    strip.setPixelColor(1, g_cachedColor);

    // Update rocker indicator: LEDs 2 (left), 3 (center), 4 (right)
    // Set all three to off first
    strip.setPixelColor(2, 0);
    strip.setPixelColor(3, 0);
    strip.setPixelColor(4, 0);
    if (g_cachedRockerState != 0) {
        const uint32_t mauve = strip.Color(80, 0, 80);
        switch (g_cachedRockerState) {
            case 1: // pin 5 HIGH -> LED 5 (index 4)
                strip.setPixelColor(4, mauve);
                break;
            case 2: // both HIGH -> LED 4 (index 3)
                strip.setPixelColor(3, mauve);
                break;
            case 3: // pin 4 HIGH -> LED 3 (index 2)
                strip.setPixelColor(2, mauve);
                break;
        }
    }
    strip.show();
    g_lastPushedColor = g_cachedColor;
    g_lastRockerState = g_cachedRockerState;
    g_needFlush = false;
}
