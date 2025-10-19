#include "calibration.h"
#include "config.h"
#include "simple_leds.h"
#include "eeprom_store.h"
#include "key_state.h" // for g_acquisition
#include <algorithm>

uint16_t gThLow[N_MUX][N_CH];
uint16_t gThHigh[N_MUX][N_CH];
uint8_t  gHighFastSeen[N_MUX][N_CH];

// === Phase2: histogrammes pour médiane Low ===
static uint16_t *gHist = nullptr; // allocation unique N_MUX*N_CH*1024 entries (lazy)
static uint32_t gCountPerKey[N_MUX][N_CH];
static bool gKeyPressedInPhase2[N_MUX][N_CH]; // Track which keys were pressed during Phase 2
enum class CalibState { STATIC_INIT, COLLECT_LOW, FINALIZE_LOW, RUN };
static CalibState gState = CalibState::STATIC_INIT;
static uint32_t gCollectStartMs = 0;

static inline uint16_t & histAt(uint8_t m, uint8_t c, uint16_t v) {
	// Layout: ((m*N_CH)+c)*1024 + v
	return gHist[ (((size_t)m)*N_CH + c)*1024 + v ];
}

void calibrationInitStatic() {
	gState = CalibState::STATIC_INIT;
	// Try load from EEPROM first (gamma loaded separately in main.cpp)
	uint16_t lowTmp[N_MUX][N_CH];
	uint16_t highTmp[N_MUX][N_CH];
	bool loaded = EepromStore::load(lowTmp, highTmp, nullptr);
	for (uint8_t m=0;m<N_MUX;m++) {
		for (uint8_t c=0;c<N_CH;c++) {
			if (loaded) {
				gThLow[m][c] = lowTmp[m][c];
				gThHigh[m][c] = highTmp[m][c];
			} else {
				uint16_t base = 740; // Placeholder avant Phase2 (médiane)
				// Low opérationnel = base ± max(min, pct*D) mais ici D inconnu -> on applique seulement le plancher
				uint16_t low = (uint16_t)std::min<int>(base + CalibCfg::kLowMarginMinCounts, 1023);
				gThLow[m][c] = low;
				uint16_t high = std::max<uint16_t>((uint16_t)(low + Calib::kMinSwingCounts), Calib::kHighStartDefault);
				gThHigh[m][c] = high;
			}
			gHighFastSeen[m][c] = 0;
			gCountPerKey[m][c] = 0;
		}
	}
}

// Démarre la collecte médiane (appeler depuis setup après init statique)
void calibrationStartCollectLow() {
	if (!gHist) {
		size_t totalEntries = (size_t)N_MUX * (size_t)N_CH * 1024;
		gHist = (uint16_t*)malloc(totalEntries * sizeof(uint16_t));
		if (gHist) memset(gHist, 0, totalEntries * sizeof(uint16_t));
	} else {
		// reset
		size_t totalEntries = (size_t)N_MUX * (size_t)N_CH * 1024;
		memset(gHist, 0, totalEntries * sizeof(uint16_t));
	}
	for (uint8_t m=0;m<N_MUX;m++) for (uint8_t c=0;c<N_CH;c++) gCountPerKey[m][c]=0;
	gCollectStartMs = millis();
	gState = CalibState::COLLECT_LOW;
}

// À appeler chaque frame (après swapBuffers) tant que collecte active
void calibrationFrameIngest(const uint16_t frameValues[N_MUX][N_CH]) {
	if (gState != CalibState::COLLECT_LOW || !gHist) return;
	// Incrémenter histogramme par touche
	for (uint8_t m=0;m<N_MUX;m++) {
		for (uint8_t c=0;c<N_CH;c++) {
			uint16_t v = frameValues[m][c];
			if (v > 1023) v = 1023;
			histAt(m,c,v)++;
			gCountPerKey[m][c]++;
		}
	}
	// Vérifier fenêtre temps
	uint32_t nowMs = millis();
	if (nowMs - gCollectStartMs >= Calib::kMedianWindowMs) {
		gState = CalibState::FINALIZE_LOW;
	}
}

