#include "io_state.h"

namespace IoState {
    static bool sInited = false;
    static RockerStatus sLast{true, true, 0, false};
    static uint32_t sLastPollMs = 0;
    static constexpr uint32_t kPollPeriodMs = 10; // ~100 Hz

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
        sLast.pin4High = p4;
        sLast.pin5High = p5;
        sLast.transpose = calcTranspose(p4, p5);
        sLast.button24Low = b24low;
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
        bool changed = (p4 != sLast.pin4High) || (p5 != sLast.pin5High) || (tr != sLast.transpose) || (b24low != sLast.button24Low);
        if (changed) {
            sLast.pin4High = p4;
            sLast.pin5High = p5;
            sLast.transpose = tr;
            sLast.button24Low = b24low;
        }
        outStatus = sLast;
        return changed;
    }
}
