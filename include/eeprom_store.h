#pragma once
#include <Arduino.h>
#include "config.h"

namespace EepromStore {
    // Load per-key Low/High from EEPROM; returns true if valid CRC and version.
    bool load(uint16_t low[N_MUX][N_CH], uint16_t high[N_MUX][N_CH]);
    // Save per-key Low/High to EEPROM (overwrites previous contents).
    void save(const uint16_t low[N_MUX][N_CH], const uint16_t high[N_MUX][N_CH]);
}
