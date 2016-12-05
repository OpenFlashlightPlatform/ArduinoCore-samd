/*
  Copyright (c) 2016 Michael Andersen <m.andersen@cs.berkeley.edu>

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

// Wait for synchronization of registers between the clock domains
static __inline__ void syncTC_8(Tc* TCx) __attribute__((always_inline, unused));
static void syncTC_8(Tc* TCx) {
  while (TCx->COUNT8.STATUS.bit.SYNCBUSY);
}

// Wait for synchronization of registers between the clock domains
static __inline__ void syncTCC(Tcc* TCCx) __attribute__((always_inline, unused));
static void syncTCC(Tcc* TCCx) {
  while (TCCx->SYNCBUSY.reg & TCC_SYNCBUSY_MASK);
}

void skookumPWM_init(uint32_t pin)
{
  PinDescription pinDesc = g_APinDescription[pin];
  uint32_t attr = pinDesc.ulPinAttribute;
  if ((attr & PIN_ATTR_SKOOKUM_PWM) != PIN_ATTR_SKOOKUM_PWM)
  {
    return;
  }
  uint32_t tcNum = GetTCNumber(pinDesc.ulPWMChannel);
  uint8_t tcChannel = GetTCChannelNumber(pinDesc.ulPWMChannel);
  pinPeripheral(pin, PIO_TIMER);
  uint16_t GCLK_CLKCTRL_IDs[] = {
    GCLK_CLKCTRL_ID(GCM_TCC0_TCC1), // TCC0
    GCLK_CLKCTRL_ID(GCM_TCC0_TCC1), // TCC1
    GCLK_CLKCTRL_ID(GCM_TCC2_TC3),  // TCC2
    GCLK_CLKCTRL_ID(GCM_TCC2_TC3),  // TC3
  };
  GCLK->CLKCTRL.reg = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_IDs[tcNum]);
  while (GCLK->STATUS.bit.SYNCBUSY == 1);
  // -- Configure TCC
  Tcc* TCCx = (Tcc*) GetTC(pinDesc.ulPWMChannel);
  // Disable TCCx
  TCCx->CTRLA.bit.ENABLE = 0;
  syncTCC(TCCx);
  // Set prescaler to 1
  TCCx->CTRLA.reg |= TCC_CTRLA_PRESCALER_DIV1;
  syncTCC(TCCx);
  // Set TCx as normal PWM
  TCCx->WAVE.reg |= TCC_WAVE_WAVEGEN_NPWM;
  syncTCC(TCCx);
  // Set the initial value
  TCCx->CC[tcChannel].reg = 0;
  syncTCC(TCCx);
  // Set PER to maximum counter value (resolution : 0xFF)
  TCCx->PER.reg = 0xFF;
  syncTCC(TCCx);
  // Enable TCCx
  TCCx->CTRLA.bit.ENABLE = 1;
  syncTCC(TCCx);
}

void skookumPWM_update(uint32_t pin, uint32_t high, uint32_t period)
{
  PinDescription pinDesc = g_APinDescription[pin];

  uint8_t tcChannel = GetTCChannelNumber(pinDesc.ulPWMChannel);
  Tcc* TCCx = (Tcc*) GetTC(pinDesc.ulPWMChannel);
  TCCx->CTRLBSET.bit.LUPD = 1;
  syncTCC(TCCx);

  //Update the CC and PER buffer registers
  TCCx->CCB[tcChannel].reg = high;
  syncTCC(TCCx);
  TCCx->PERB.reg = period;
  syncTCC(TCCx);

  //allow the buffer registers to propogate
  TCCx->CTRLBCLR.bit.LUPD = 1;
  syncTCC(TCCx);
  return;
}

#ifdef __cplusplus
}
#endif
