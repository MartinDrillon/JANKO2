#include "adc_monitor.h"
#include "calibration.h"
#if DEBUG_ADC_MONITOR

namespace AdcMonitor {
    volatile uint16_t g_lastValue = 0;
    volatile uint32_t g_lastTimestampUs = 0;
    static uint32_t g_lastPrintMs = 0;

    void begin() {
        // Serial activé uniquement si monitor actif (pour ne pas réintroduire ailleurs)
        Serial.begin(115200);
        while(!Serial && millis() < 2000) { /* attendre éventuelle connexion USB */ }
        Serial.println("[ADC MONITOR] Active");
        Serial.print("Target MUX="); Serial.print(DEBUG_ADC_MONITOR_MUX);
        Serial.print(" CHANNEL="); Serial.println(DEBUG_ADC_MONITOR_CHANNEL);
    }

    void updateIfMatch(uint8_t mux, uint8_t channel, uint16_t adcValue, uint32_t t_us) {
        if (mux == DEBUG_ADC_MONITOR_MUX && channel == DEBUG_ADC_MONITOR_CHANNEL) {
            g_lastValue = adcValue;
            g_lastTimestampUs = t_us;
        }
    }

    void printPeriodic() {
        uint32_t nowMs = millis();
        if (nowMs - g_lastPrintMs >= DEBUG_ADC_MONITOR_INTERVAL_MS) {
            g_lastPrintMs = nowMs;
            // Read thresholds live
            uint16_t low  = calibLow(DEBUG_ADC_MONITOR_MUX, DEBUG_ADC_MONITOR_CHANNEL);
            uint16_t high = calibHigh(DEBUG_ADC_MONITOR_MUX, DEBUG_ADC_MONITOR_CHANNEL);
            uint16_t rel  = calibRelease(DEBUG_ADC_MONITOR_MUX, DEBUG_ADC_MONITOR_CHANNEL);
            int8_t   s    = (high >= low) ? 1 : -1;
            Serial.print("ADC[");
            Serial.print(DEBUG_ADC_MONITOR_MUX);
            Serial.print(",");
            Serial.print(DEBUG_ADC_MONITOR_CHANNEL);
            Serial.print("]=");
            Serial.print(g_lastValue);
            Serial.print(" @");
            Serial.print(g_lastTimestampUs);
            Serial.print("us  Low=");
            Serial.print(low);
            Serial.print(" High=");
            Serial.print(high);
            Serial.print(" Release=");
            Serial.print(rel);
            Serial.print(" Pol=");
            Serial.println(s > 0 ? "+" : "-");
        }
    }
}

#endif
