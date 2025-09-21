#include "calibration_controller.h"
#include "io_state.h"
#include "simple_leds.h"
#include "calibration.h"
#include "config.h"
#include "key_state.h"
#include <EEPROM.h>

namespace {
    enum class State : uint8_t { Idle, ArmingStart, CollectLow, CaptureHigh, AwaitFinish };
    State gState = State::Idle;
    uint32_t gStateStartMs = 0;
    uint32_t gPin24LowSinceMs = 0;
    uint32_t gPin24HighSinceMs = 0;

    // CaptureHigh tracking: per-key maximum absolute delta from Low
    uint16_t gMaxDeltaAbs[N_MUX][N_CH];

    inline int16_t absDelta(uint16_t a, uint16_t b) {
        return (a > b) ? (int16_t)(a - b) : (int16_t)(b - a);
    }

    // EEPROM layout: header + per key Low/High (2x uint16)
    struct Header { uint32_t magic; uint16_t version; uint16_t crc; };
    constexpr uint32_t kMagic = 0x4A4B3243; // 'JK2C'
    constexpr uint16_t kVersion = 1;

    uint16_t crc16_update(uint16_t crc, uint8_t data) {
        crc ^= data;
        for (uint8_t i=0;i<8;i++) crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : (crc >> 1);
        return crc;
    }

    void eepromWriteThresholds() {
        Header hdr{ kMagic, kVersion, 0 };
        size_t addr = 0;
        EEPROM.put(addr, hdr); addr += sizeof(Header);
        // Write per-key Low/High
        for (uint8_t m=0;m<N_MUX;m++) {
            for (uint8_t c=0;c<N_CH;c++) {
                uint16_t low = gThLow[m][c];
                uint16_t high = gThHigh[m][c];
                EEPROM.put(addr, low); addr += sizeof(uint16_t);
                EEPROM.put(addr, high); addr += sizeof(uint16_t);
            }
        }
        // Compute CRC over payload (Low/High array)
        uint16_t crc = 0xFFFF;
        size_t payloadAddr = sizeof(Header);
        for (size_t i=0; i< (size_t)N_MUX*(size_t)N_CH*2*sizeof(uint16_t); ++i) {
            uint8_t b = EEPROM.read(payloadAddr + i);
            crc = crc16_update(crc, b);
        }
        // Update header CRC
        hdr.crc = crc;
        EEPROM.put(0, hdr);
    }

    bool eepromReadThresholds() {
        Header hdr; EEPROM.get(0, hdr);
        if (hdr.magic != kMagic || hdr.version != kVersion) return false;
        // verify CRC
        uint16_t crc = 0xFFFF;
        size_t payloadAddr = sizeof(Header);
        for (size_t i=0; i< (size_t)N_MUX*(size_t)N_CH*2*sizeof(uint16_t); ++i) {
            uint8_t b = EEPROM.read(payloadAddr + i);
            crc = crc16_update(crc, b);
        }
        if (crc != hdr.crc) return false;
        // load values
        size_t addr = sizeof(Header);
        for (uint8_t m=0;m<N_MUX;m++) {
            for (uint8_t c=0;c<N_CH;c++) {
                uint16_t low, high;
                EEPROM.get(addr, low); addr += sizeof(uint16_t);
                EEPROM.get(addr, high); addr += sizeof(uint16_t);
                gThLow[m][c] = low;
                gThHigh[m][c] = high;
            }
        }
        return true;
    }

    void startCollectLow(uint32_t nowMs) {
        simpleLedsSetOverride(LedOverride::CalibBlinkRed);
        // Set window to 1s per config
        calibrationStartCollectLow();
        gState = State::CollectLow;
        gStateStartMs = nowMs;
    }

    void finalizeCollectLow() {
        // Will be finalized by calibrationService; then we switch state in service loop
        (void)0;
    }

    void startCaptureHigh(uint32_t nowMs) {
        simpleLedsSetOverride(LedOverride::CalibBlueSolid);
        // zero max deltas
        memset(gMaxDeltaAbs, 0, sizeof(gMaxDeltaAbs));
        gState = State::CaptureHigh;
        gStateStartMs = nowMs;
    }