static void finalizeMedian() {
	if (!gHist) { gState = CalibState::RUN; return; }
	for (uint8_t m=0;m<N_MUX;m++) {
		for (uint8_t c=0;c<N_CH;c++) {
			uint32_t total = gCountPerKey[m][c];
			if (total == 0) continue; // garde valeurs existantes
			uint32_t half = total / 2;
			uint32_t accum = 0;
			uint16_t median = 0;
			for (uint16_t v=0; v<1024; ++v) {
				accum += histAt(m,c,v);
				if (accum >= half) { median = v; break; }
			}
			// Sans High fiable on pose Low = median + plancher (pct*D inconnu ici)
			uint16_t low = (uint16_t)std::min<int>(median + CalibCfg::kLowMarginMinCounts, 1023);
			gThLow[m][c] = low;
			// Ajuster High si inférieur aux contraintes
			if (gThHigh[m][c] < (uint16_t)(low + Calib::kMinSwingCounts)) {
				gThHigh[m][c] = (uint16_t)std::min<int>(low + Calib::kMinSwingCounts, 1023);
			}
		}
	}
	// Option: libérer mémoire (on libère pour récupérer RAM)
	free(gHist); gHist = nullptr;
	gState = CalibState::RUN;
}

void calibrationService() {
	if (gState == CalibState::FINALIZE_LOW) {
		finalizeMedian();
	}
}

bool calibrationIsCollecting() { return gState == CalibState::COLLECT_LOW; }
bool calibrationIsRunning() { return gState == CalibState::RUN; }

// === EEPROM wrappers ===
void calibrationLoadFromEeprom() {
	uint16_t lowTmp[N_MUX][N_CH];
	uint16_t highTmp[N_MUX][N_CH];
	if (EepromStore::load(lowTmp, highTmp, nullptr)) {
		for (uint8_t m=0;m<N_MUX;m++) for (uint8_t c=0;c<N_CH;c++) {
			gThLow[m][c] = lowTmp[m][c];
			gThHigh[m][c] = highTmp[m][c];
		}
	}
}
void calibrationSaveToEeprom() {
	// Save thresholds only; gamma is managed separately in main.cpp
	EepromStore::save(gThLow, gThHigh, nullptr);
}

// === Calibration FSM driven by button 24 (LOW when pressed) ===
enum class UX { Idle, HoldStart, Phase1, Phase2, HoldEnd };
static UX gUx = UX::Idle;
static uint32_t gUxT0 = 0;
static uint16_t gPhase2Peak[N_MUX][N_CH];

