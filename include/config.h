#pragma once
// Configuration for JANKO2 8-MUX dual-ADC controller

#include <Arduino.h>

// === ADC Synchronized Read Order Selector ===
// 0 = startSynchronizedSingleRead(ADC0, ADC1)
// 1 = startSynchronizedSingleRead(ADC1, ADC0)
#ifndef ADC_SYNC_ORDER
#define ADC_SYNC_ORDER 0
#endif

// Temporary debug: print per synchronized pair results
#ifndef DEBUG_PRINT_PAIRS
#define DEBUG_PRINT_PAIRS 1
#endif

// === Hardware Configuration ===
// 8 MUX configuration with 2 ADC groups for true parallel scanning
//
// Wiring Recap (Source of Truth) — Teensy 4.1
// Group A (ADC1, MUX0..MUX3):
//   Select Lines (shared by MUX0..3): S0=35 (LSB), S1=34, S2=36, S3=37 (MSB)
//   COM Outputs → ADC1 inputs:
//     MUX0 COM → 41 (A22 / ADC1)
//     MUX1 COM → 40 (A21 / ADC1)
//     MUX2 COM → 39 (A20 / ADC1)
//     MUX3 COM → 38 (A19 / ADC1)
// Group B (ADC0, MUX4..MUX7):
//   Select Lines (shared by MUX4..7): S0=16 (LSB), S1=17, S2=15, S3=14 (MSB)
//   COM Outputs → ADC0 inputs:
//     MUX4 COM → 20 (A6  / ADC0)
//     MUX5 COM → 21 (A7  / ADC0)
//     MUX6 COM → 22 (A8  / ADC0)
//     MUX7 COM → 23 (A9  / ADC0)
// Channel index (0..15) encoded as (S3 S2 S1 S0) with S0 = LSB for BOTH groups.
// IMPORTANT: Use only MUX_ADC_PINS[] inside scanning (no PAIR* arrays) to avoid logic errors.
static constexpr int N_MUX = 8;        // Total number of multiplexers 
static constexpr int N_CH = 16;        // Channels per multiplexer
static constexpr uint16_t kTotalKeys = N_MUX * N_CH; // 128 keys total

// ADC pins for the 8 MUX outputs - optimized for Teensy 4.1 dual ADC
// Group A (ADC1): MUX 0-3 
// Group B (ADC0): MUX 4-7
static constexpr int MUX_ADC_PINS[N_MUX] = {
    // Group A - ADC1 (pins 38-41)
    41,  // MUX0 → pin 41 (ADC1_0 / A22)
    40,  // MUX1 → pin 40 (ADC1_1 / A21) 
    39,  // MUX2 → pin 39 (ADC1_2 / A20)
    38,  // MUX3 → pin 38 (ADC1_3 / A19)
    // Group B - ADC0 (pins 20-23)
    20,  // MUX4 → pin 20 (ADC0_6 / A6)
    21,  // MUX5 → pin 21 (ADC0_7 / A7)
    22,  // MUX6 → pin 22 (ADC0_8 / A8) 
    23   // MUX7 → pin 23 (ADC0_9 / A9)
};

// Synchronized ADC pairs for parallel reading (Target 2 optimization)
// (Legacy helper arrays kept only for potential diagnostics; NOT to be used in scanChannelDualADC)
static constexpr int PAIR_ADC1_PINS[4] = {41, 40, 39, 38}; // DO NOT USE in scanning logic
static constexpr int PAIR_ADC0_PINS[4] = {20, 21, 22, 23}; // DO NOT USE in scanning logic

// Group configuration for dual ADC scanning
static constexpr int N_GROUPS = 2;
static constexpr int MUX_PER_GROUP = 4;

// CD74HC4067 MUX control pins - separate for each group
// Group A (MUX 0-3) - S0-S3 lines
static constexpr uint8_t kMuxGroupA_S0 = 35; // S0 for group A
static constexpr uint8_t kMuxGroupA_S1 = 34; // S1 for group A  
static constexpr uint8_t kMuxGroupA_S2 = 36; // S2 for group A
static constexpr uint8_t kMuxGroupA_S3 = 37; // S3 for group A