    void applyFinalizeRulesAndSave() {
        // Apply delta<30 rule, default High=920 if none existed
        for (uint8_t m=0;m<N_MUX;m++) {
            for (uint8_t c=0;c<N_CH;c++) {
                uint16_t low = gThLow[m][c];
                uint16_t prevHigh = gThHigh[m][c];
                uint16_t newHigh;
                if (gMaxDeltaAbs[m][c] < 30) {
                    newHigh = (prevHigh != 0) ? prevHigh : 920; // default 920 if none
                } else {
                    // Determine absolute High value farthest from Low using captured delta
                    // We need actual ADC value corresponding to max |ADC-Low|. Simplify: choose side by last observed sample direction vs Low.
                    // Without per-key last-sample store here, approximate: if prevHigh>=low then newHigh = clamp(low + gMaxDeltaAbs, 0..1023) else low - gMaxDeltaAbs.
                    int sign = (prevHigh >= low) ? +1 : -1;
                    int candidate = (int)low + sign * (int)gMaxDeltaAbs[m][c];
                    if (candidate < 0) candidate = 0; if (candidate > 1023) candidate = 1023;
                    newHigh = (uint16_t)candidate;
                }
                // After setting final High/Low per key, apply 10% adjustment:
                // - Move High 10% toward Low
                // - Move Low 10% away from High
                // This is polarity-aware and preserves the swing (gap unchanged), shifting both by the same delta.
                int signHL = (newHigh >= low) ? +1 : -1;
                uint16_t gap = (newHigh >= low) ? (newHigh - low) : (low - newHigh);
                // Fractional nudge of gap from config (rounded)
                int delta = (int)((float)gap * kThresholdNudgePct + 0.5f);
                // Adjust
                int adjHigh = (int)newHigh - signHL * delta; // toward Low
                int adjLow  = (int)low     - signHL * delta; // away from High
                // Clamp to ADC range
                if (adjHigh < 0) adjHigh = 0; if (adjHigh > 1023) adjHigh = 1023;
                if (adjLow  < 0) adjLow  = 0; if (adjLow  > 1023) adjLow  = 1023;
                gThLow[m][c]  = (uint16_t)adjLow;
                gThHigh[m][c] = (uint16_t)adjHigh;
            }
        }
        eepromWriteThresholds();
    }
}

namespace CalibrationCtl {
    void init() {
        // Try to load EEPROM thresholds at boot; else keep current defaults
        eepromReadThresholds();
        gState = State::Idle;
        gStateStartMs = millis();
        gPin24LowSinceMs = 0;
        gPin24HighSinceMs = millis();
    }

    void onFrameSwap() {
        if (calibrationIsCollecting()) {
            calibrationFrameIngest(g_acquisition.workingValues);
        }
    }

    void service(uint32_t nowMs) {
        // Read button 24 via IoState (we assume main already called IoState::update and set LEDs elsewhere)
        // For timing robustness, we read the pin directly here too.
        pinMode(IoState::kPinButton24, INPUT_PULLUP);
        bool low = (digitalReadFast(IoState::kPinButton24) == LOW);
        if (low) {
            if (gPin24LowSinceMs == 0) gPin24LowSinceMs = nowMs;
            gPin24HighSinceMs = 0;
        } else {
            if (gPin24HighSinceMs == 0) gPin24HighSinceMs = nowMs;
            gPin24LowSinceMs = 0;
        }

        switch (gState) {
            case State::Idle: {
                if (low && (nowMs - gPin24LowSinceMs >= 3000)) {
                    gState = State::ArmingStart; // require release before actual start
                }
                break; }
            case State::ArmingStart: {
                // wait for HIGH ≥ 200 ms before starting collection
                if (!low && (nowMs - gPin24HighSinceMs >= 200)) {
                    startCollectLow(nowMs);
                }
                break; }
            case State::CollectLow: {
                // Keep blinking red; median window set to 1s in config
                if (!calibrationIsCollecting()) {
                    // finalize has been scheduled; calibrationService will set RUN after finalize
                    finalizeCollectLow();
                    // switch when service finalized
                    startCaptureHigh(nowMs);
                }
                break; }
            case State::CaptureHigh: {
                // Track max |ADC - Low| per key
                for (uint8_t m=0;m<N_MUX;m++) {
                    for (uint8_t c=0;c<N_CH;c++) {
                        uint16_t v = g_acquisition.workingValues[m][c];
                        uint16_t lowV = gThLow[m][c];
                        uint16_t d = (uint16_t)absDelta(v, lowV);
                        if (d > gMaxDeltaAbs[m][c]) gMaxDeltaAbs[m][c] = d;
                    }
                }
                // Transition to finish when LOW ≥ 1s, with prior HIGH ≥200ms constraint satisfied earlier
                if (low && (nowMs - gPin24LowSinceMs >= 1000)) {
                    applyFinalizeRulesAndSave();
                    simpleLedsSetOverride(LedOverride::None);
                    gState = State::Idle;
                }
                break; }
            case State::AwaitFinish: {
                // Unused in this refined flow
                break; }
        }
        // finalize low median if needed
        calibrationService();
    }
}