void calibrationServiceFSM(uint32_t nowMs, bool button24Low) {
	switch (gUx) {
		case UX::Idle:
			if (button24Low) {
				gUx = UX::HoldStart; gUxT0 = nowMs;
			}
			break;
		case UX::HoldStart:
			if (!button24Low) { gUx = UX::Idle; break; }
			if (nowMs - gUxT0 >= Calib::kHoldToStartMs) {
				// Start Phase1: collect median Low
				calibrationStartCollectLow();
				simpleLedsSetBrightness(120); // slight bump
				// Blink red at 5 Hz by toggling calibration LEDs each service call based on time
				gUx = UX::Phase1; gUxT0 = nowMs;
			}
			break;
		case UX::Phase1: {
			// Blink red 5 Hz
			bool on = ((nowMs / 100) % 2) == 0;
			setCalibrationLeds(on);
			// Ensure blue override is off during Phase1
			simpleLedsSetCalibrationBlue(false);
			if (!button24Low) {
				// Wait until median window auto-finalizes in calibrationService()
				if (!calibrationIsCollecting()) {
					// Move to Phase2: reset peaks and pressed-key tracking
					for (uint8_t m=0;m<N_MUX;m++) {
						for (uint8_t c=0;c<N_CH;c++) {
							gPhase2Peak[m][c] = gThLow[m][c];
							gKeyPressedInPhase2[m][c] = false; // Reset tracking
						}
					}
					// Enable solid blue override on all LEDs during Phase 2
					setCalibrationLeds(false);
					simpleLedsSetCalibrationBlue(true);
					gUx = UX::Phase2; gUxT0 = nowMs;
				}
			}
			break; }
		case UX::Phase2: {
			// During Phase2, update peaks from current workingValues and mark pressed keys
			for (uint8_t m=0;m<N_MUX;m++) {
				for (uint8_t c=0;c<N_CH;c++) {
					uint16_t low = gThLow[m][c];
					uint16_t v = g_acquisition.workingValues[m][c];
					int dv = (int)v - (int)low;
					int bestDv = (int)gPhase2Peak[m][c] - (int)low;
					if (abs(dv) > abs(bestDv)) {
						gPhase2Peak[m][c] = v;
					}
					// Mark key as pressed if swing exceeds minimum threshold
					if (abs(dv) >= (int)Calib::kMinSwingCounts) {
						gKeyPressedInPhase2[m][c] = true;
					}
				}
			}
			if (button24Low) {
				gUx = UX::HoldEnd; gUxT0 = nowMs;
			}
			break; }
		case UX::HoldEnd:
			if (!button24Low) { gUx = UX::Phase2; break; }
			if (nowMs - gUxT0 >= Calib::kHoldToFinishMs) {
				// Finalize: update Low for ALL keys, but High ONLY for pressed keys
				for (uint8_t m=0;m<N_MUX;m++) {
					for (uint8_t c=0;c<N_CH;c++) {
						uint16_t lowPrev = gThLow[m][c];
						uint16_t peak = gPhase2Peak[m][c];
						int s = ((int)peak >= (int)lowPrev) ? +1 : -1; // press direction
						int D = abs((int)peak - (int)lowPrev);
						
						// ALWAYS recompute Low using polarity and percent margin
						int median_est = (int)lowPrev - (int)CalibCfg::kLowMarginMinCounts;
						int lowMargin = std::max<int>((int)CalibCfg::kLowMarginMinCounts, (int)(CalibCfg::kLowMarginPct * (float)D));
						int lowNew = median_est + s * lowMargin;
						if (lowNew < 0) lowNew = 0; if (lowNew > 1023) lowNew = 1023;
						gThLow[m][c] = (uint16_t)lowNew;

						// Update High ONLY if key was pressed during Phase 2
						if (gKeyPressedInPhase2[m][c]) {
							if (D < (int)Calib::kMinSwingForHigh) {
								// Too small swing: enforce minimum around Low in the press direction
								int target = lowNew + s * (int)Calib::kMinSwingCounts;
								if (target < 0) target = 0; if (target > 1023) target = 1023;
								gThHigh[m][c] = (uint16_t)target;
							} else {
								int margin = std::max<int>((int)CalibCfg::kHighTargetMarginMin, (int)(CalibCfg::kHighTargetMarginPct * (float)D));
								int target = (int)peak - s * margin;
								// Enforce minimal swing symmetrical to polarity
								if (abs(target - lowNew) < (int)Calib::kMinSwingCounts) target = lowNew + s * (int)Calib::kMinSwingCounts;
								if (target < 0) target = 0; if (target > 1023) target = 1023;
								gThHigh[m][c] = (uint16_t)target;
							}
						}
						// If key was NOT pressed, gThHigh[m][c] keeps its previous value from EEPROM
					}
				}
				
				// Count and report how many keys were calibrated
#ifdef DEBUG_GAMMA_MONITOR
				uint16_t calibratedCount = 0;
				for (uint8_t m=0;m<N_MUX;m++) {
					for (uint8_t c=0;c<N_CH;c++) {
						if (gKeyPressedInPhase2[m][c]) calibratedCount++;
					}
				}
				Serial.printf("[Calibration] Completed: %u keys calibrated (High updated), all keys Low updated\n", calibratedCount);
#endif
				
				// Save to EEPROM and exit calibration
				calibrationSaveToEeprom();
				setCalibrationLeds(false);
				simpleLedsSetCalibrationBlue(false);
				gUx = UX::Idle;
			}
			break;
	}
}

void updateHighAfterNote(uint8_t mux, uint8_t ch, uint16_t peak) {
	if (mux >= N_MUX || ch >= N_CH) return;
	uint16_t low = gThLow[mux][ch];
	if (peak < (uint16_t)(low + Calib::kMinSwingCounts)) return; // frôlement
	// Cible High op: peak reculé d'une marge relative
	// D provisoire: |peak - low| (on se base sur la frappe courante)
	int D = abs((int)peak - (int)low);
	int marginRel = (int)std::max<int>((int)CalibCfg::kHighTargetMarginMin, (int)(CalibCfg::kHighTargetMarginPct * (float)D));
	int s = ((int)peak >= (int)low) ? +1 : -1;
	int target = (int)peak - s * marginRel;
	if (target < (int)(low + Calib::kMinSwingCounts)) target = low + Calib::kMinSwingCounts;
	if (target > 1023) target = 1023;
	uint16_t oldH = gThHigh[mux][ch];
	float alpha = (gHighFastSeen[mux][ch] < Calib::kHighFastNotes) ? Calib::kHighAlphaFast : Calib::kHighAlpha;
	float blended = alpha * oldH + (1.0f - alpha) * (float)target;
	int newH = (int)blended;
	if (newH < (int)(low + Calib::kMinSwingCounts)) newH = low + Calib::kMinSwingCounts;
	if (newH > 1023) newH = 1023;
	gThHigh[mux][ch] = (uint16_t)newH;
	if (gHighFastSeen[mux][ch] < 255) gHighFastSeen[mux][ch]++;
}