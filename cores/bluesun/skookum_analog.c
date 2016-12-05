/*
  Copyright (c) 2014 Arduino LLC.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "Arduino.h"
#include "wiring_private.h"

#ifdef __cplusplus
extern "C" {
#endif

static int _readResolution = 10;
static int _ADCResolution = 10;
static int _writeResolution = 8;

// Wait for synchronization of registers between the clock domains
static __inline__ void syncADC() __attribute__((always_inline, unused));
static void syncADC() {
  while (ADC->STATUS.bit.SYNCBUSY == 1);
}

void skookumADC_init(uint32_t pin) {
  pinPeripheral(pin, PIO_ANALOG);
  syncADC();
  ADC->CTRLA.bit.ENABLE = 0x01;             // Enable ADC
  syncADC();
  ADC->REFCTRL.bit.REFSEL = ADC_REFCTRL_REFSEL_INT1V_Val;   // 1.0V voltage reference
  syncADC();
  ADC->REFCTRL.bit.REFCOMP = 1; //Increase gain stage accuracy
  syncADC();
  //XTAG can we get acceptable results without having this ludicrously low sample rate?
  ADC->CTRLB.reg = ADC_CTRLB_PRESCALER_DIV512 |    // Divide Clock by 512.
                   ADC_CTRLB_RESSEL_16BIT;         // 10 bits resolution as default

  ADC->SAMPCTRL.reg = 0x3f;                        // Set max Sampling Time Length
  syncADC();
  ADC->INPUTCTRL.reg = ADC_INPUTCTRL_MUXNEG_GND;   // No Negative input (Internal Ground)
  syncADC();
  // Averaging (see datasheet table in AVGCTRL register description)
  ADC->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM_64 |    // 64 samples (16 bits, 4x oversample)
                     ADC_AVGCTRL_ADJRES(0x0ul);   // Adjusting result by 0
  syncADC();
}

uint32_t skookumADC_sampleDifferential(uint32_t neg, uint32_t pos, uint8_t gain)
{
  uint32_t valueRead = 0;
  ADC->INPUTCTRL.bit.GAIN = gain;      // Gain Factor Selection
  syncADC();
  ADC->INPUTCTRL.bit.MUXPOS = g_APinDescription[pos].ulADCChannelNumber; // Selection for the positive ADC input
  syncADC();
  if (neg == SKOOKUM_ADC_GROUND) {
    ADC->INPUTCTRL.bit.MUXNEG = 0x18;
  } else {
    ADC->INPUTCTRL.bit.MUXNEG = g_APinDescription[neg].ulADCChannelNumber; // Selection for the negative ADC input
  }
  // Start conversion
  syncADC();
  ADC->SWTRIG.bit.START = 1;

  syncADC();
  while (ADC->INTFLAG.bit.RESRDY == 0);   // Waiting for conversion to complete

  // Clear the Data Ready flag
  ADC->INTFLAG.bit.RESRDY = 1;

  // Start conversion again, since The first conversion after the reference/channel is changed must not be used.
  syncADC();
  ADC->SWTRIG.bit.START = 1;

  // Store the value
  while (ADC->INTFLAG.bit.RESRDY == 0);   // Waiting for conversion to complete
  valueRead = ADC->RESULT.reg;

  return valueRead;
}

#ifdef __cplusplus
}
#endif
