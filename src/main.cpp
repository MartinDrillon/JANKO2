// Minimal entry point for the 120-key piano project skeleton.
// Robust MUX+ADC scanning with filtering

#include <Arduino.h>
#include "config.h"
#include "simple_leds.h"

// --- Paramètres MUX/ADC ---
constexpr uint8_t N_CHANNELS = 16;

// Broches de sélection du MUX (utilise tes pins existants) :
constexpr uint8_t PIN_S0 = kMuxPinS0;  // 16
constexpr uint8_t PIN_S1 = kMuxPinS1;  // 17
constexpr uint8_t PIN_S2 = kMuxPinS2;  // 15
constexpr uint8_t PIN_S3 = kMuxPinS3;  // 14

// Entrée analogique :
constexpr uint8_t PIN_ADC = 39;  // A15 sur Teensy 4.1

// Temporisations (switch toutes les secondes)
constexpr uint32_t DWELL_PER_CHANNEL_MS = 1000;  // 1 seconde par canal
constexpr uint32_t SETTLE_US = 100;              // stabilisation après sélection

// Échantillonnage/filtrage
constexpr uint8_t SAMPLES_PER_READ = 4;         // 4 échantillons (compromis vitesse/qualité)
constexpr bool USE_TRIMMED_MEAN = true;         // retire min/max puis moyenne

// --- Stockage des mesures ---
volatile bool frameReady = false;

// Buffer de travail (rempli pendant le balayage)
int workingValues[N_CHANNELS] = {0};
// Snapshot stable (lisible ailleurs)
int snapshotValues[N_CHANNELS] = {0};

// État de balayage
uint8_t currentChannel = 0;
uint32_t lastSwitchMs = 0;

// --- Utilitaires ---
void muxSelect(uint8_t ch) {
  digitalWriteFast(PIN_S0, (ch & 0x01) ? HIGH : LOW);
  digitalWriteFast(PIN_S1, (ch & 0x02) ? HIGH : LOW);
  digitalWriteFast(PIN_S2, (ch & 0x04) ? HIGH : LOW);
  digitalWriteFast(PIN_S3, (ch & 0x08) ? HIGH : LOW);
}

int readADCFiltered() {
  delayMicroseconds(SETTLE_US);

  uint16_t buf[SAMPLES_PER_READ];
  for (uint8_t i = 0; i < SAMPLES_PER_READ; ++i) {
    buf[i] = analogRead(PIN_ADC);
  }

  if (USE_TRIMMED_MEAN && SAMPLES_PER_READ >= 3) {
    uint16_t mn = 0xFFFF, mx = 0;
    uint32_t sum = 0;
    for (uint8_t i = 0; i < SAMPLES_PER_READ; ++i) {
      uint16_t v = buf[i];
      if (v < mn) mn = v;
      if (v > mx) mx = v;
      sum += v;
    }
    sum -= (mn + mx);
    return sum / (SAMPLES_PER_READ - 2);
  } else {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < SAMPLES_PER_READ; ++i) sum += buf[i];
    return sum / SAMPLES_PER_READ;
  }
}

void beginMux() {
  pinMode(PIN_S0, OUTPUT);
  pinMode(PIN_S1, OUTPUT);
  pinMode(PIN_S2, OUTPUT);
  pinMode(PIN_S3, OUTPUT);
  analogReadResolution(10); // 10 bits
}

// Fonction pour obtenir la valeur d'un canal spécifique
int getChannelValue(uint8_t channel) {
    if (channel < N_CHANNELS) {
        return snapshotValues[channel];
    }
    return -1;
}

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 2000) { /* wait briefly for host */ }
    Serial.println(F("JANKO2 MUX+ADC robuste demo start"));
    
    beginMux();
    muxSelect(currentChannel);
    lastSwitchMs = millis();
    simpleLedsInit();
}

void loop() {
    const uint32_t now = millis();

    // Si on est resté assez longtemps sur ce canal, on lit et on passe au suivant
    if (now - lastSwitchMs >= DWELL_PER_CHANNEL_MS) {
        // 1) Lire l'ADC filtré sur le canal courant
        int v = readADCFiltered();

        // 2) Stocker au bon index
        workingValues[currentChannel] = v;

        // 3) Affichage immédiat pour débogage
        Serial.printf("CH%u=%d\n", currentChannel, v);

        // 4) Avancer au canal suivant
        currentChannel = (currentChannel + 1) % N_CHANNELS;
        muxSelect(currentChannel);

        // 5) Si on boucle vers 0, c'est qu'un tour complet est fini
        if (currentChannel == 0) {
            // Copier working -> snapshot
            for (uint8_t i = 0; i < N_CHANNELS; ++i) {
                snapshotValues[i] = workingValues[i];
            }
            frameReady = true;
            Serial.println("=== Frame complete ===");
        }

        // 6) Recaler l'horloge
        lastSwitchMs = now;
    }

    simpleLedsTask();
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        