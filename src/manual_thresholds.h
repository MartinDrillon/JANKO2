#pragma once
#include <stdint.h>
#include "config.h"

// Forward declarations for manual override setters (expected to be provided in calibration or thresholds module)
// If they don't exist yet, you'll need to implement them accordingly.
void setManualLow(int mux, int ch, uint16_t v);
void setManualHigh(int mux, int ch, uint16_t v);
void setManualMask(int mux, int ch, bool enabled);

namespace ManualThresholds {

struct Entry {
    uint16_t low;
    uint16_t high;
};

// Editable table: default all {720,880}. User can later modify values.
extern const Entry kTable[N_MUX][N_CH];

void setEnabled(bool on);
bool isEnabled();

// Apply the manual table to runtime manual threshold buffers and activate mask if enable==true.
// Clamps into 0..1023 and enforces high >= low + kMinSwingForValidity
void apply(bool enable);

// Debug print of table (always prints stored constants, not live runtime values)
void print();

} // namespace ManualThresholds
