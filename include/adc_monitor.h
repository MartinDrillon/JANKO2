#pragma once
#include <Arduino.h>
#include "config.h"

#if DEBUG_ADC_MONITOR
// Stocke la dernière valeur ADC pour la paire (MUX, CHANNEL) choisie.
namespace AdcMonitor {
    extern volatile uint16_t g_lastValue;
    extern volatile uint32_t g_lastTimestampUs;
    void updateIfMatch(uint8_t mux, uint8_t channel, uint16_t adcValue, uint32_t t_us);
    void printPeriodic();
    void begin();
}
#endif
