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

#include "ADC.h"

#if defined(__AVR__)

uint16_t adcRead_(void){
  ADCSRA |= _BV(ADSC);
  while (bit_is_set(ADCSRA, ADSC));
  return ADC;
}

uint16_t adcGet_(void){
  uint16_t pVal;

#if defined(__LGT8FX8P__)
  uint16_t nVal;
  ADCSRC |=  _BV(SPN);
  nVal = adcRead_();
  ADCSRC &= ~_BV(SPN);
#elif defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__) || defined (__AVR_ATmega328__)
  ADCSRA |= _BV(ADIE); //Setting the ADIE bit to 1 means when the ADC is done a measurement it generates an interrupt, the interrupt will wake the chip up from low noise sleep
  ADCSRA |= _BV(ADEN); //Turn the ADC on by setting the ADEN bit to 1 in the ADCSRA register
  sleep_enable();
  set_sleep_mode(SLEEP_MODE_ADC);
  sei();
  sleep_cpu();

  pVal = ADC;

  ADCSRA &= ~(_BV(ADIE));
  sleep_disable();
#endif

#if defined(__LGT8F__)  
  pVal = adcRead_();
#endif

#if defined(__LGT8FX8P__)
  pVal = (pVal + nVal) >> 1;
#endif

// Logicgreen gain-error correction
#if defined(__LGT8FX8E__)
  pVal -= (pVal >> 5);
#elif defined(__LGT8FX8P__)
  pVal -= (pVal >> 7);
#endif
  
  return pVal;
}

#endif