// Group B (MUX 4-7) - S0-S3 lines  
static constexpr uint8_t kMuxGroupB_S0 = 16; // S0 for group B
static constexpr uint8_t kMuxGroupB_S1 = 17; // S1 for group B
static constexpr uint8_t kMuxGroupB_S2 = 15; // S2 for group B
static constexpr uint8_t kMuxGroupB_S3 = 14; // S3 for group B

// Safety check: ensure array size matches N_MUX
static_assert(N_MUX == sizeof(MUX_ADC_PINS)/sizeof(MUX_ADC_PINS[0]), "MUX_ADC_PINS array size must match N_MUX");

// ADC Configuration
static constexpr uint8_t kAdcResolution = 10; // 10-bit ADC (0-1023)

// MIDI Configuration
static constexpr uint8_t kMidiChannel = 1; // MIDI channel (1-16)

// Scanning Configuration - Target 2 synchronized pairs optimization
// Base interval entre deux channels (si kContinuousScan == false). Peut descendre à 0.
static constexpr uint32_t kScanIntervalMicros = 0;   // 1µs par channel (peut être mis à 0)
// Délai après changement des lignes d'adresse MUX avant le premier read synchronisé.
// Mettre à 0 pour tester la limite physique (le 4067 ajoute typiquement ~100ns tSW + source impedance).
static constexpr uint32_t kSettleMicros = 0;         // 1µs actuel; essayer 0 si pas de "cloned readings"
// Délai optionnel entre chaque paire synchronisée (4 paires * 2 ADC). 1 supprimait des duplications.
// Mettre à 0 pour vitesse max; si valeurs dupliquées réapparaissent, revenir à 1.
static constexpr uint32_t kPerPairDelayMicros = 0;   // Délai en microsecondes (grossier). Si 0 on peut utiliser kPerPairDelayCycles.
// Délai ultra-fin en CYCLES CPU (Teensy 4.1 @600 MHz ⇒ 1 µs ≈ 600 cycles). Permet <1 µs.
// Utilisé uniquement si kPerPairDelayMicros == 0 et kPerPairDelayCycles > 0.
static constexpr uint32_t kPerPairDelayCycles = 150;   // Exemple: 120 (~0.2 µs), 300 (~0.5 µs), 600 (~1.0 µs)
// Mode scan continu : ignore kScanIntervalMicros et enchaîne les channels sans attente active.
static constexpr bool kContinuousScan = true;       // Passer à true pour pousser au maximum
// Objectif théorique de fréquence frawme (informative seulement)
static constexpr uint32_t kFrameTargetHz = 3125;     // 3.125kHz cible (maj selon nouveaux réglages)
// === Détection duplications de paires (valeurs clonées entre MUX0..3 et MUX4..7) ===
#ifndef DEBUG_DUPLICATE_DETECT
#define DEBUG_DUPLICATE_DETECT 0   // 1=active détection & stats
#endif
#ifndef DEBUG_DUPLICATE_PRINT_INTERVAL_FRAMES
#define DEBUG_DUPLICATE_PRINT_INTERVAL_FRAMES 1000  // Affiche toutes les X frames
#endif
// Tolérance (LSB) entre valeurs[i] et valeurs[4+i] pour considérer « identique » (ADC bruit)
static constexpr uint16_t kDuplicateTolerance = 1;
// === Profiling (mesures internes) ===
#ifndef DEBUG_PROFILE_SCAN
#define DEBUG_PROFILE_SCAN 0   // 1=active mesures temps channel/frame
#endif
#ifndef DEBUG_PROFILE_INTERVAL_FRAMES
#define DEBUG_PROFILE_INTERVAL_FRAMES 200  // Regroupe sur 200 frames avant impression
#endif
// === Frame rate debug ===
#ifndef DEBUG_FRAME_RATE
#define DEBUG_FRAME_RATE 1          // 1=imprime la fréquence réelle (frames/s)
#endif
#ifndef DEBUG_FRAME_RATE_INTERVAL_MS
#define DEBUG_FRAME_RATE_INTERVAL_MS 1000  // Période d'affichage
#endif
#ifndef DEBUG_FRAME_RATE_TOGGLE_LED
#define DEBUG_FRAME_RATE_TOGGLE_LED 0  // 1 = toggle LED à chaque frame (diagnostic scope)
#endif
// === Debug Options ===
// Freeze scanning to a fixed logical channel (0..15). Set to -1 for normal operation.
// Set to 6 to freeze scanning on logical channel 6 for debug logging
#ifndef DEBUG_FREEZE_CHANNEL
#define DEBUG_FREEZE_CHANNEL -1  // Reset to -1 to scan all channels
#endif

