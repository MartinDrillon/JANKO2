#pragma once
#include <Arduino.h>
#include "config.h" // pour N_MUX / N_CH
#include "io_state.h"
#include "eeprom_store.h"

// === ADC Configuration ===
// Référence ADC : 10 bits = 0..1023, Vref = 3.3V

// === Calibration dynamique par touche (Phase 1) ===
namespace Calib {
	// Constantes Phase1 (Phase2 en ajoutera d'autres)
	// kLowMarginCounts remplacé par CalibCfg::kLowMarginPct/Min
	constexpr uint16_t kMinSwingCounts   = 30;   // High >= Low + min swing
	constexpr uint16_t kHighStartDefault = 915;   // High initial si pas encore appris
	// kHighTargetMargin remplacé par CalibCfg::kHighTargetMarginPct/Min
	constexpr float    kHighAlpha        = 0.85f; // EMA lente
	constexpr float    kHighAlphaFast    = 0.60f; // EMA rapide premières notes
	constexpr uint8_t  kHighFastNotes    = 3;     // nb notes « fast »
	// kReleaseDelta remplacé par CalibCfg::kReleaseDeltaPct/Min
		// Phase2
		constexpr uint32_t kMedianWindowMs   = 2000;  // collecte médiane Low
		// FSM UX timings
		constexpr uint32_t kHoldToStartMs    = 3000;  // hold 3s to start
		constexpr uint32_t kHoldToFinishMs   = 1000;  // hold 1s to end Phase2
		constexpr uint16_t kMinSwingForHigh  = 50;    // if |High-Low| < this, fallback
}

// Tables runtime (initialisées dans calibration.cpp)
extern uint16_t gThLow[N_MUX][N_CH];
extern uint16_t gThHigh[N_MUX][N_CH];
extern uint8_t  gHighFastSeen[N_MUX][N_CH];

// Initialisation statique (Phase1): valeurs de base avant médiane (Phase2)
void calibrationInitStatic();
// Mise à jour High après NoteOff avec peak détecté
void updateHighAfterNote(uint8_t mux, uint8_t ch, uint16_t peak);
// Phase2 API
void calibrationStartCollectLow();
void calibrationFrameIngest(const uint16_t frameValues[N_MUX][N_CH]);
void calibrationService();
bool calibrationIsCollecting();
bool calibrationIsRunning();

// Calibration control API (FSM)
void calibrationLoadFromEeprom();
void calibrationSaveToEeprom();
void calibrationServiceFSM(uint32_t nowMs, bool button24Low);

// Getters inline rapides
inline uint16_t calibLow(uint8_t m, uint8_t c) {
	return gThLow[m][c];
}
inline uint16_t calibHigh(uint8_t m, uint8_t c) {
	return gThHigh[m][c];
}
inline uint16_t calibRelease(uint8_t m, uint8_t c) {
	// Release est positionné entre High et Low, en s'écartant de High de max(min, pct*|High-Low|)
	int h = (int)gThHigh[m][c];
	int l = (int)gThLow[m][c];
	int D = abs(h - l);
	int relFromPct = (int)(CalibCfg::kReleaseDeltaPct * (float)D);
	int offset = relFromPct;
	if (offset < (int)CalibCfg::kReleaseDeltaMin) offset = (int)CalibCfg::kReleaseDeltaMin;
	int s = (h >= l) ? +1 : -1; // signe de la polarité
	int rel = h - s * offset;
	if (rel < 0) rel = 0;
	if (rel > 1023) rel = 1023;
	return (uint16_t)rel;
}

// Compat (si ancien code appelle encore ces noms globaux)
inline uint16_t getThresholdLow() { return calibLow(0,0); }

