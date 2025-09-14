#include "calibration.h"

// === Calibration par touche - Données de collecte ===
// Histogrammes pour calcul de médiane par touche (256 Ko RAM sur Teensy 4.1)
uint16_t gHistLow[N_MUX][N_CH][1024] = {0};   // zero-initialized
uint16_t gTotalLow[N_MUX][N_CH] = {0};        // compteurs d'échantillons

// === Seuils runtime par touche ===
// Initialisés à valeur par défaut 650, mis à jour par calibration
uint16_t gThresholdLowPerKey[N_MUX][N_CH];

// === Fonctions de calibration ===
void initializeCalibration() {
    // Initialiser tous les seuils à la valeur par défaut
    for (uint8_t mux = 0; mux < N_MUX; mux++) {
        for (uint8_t ch = 0; ch < N_CH; ch++) {
            gThresholdLowPerKey[mux][ch] = 650; // valeur par défaut
        }
    }
    
    // Les histogrammes sont déjà zero-initialized
}

void clearCalibrationData() {
    // Optionnel: réinitialiser les histogrammes pour libérer la mémoire
    for (uint8_t mux = 0; mux < N_MUX; mux++) {
        for (uint8_t ch = 0; ch < N_CH; ch++) {
            gTotalLow[mux][ch] = 0;
            for (uint16_t i = 0; i < 1024; i++) {
                gHistLow[mux][ch][i] = 0;
            }
        }
    }
}

uint16_t calculateMedianForKey(uint8_t mux, uint8_t channel) {
    if (mux >= N_MUX || channel >= N_CH) return 650;
    
    uint16_t total = gTotalLow[mux][channel];
    if (total == 0) {
        return 650; // fallback si aucun échantillon
    }
    
    uint16_t target = total / 2;
    uint16_t cumul = 0;
    
    for (uint16_t v = 0; v < 1024; v++) {
        cumul += gHistLow[mux][channel][v];
        if (cumul >= target) {
            return v; // médiane trouvée
        }
    }
    
    return 650; // fallback (ne devrait pas arriver)
}