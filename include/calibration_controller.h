#pragma once
#include <Arduino.h>

namespace CalibrationCtl {
    void init();
    // Call at end of each loop (outside hot path) with current time
    void service(uint32_t nowMs);
    // Notify per-frame acquisition (snapshot) for median ingest during CollectLow
    void onFrameSwap();
}
