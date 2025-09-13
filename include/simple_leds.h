#pragma once
#include <Arduino.h>

// Initialise la LED simple (pin 12) et le strip (pin 10)
void simpleLedsInit();

// Met à jour les LEDs selon l'état du pin 3
void simpleLedsTask();
