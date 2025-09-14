#include "calibration.h"
#include <algorithm>

namespace Calib {

uint16_t thLowAuto  [N_MUX][N_CH];
uint16_t thHighAuto [N_MUX][N_CH];
uint16_t thLowManual[N_MUX][N_CH];
uint16_t thHighManual[N_MUX][N_CH];
bool     thManualMask[N_MUX][N_CH];

static inline uint16_t clamp10(int v){ return (uint16_t)std::min(1023, std::max(0, v)); }

void thresholdsInit(uint16_t lowDefault, uint16_t highDefault){
	for(int m=0;m<N_MUX;++m){
		for(int c=0;c<N_CH;++c){
			thLowAuto  [m][c] = clamp10(lowDefault);
			thHighAuto [m][c] = clamp10(std::max<int>(lowDefault + kMinSwingForValidity, highDefault));
			thLowManual[m][c] = thLowAuto[m][c];
			thHighManual[m][c] = thHighAuto[m][c];
			thManualMask[m][c] = false;
		}
	}
}

void setAutoLow (int m,int c,uint16_t v){ thLowAuto [m][c] = clamp10(v); }
void setAutoHigh(int m,int c,uint16_t v){
	v = clamp10(std::max<int>(v, thLowAuto[m][c] + kMinSwingForValidity));
	thHighAuto[m][c] = v;
}

void setManualLow (int m,int c,uint16_t v){ thLowManual [m][c] = clamp10(v); }
void setManualHigh(int m,int c,uint16_t v){
	v = clamp10(std::max<int>(v, thLowManual[m][c] + kMinSwingForValidity));
	thHighManual[m][c] = v;
}
void setManualMask(int m,int c,bool on){ thManualMask[m][c] = on; }

static inline void pick(int m,int c, uint16_t& low, uint16_t& high){
	if (thManualMask[m][c]) { low = thLowManual[m][c];  high = thHighManual[m][c]; }
	else                    { low = thLowAuto  [m][c];  high = thHighAuto  [m][c]; }
	if (high < low + kMinSwingForValidity) high = clamp10(low + kMinSwingForValidity);
}

uint16_t getLow(int m,int c){ uint16_t l,h; pick(m,c,l,h); return l; }
uint16_t getHigh(int m,int c){ uint16_t l,h; pick(m,c,l,h); return h; }
uint16_t getRelease(int m,int c){
	uint16_t h = getHigh(m,c);
	return (h >= kReleaseDelta) ? (h - kReleaseDelta) : 0;
}

} // namespace Calib