// Enable printing of per-channel values after each scan if non-zero.
// Enable verbose per-scan value printing
#ifndef DEBUG_PRINT_VALUES
#define DEBUG_PRINT_VALUES 1
#endif

// === ADC Monitor (optional) ===
// Active si DEBUG_ADC_MONITOR = 1 : le firmware envoie toutes les DEBUG_ADC_MONITOR_INTERVAL_MS
// la dernière valeur ADC brute (0..1023) pour le MUX et la channel ciblés.
// Configuration :
//   DEBUG_ADC_MONITOR_MUX    : index MUX (0..7)
//   DEBUG_ADC_MONITOR_CHANNEL: index channel (0..15)
//   DEBUG_ADC_MONITOR_INTERVAL_MS : période d'impression
#ifndef DEBUG_ADC_MONITOR
#define DEBUG_ADC_MONITOR 1
#endif
#ifndef DEBUG_ADC_MONITOR_MUX
#define DEBUG_ADC_MONITOR_MUX 3
#endif
#ifndef DEBUG_ADC_MONITOR_CHANNEL
#define DEBUG_ADC_MONITOR_CHANNEL 0
#endif
#ifndef DEBUG_ADC_MONITOR_INTERVAL_MS
#define DEBUG_ADC_MONITOR_INTERVAL_MS 1
#endif

// === Velocity curve shaping ===
// Exposant appliqué à la valeur normalisée (0..1) avant mapping 1..127.
// <1.0 rend les faibles vitesses plus sensibles (valeurs de vélocité plus élevées plus tôt)
// >1.0 compresse le bas et étire le haut.
static constexpr float kVelocityGamma = 0.20f; // Option A proposée

// === Re-press detection (ThresholdMed) ===
// Quand une touche est relâchée (sous ThresholdRelease) mais ne revient pas jusqu'à ThresholdLow,
// on tracke la "vallée" (minimum) et, si la touche repart, on démarre le timing depuis cette vallée.
// Hystérésis pour considérer la remontée depuis la vallée (en LSB ADC)
static constexpr uint16_t kRepressHyst = 3;
// Nombre d'échantillons consécutifs au-dessus de (vallée + hystérésis) pour valider la remontée
static constexpr uint8_t  kRepressStableCount = 1; // passer à 2 si besoin de plus d'anti-rebond

// === Calibration relative (pourcentages globaux) ===
// Ces constantes pilotent l'adaptation des seuils par touche à partir des valeurs brutes Low/High.
// Elles remplacent les marges absolues et s'appliquent en proportion de D = |High - Low|.
namespace CalibCfg {
    // Low opérationnel: LowOp = Low ± max(Min, Pct * D)
    constexpr float    kLowMarginPct        = 0.10f;  // 10%
    constexpr uint16_t kLowMarginMinCounts  = 10;     // plancher en counts

    // High opérationnel: HighOp = High ∓ max(Min, Pct * D)
    constexpr float    kHighTargetMarginPct = 0.15f;  // 15%
    constexpr uint16_t kHighTargetMarginMin = 10;

    // Release: Release = High ∓ max(Min, Pct * D)
    constexpr float    kReleaseDeltaPct     = 0.15f;  // 15%
    constexpr uint16_t kReleaseDeltaMin     = 10;
}

