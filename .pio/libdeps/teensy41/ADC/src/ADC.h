/*
ADC.h - Arduino library for get fastly ADC value on AVR,s devices.
v0.1.2

Copyright © 2022 Francisco Rafael Reyes Carmona.
All rights reserved.

rafael.reyes.carmona@gmail.com


  This file is part of ADC Library.

  ADC Library is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  ADC Library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with ADC Library.  If not, see <https://www.gnu.org/licenses/>.

*/

#if ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#ifndef ADC_h
#define ADC_h

#if defined(__AVR__)

#include <math.h>
#include <avr/sleep.h>

extern uint16_t adcRead_(void);
extern uint16_t adcGet_(void);

#endif

#endif