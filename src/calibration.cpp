#include "calibration.h"
#include "config.h"
#include <algorithm>

uint16_t gThLow[N_MUX][N_CH];
uint16_t gThHigh[N_MUX][N_CH];
uint8_t  gHighFastSeen[N_MUX][N_CH];

// === Phase2: histogrammes pour médiane Low ===
static uint16_t *gHist = nullptr; // allocation unique N_MUX*N_CH*1024 entries (lazy)
static uint32_t gCountPerKey[N_MUX][N_CH];
enum class CalibState { STATIC_INIT, COLLECT_LOW, FINALIZE_LOW, RUN };
static CalibState gState = CalibState::STATIC_INIT;
static uint32_t gCollectStartMs = 0;

static inline uint16_t & histAt(uint8_t m, uint8_t c, uint16_t v) {
	// Layout: ((m*N_CH)+c)*1024 + v
	return gHist[ (((size_t)m)*N_CH + c)*1024 + v ];
}

void calibrationInitStatic() {
	gState = CalibState::STATIC_INIT;
	for (uint8_t m=0;m<N_MUX;m++) {
		for (uint8_t c=0;c<N_CH;c++) {
			uint16_t base = 740; // Placeholder avant Phase2 (médiane)
			uint16_t low = (uint16_t)(base + Calib::kLowMarginCounts);
			gThLow[m][c] = low;
			uint16_t high = std::max<uint16_t>((uint16_t)(low + Calib::kMinSwingCounts), Calib::kHighStartDefault);
			gThHigh[m][c] = high;
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
			uint16_t low = (uint16_t)std::min<int>(median + Calib::kLowMarginCounts, 1023);
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

void updateHighAfterNote(uint8_t mux, uint8_t ch, uint16_t peak) {
	if (mux >= N_MUX || ch >= N_CH) return;
	uint16_t low = gThLow[mux][ch];
	if (peak < (uint16_t)(low + Calib::kMinSwingCounts)) return; // frôlement
	int target = (int)peak - (int)Calib::kHighTargetMargin;
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