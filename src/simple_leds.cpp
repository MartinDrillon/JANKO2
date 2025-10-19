#include "simple_leds.h"
#include "config.h"
#include <Adafruit_NeoPixel.h>

// Pins (adapter si besoin)
static constexpr uint8_t PIN_SINGLE = 12;   // LED NeoPixel (SK6812 mini e) unique
static constexpr uint8_t PIN_STRIP  = 10;   // Strip NeoPixel
static constexpr uint8_t PIN_INPUT  = 3;    // Entrée à surveiller

// Strip de 5 LEDs sur PIN_STRIP; on utilisera l'index 1 (2ème LED) pour l'état du pin 3.
static Adafruit_NeoPixel strip(5, PIN_STRIP, NEO_GRB + NEO_KHZ800);
static Adafruit_NeoPixel single(1, PIN_SINGLE, NEO_GRB + NEO_KHZ800);

// Cache de l'état désiré pour la LED d'index 1 sur le strip.
static uint32_t g_cachedColor = 0;      // couleur souhaitée actuelle (LED index 1)
static uint32_t g_lastPushedColor = 0;  // dernière couleur réellement envoyée (LED index 1)
static bool g_button24Low_cached = false; // LED index 0 white when true
static bool g_button24Low_last = false;
// Rocker cached state: 0=off, 1=LEFT(LED idx 2), 2=CENTER(LED idx 3), 3=RIGHT(LED idx 4)
static uint8_t g_cachedRockerState = 0; // 0=off, 1=pin5, 2=both, 3=pin4
static uint8_t g_lastRockerState = 0;
static bool g_needFlush = false;        // un flush est nécessaire
static uint8_t g_brightness = kLedBrightness; // default from config.h
static bool g_calibBlueOverride = false; // when true, Phase 2: all LEDs solid blue
static bool g_firstFlush = true;        // Force first flush without conditions

// LED simple sur pin 12 sera traitée en sortie numérique + couleur fixe verte par R/G/B discret (utiliser RGB discret ou NeoPixel ?)
// Supposons LED unique standard: ON = vert -> si LED RGB séparée faudrait lib; ici on simplifie: on allume HIGH.

void simpleLedsInit() {
    pinMode(PIN_INPUT, INPUT);
    single.begin();
    single.setBrightness(kLedBrightness); // Apply brightness from config
    single.setPixelColor(0, single.Color(0, 50, 0)); // vert fixe
    single.show();

    strip.begin();
    strip.setBrightness(kLedBrightness); // Apply brightness from config
    strip.clear();
    strip.show();
    
    // Small delay to ensure NeoPixel hardware is ready
    delayMicroseconds(100); // 100µs should be enough for WS2812
    
    // Initialize state variables - set "last" states to invalid values to force initial update
    g_cachedColor = 0;
    g_lastPushedColor = 0xFFFFFFFF; // Force different from cached to trigger first flush
    g_cachedRockerState = 0;
    g_lastRockerState = 0xFF; // Force different from cached
    g_button24Low_cached = false;
    g_button24Low_last = true; // Force different from cached
    g_needFlush = true; // Mark flush needed
    g_brightness = kLedBrightness; // Initialize from config.h
    g_calibBlueOverride = false;
    
    // Read actual pin states to initialize cached colors
    int state = digitalRead(PIN_INPUT);
    g_cachedColor = (state == HIGH) ? strip.Color(0,0,80) : 0;
    
    // Force initial flush to display correct state at startup
    // This ensures LEDs light up immediately after power-on or firmware flash
    simpleLedsFrameFlush();
}

void simpleLedsTask() {
    // Conservé pour compat rétro; appelle la logique lazy puis flush direct.
    simpleLedsUpdateInputState();
    simpleLedsFrameFlush();
}

void setCalibrationLeds(bool enabled) {
    // NOTE: This function directly manipulates strip pixels and calls show()
    // It bypasses the normal cached state system and should only be used
    // during calibration mode when normal LED updates are suspended
    if (enabled) {
        // Turn on LEDs 2, 3, 4 in red (indices 2, 3, 4 - last 3 LEDs of the 5-LED strip)
        strip.setPixelColor(2, strip.Color(255, 0, 0)); // Red
        strip.setPixelColor(3, strip.Color(255, 0, 0)); // Red
        strip.setPixelColor(4, strip.Color(255, 0, 0)); // Red
        strip.show();
    }
    // When disabled, do nothing - let normal LED system handle it
    // This prevents overwriting the rocker indicator LEDs at startup
}

void simpleLedsSetCalibrationBlue(bool enabled) {
    if (g_calibBlueOverride != enabled) {
        g_calibBlueOverride = enabled;
        g_needFlush = true;
    }
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

// External setter called by IoState wiring
void simpleLedsSetRocker(bool pin4High, bool pin5High) {
    uint8_t rockerState = 0;
    if (pin4High && pin5High) rockerState = 2;     // both -> LED 4
    else if (pin4High)        rockerState = 3;     // pin 4 -> LED 3
    else if (pin5High)        rockerState = 1;     // pin 5 -> LED 5
    else                      rockerState = 0;     // none
    if (rockerState != g_cachedRockerState) {
        g_cachedRockerState = rockerState;
        g_needFlush = true;
    }
}

void simpleLedsFrameFlush() {
    // Force first flush to ensure LEDs display at startup
    if (!g_firstFlush && !g_needFlush && g_cachedColor == g_lastPushedColor && g_cachedRockerState == g_lastRockerState && g_button24Low_cached == g_button24Low_last && !g_calibBlueOverride) {
        return; // rien à faire
    }
    g_firstFlush = false; // Clear first flush flag after first call
    
    // Apply brightness globally
    strip.setBrightness(g_brightness);
    if (g_calibBlueOverride) {
        // Solid blue on all 5 LEDs
        const uint32_t blue = strip.Color(0, 0, 120);
        for (uint16_t i = 0; i < strip.numPixels(); ++i) strip.setPixelColor(i, blue);
    } else {
        // Update button-24 LED 1 (index 0): white when LOW
        strip.setPixelColor(0, g_button24Low_cached ? strip.Color(80,80,80) : 0);
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
    }
    strip.show();
    g_lastPushedColor = g_cachedColor;
    g_lastRockerState = g_cachedRockerState;
    g_button24Low_last = g_button24Low_cached;
    g_needFlush = false;
}

void simpleLedsSetButton24(bool isLowPressed) {
    if (g_button24Low_cached != isLowPressed) {
        g_button24Low_cached = isLowPressed;
        g_needFlush = true;
    }
}

void simpleLedsSetBrightness(uint8_t level) {
    if (g_brightness != level) {
        g_brightness = level;
        g_needFlush = true;
    }
}

uint8_t simpleLedsGetBrightness() { return g_brightness; }
