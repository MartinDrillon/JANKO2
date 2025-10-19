#pragma once
#include <Arduino.h>
#include "config.h"

namespace EepromStore {
    // Load per-key Low/High and velocity gamma from EEPROM; returns true if valid CRC and version.
    // If gamma is nullptr, it is not loaded.
    bool load(uint16_t low[N_MUX][N_CH], uint16_t high[N_MUX][N_CH], float* gamma = nullptr);
    // Save per-key Low/High and velocity gamma to EEPROM (overwrites previous contents).
    // If gamma is nullptr, current value is preserved or defaults to kVelocityGammaDefault.
    void save(const uint16_t low[N_MUX][N_CH], const uint16_t high[N_MUX][N_CH], const float* gamma = nullptr);
}
