// JANKO2 - 120-key piano with velocity detection
// Integrated velocity engine with state machine per key

#include <Arduino.h>
#include "config.h"
#include "simple_leds.h"
#include "velocity_engine.h"
#include "calibration.h"
#include "note_map.h"

// === USB MIDI is built-in on Teensy ===
// No additional setup required

// --- Paramètres MUX/ADC ---
constexpr uint8_t N_CHANNELS = 16;

// Broches de sélection du MUX
constexpr uint8_t PIN_S0 = kMuxPinS0;  // 35
constexpr uint8_t PIN_S1 = kMuxPinS1;  // 34
constexpr uint8_t PIN_S2 = kMuxPinS2;  // 36
constexpr uint8_t PIN_S3 = kMuxPinS3;  // 37

// Entrée analogique
constexpr uint8_t PIN_ADC = 39;  // A15 sur Teensy 4.1

// Temporisations pour scan ultra-rapide (focus canal 6)
constexpr uint32_t SCAN_INTERVAL_US = 100;      // 100µs = scan ultra-rapide
constexpr uint32_t SETTLE_US = 10;              // stabilisation minimale

// Échantillonnage minimal pour vitesse max
constexpr uint8_t SAMPLES_PER_READ = 1;         // 1 seul échantillon
constexpr bool USE_TRIMMED_MEAN = false;        // pas de filtrage

// --- État de balayage ---
uint8_t currentChannel = 0;
uint32_t lastScanUs = 0;
uint32_t frameCounter = 0;

// Debug output control - focus sur canal 6 uniquement
uint32_t lastDebugMs = 0;
constexpr uint32_t DEBUG_INTERVAL_MS = 50;      // Debug toutes les 50ms pour voir l'activité
constexpr uint8_t MONITORED_CHANNEL = 6;        // Canal à surveiller

// --- Utilitaires ---
void muxSelect(uint8_t ch) {
  digitalWriteFast(PIN_S0, (ch & 0x01) ? HIGH : LOW);
  digitalWriteFast(PIN_S1, (ch & 0x02) ? HIGH : LOW);
  digitalWriteFast(PIN_S2, (ch & 0x04) ? HIGH : LOW);
  digitalWriteFast(PIN_S3, (ch & 0x08) ? HIGH : LOW);
}

uint16_t readADCFast() {
  delayMicroseconds(SETTLE_US);
  
  if (SAMPLES_PER_READ == 1) {
    return analogRead(PIN_ADC);
  }
  
  uint32_t sum = 0;
  for (uint8_t i = 0; i < SAMPLES_PER_READ; ++i) {
    sum += analogRead(PIN_ADC);
  }
  return sum / SAMPLES_PER_READ;
}

void beginMux() {
  pinMode(PIN_S0, OUTPUT);
  pinMode(PIN_S1, OUTPUT);
  pinMode(PIN_S2, OUTPUT);
  pinMode(PIN_S3, OUTPUT);
  analogReadResolution(10); // 10 bits pour correspondre aux seuils
}

void printChannelSixInfo() {
  // Sélectionner et lire le canal 6 directement
  muxSelect(MONITORED_CHANNEL);
  uint16_t adc = readADCFast();
  
  // Afficher valeur ADC + état de la machine d'état
  KeyData& key = g_keys[0][MONITORED_CHANNEL];
  
  Serial.print("CH6: ADC="); Serial.print(adc);
  Serial.print(" State="); Serial.print(getStateName(key.state));
  Serial.print(" Triggers="); Serial.print(key.total_triggers);
  Serial.print(" FalseStarts="); Serial.print(key.false_starts);
  
  // Afficher seuils pour référence
  Serial.print(" [L="); Serial.print(kThresholdLow);
  Serial.print(" H="); Serial.print(kThresholdHigh);
  Serial.print(" R="); Serial.print(kThresholdRelease); Serial.print("]");
  Serial.println();
}

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000) { /* wait for host */ }
    
    Serial.println(F("=== JANKO2 Velocity Engine Starting ==="));
    
    // Initialize USB MIDI
    Serial.println("USB MIDI ready");
    
    // Initialize MUX hardware
    beginMux();
    muxSelect(currentChannel);
    lastScanUs = micros();
    Serial.println("MUX initialized");
    
    // Initialize LEDs
    simpleLedsInit();
    Serial.println("LEDs initialized");
    
    // Initialize velocity engine
    VelocityEngine::initialize();
    
    Serial.println("=== Ready for velocity detection ===");
    Serial.println("Touch keys to trigger notes!");
    Serial.println();
}

void loop() {
    const uint32_t nowUs = micros();
    const uint32_t nowMs = millis();

    // === Main scanning loop - Fast key sampling ===
    if (nowUs - lastScanUs >= SCAN_INTERVAL_US) {
        lastScanUs = nowUs;
        
        // Read current channel
        uint16_t adc_value = readADCFast();
        
        // IMMEDIATE velocity processing - don't wait for frame completion
        VelocityEngine::processKey(0, currentChannel, adc_value, nowUs);
        
        // Advance to next channel
        currentChannel = (currentChannel + 1) % N_CHANNELS;
        muxSelect(currentChannel);
        
        // Frame completion tracking
        if (currentChannel == 0) {
            frameCounter++;
        }
    }

    // === Handle other tasks ===
    // USB MIDI is handled automatically
    
    // Update LEDs
    simpleLedsTask();
    
    // === Debug output - Canal 6 uniquement ===
    if (nowMs - lastDebugMs >= DEBUG_INTERVAL_MS) {
        lastDebugMs = nowMs;
        printChannelSixInfo();
    }
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        