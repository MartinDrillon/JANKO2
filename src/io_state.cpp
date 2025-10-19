#include "io_state.h"

namespace IoState {
    static bool sInited = false;
    static RockerStatus sLast{true, true, 0, false, 0, Btn24Click::None};
    static uint32_t sLastPollMs = 0;
    static constexpr uint32_t kPollPeriodMs = 10; // ~100 Hz
    // Quadrature decoder state
    static volatile int16_t sEncAccum = 0;
    static volatile uint8_t sEncState = 0; // 2-bit: (A<<1)|B
    // Button 24 click detection state
    static bool sBtn24Prev = false; // false = released (HIGH when not pressed)
    static uint32_t sBtn24PressTimeMs = 0;
    static uint8_t sBtn24ClickCount = 0;
    static uint32_t sBtn24LastReleaseMs = 0;
    static constexpr uint32_t kClickWindowMs = 400; // max time between clicks for double/triple
    static constexpr uint32_t kShortPressMaxMs = 300; // short press < 300ms
    static constexpr uint32_t kDebounceMs = 20;

    // Transition table: prev(2b)<<2 | cur(2b) -> delta {-1,0,+1}
    // Valid steps: 00->01(+1), 01->11(+1), 11->10(+1), 10->00(+1)
    // Reverse for -1; all others 0 (including bounces)
    static inline int8_t quadDelta(uint8_t prev, uint8_t cur) {
        uint8_t key = (prev << 2) | cur;
        switch (key) {
            case 0b0001:
            case 0b0111:
            case 0b1110:
            case 0b1000: return +1;
            case 0b0010:
            case 0b1011:
            case 0b1101:
            case 0b0100: return -1;
            default: return 0;
        }
    }

    static void onEncChange() {
        // Read A/B quickly
        uint8_t a = digitalReadFast(kPinEncA) ? 1 : 0;
        uint8_t b = digitalReadFast(kPinEncB) ? 1 : 0;
        uint8_t cur = (a << 1) | b;
        uint8_t prev = sEncState & 0x3;
        if (cur != prev) {
            int8_t d = quadDelta(prev, cur);
            sEncAccum += d;
            sEncState = cur;
        }
    }

    static inline int8_t calcTranspose(bool pin4High, bool pin5High) {
        if (pin4High && pin5High) return 0;   // neutral
        if (pin4High && !pin5High) return -12; // 4 HIGH -> down
        if (!pin4High && pin5High) return +12; // 5 HIGH -> up
        return 0; // both LOW -> neutral
    }

    void init() {
        if (sInited) return;
        pinMode(kPinRocker4, INPUT_PULLUP);
        pinMode(kPinRocker5, INPUT_PULLUP);
        bool p4 = digitalReadFast(kPinRocker4);
        bool p5 = digitalReadFast(kPinRocker5);
        pinMode(kPinButton24, INPUT_PULLUP);
        bool b24low = (digitalReadFast(kPinButton24) == LOW);
    // Encoder inputs as pullups (A/B)
    pinMode(kPinEncA, INPUT_PULLUP);
    pinMode(kPinEncB, INPUT_PULLUP);
    uint8_t a = digitalReadFast(kPinEncA) ? 1 : 0;
    uint8_t b = digitalReadFast(kPinEncB) ? 1 : 0;
    sEncState = (a << 1) | b;
    sEncAccum = 0;
    attachInterrupt(digitalPinToInterrupt(kPinEncA), onEncChange, CHANGE);
    attachInterrupt(digitalPinToInterrupt(kPinEncB), onEncChange, CHANGE);
        // Button 24 click detection init
        sBtn24Prev = b24low;
        sLast.pin4High = p4;
        sLast.pin5High = p5;
        sLast.transpose = calcTranspose(p4, p5);
        sLast.button24Low = b24low;
        sLast.encDelta = 0;
        sLast.btn24Click = Btn24Click::None;
        sLastPollMs = millis();
        sInited = true;
    }

    bool update(uint32_t nowMs, RockerStatus& outStatus) {
        if (!sInited) init();
        if ((nowMs - sLastPollMs) < kPollPeriodMs) {
            outStatus = sLast;
            return false;
        }
        sLastPollMs = nowMs;
        bool p4 = digitalReadFast(kPinRocker4);
        bool p5 = digitalReadFast(kPinRocker5);
        bool b24low = (digitalReadFast(kPinButton24) == LOW);
        int8_t tr = calcTranspose(p4, p5);
    // Drain accumulated quadrature steps since last poll (atomically)
    int16_t acc;
    noInterrupts();
    acc = sEncAccum;
    // Keep remainder so partial transitions aren't lost between polls
    sEncAccum = (int16_t)(acc % 4);
    interrupts();
    // Each valid transition gives +/-1; typical encoders produce 4 transitions per detent.
    // Convert to detents (round toward zero)
    int8_t encDelta = (int8_t)(acc / 4);
        // Button 24 click detection (short/double/triple, not long 3s hold)
        Btn24Click detectedClick = Btn24Click::None;
        if (sBtn24Prev && !b24low) {
            // Button released (pressed->released transition)
            uint32_t pressDur = nowMs - sBtn24PressTimeMs;
            if (pressDur >= kDebounceMs && pressDur < kShortPressMaxMs) {
                // Valid short press (<300ms)
                uint32_t timeSinceLastRelease = nowMs - sBtn24LastReleaseMs;
                if (timeSinceLastRelease < kClickWindowMs && sBtn24ClickCount > 0) {
                    // Within window: increment click count
                    sBtn24ClickCount++;
                } else {
                    // Start new click sequence
                    sBtn24ClickCount = 1;
                }
                sBtn24LastReleaseMs = nowMs;
            }
        } else if (!sBtn24Prev && b24low) {
            // Button pressed (released->pressed transition)
            sBtn24PressTimeMs = nowMs;
        }
        // Check if click window expired
        if (sBtn24ClickCount > 0 && (nowMs - sBtn24LastReleaseMs) >= kClickWindowMs) {
            // Window expired: report accumulated clicks
            if (sBtn24ClickCount == 1) detectedClick = Btn24Click::Short;
            else if (sBtn24ClickCount == 2) detectedClick = Btn24Click::Double;
            else if (sBtn24ClickCount >= 3) detectedClick = Btn24Click::Triple;
#ifdef DEBUG_GAMMA_MONITOR
            Serial.printf("[IoState] Button24 click detected: count=%u type=%d\n", sBtn24ClickCount, (int)detectedClick);
#endif
            sBtn24ClickCount = 0;
        }
        sBtn24Prev = b24low;
        bool changed = (p4 != sLast.pin4High) || (p5 != sLast.pin5High) || (tr != sLast.transpose) || (b24low != sLast.button24Low);
        // Always update btn24Click (even if None) to clear previous clicks
        sLast.pin4High = p4;
        sLast.pin5High = p5;
        sLast.transpose = tr;
        sLast.button24Low = b24low;
        sLast.encDelta = encDelta;
        sLast.btn24Click = detectedClick;
        outStatus = sLast;
        return changed || (encDelta != 0) || (detectedClick != Btn24Click::None);
    }
}
