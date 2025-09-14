// New threshold API with manual override support
#pragma once
#include <stdint.h>
#include "config.h"

namespace Calib {

// Auto-learned thresholds (from histogram/calibration)
extern uint16_t thLowAuto[N_MUX][N_CH];
extern uint16_t thHighAuto[N_MUX][N_CH];

// Manual override thresholds and mask
extern uint16_t thLowManual[N_MUX][N_CH];
extern uint16_t thHighManual[N_MUX][N_CH];
extern bool     thManualMask[N_MUX][N_CH];

// Core constants
constexpr int  kMinSwingForValidity = 50;   // High >= Low + 50
constexpr int  kReleaseDelta        = 50;   // Release = High - 50
constexpr uint8_t kStableCount      = 1;    // consecutive samples
constexpr uint16_t kMinDeltaADC     = 5;    // minimum delta for velocity
constexpr float kSpeedMin           = 300.0f;
constexpr float kSpeedMax           = 30000.0f;
constexpr uint32_t kTrackingTimeoutUs = 1000000; // 1s

// Initialize all buffers to defaults
void thresholdsInit(uint16_t lowDefault, uint16_t highDefault);

// Auto setters (calibration)
void setAutoLow (int m,int c,uint16_t v);
void setAutoHigh(int m,int c,uint16_t v);

// Manual setters (manual_thresholds)
void setManualLow (int m,int c,uint16_t v);
void setManualHigh(int m,int c,uint16_t v);
void setManualMask(int m,int c,bool on);

// Getters (always use these in engine)
uint16_t getLow(int m,int c);
uint16_t getHigh(int m,int c);
uint16_t getRelease(int m,int c);

// Velocity utility (speed already computed) - optional
inline uint8_t mapSpeedToVelocity(float speed) {
    if (speed <= kSpeedMin) return 1;
    if (speed >= kSpeedMax) return 127;
    float normalized = (speed - kSpeedMin) / (kSpeedMax - kSpeedMin);
    return 1 + static_cast<uint8_t>(normalized * 126.0f);
}

} // namespace Calib
