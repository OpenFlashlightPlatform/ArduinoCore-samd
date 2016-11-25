/*
  Copyright (c) 2015 Arduino LLC.  All right reserved.
  Copyright (c) 2015 Atmel Corporation/Thibaut VIARD.  All right reserved.
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

#include <sam.h>
#include "board_definitions.h"


#define NVMCTRL_OTP4   (0x00806020U)
#define SYSCTRL_FUSES_OSC32KCAL_ADDR   (NVMCTRL_OTP4 + 4)
#define SYSCTRL_FUSES_OSC32KCAL_Msk   (0x7Fu << SYSCTRL_FUSES_OSC32KCAL_Pos)
#define SYSCTRL_FUSES_OSC32KCAL_Pos   6

/**
 * \brief system_init() configures the needed clocks and according Flash Read Wait States.
 * At reset:
 * - OSC8M clock source is enabled with a divider by 8 (1MHz).
 * - Generic Clock Generator 0 (GCLKMAIN) is using OSC8M as source.
 * We need to:
 * 1) Enable XOSC32K clock (External on-board 32.768Hz oscillator), will be used as DFLL48M reference.
 * 2) Put XOSC32K as source of Generic Clock Generator 1
 * 3) Put Generic Clock Generator 1 as source for Generic Clock Multiplexer 0 (DFLL48M reference)
 * 4) Enable DFLL48M clock
 * 5) Switch Generic Clock Generator 0 to DFLL48M. CPU will run at 48MHz.
 * 6) Modify PRESCaler value of OSCM to have 8MHz
 * 7) Put OSC8M as source for Generic Clock Generator 3
 */
// Constants for Clock generators
#define GENERIC_CLOCK_GENERATOR_MAIN      (0u)
#define GENERIC_CLOCK_GENERATOR_XOSC32K   (1u)
#define GENERIC_CLOCK_GENERATOR_OSCULP32K (2u) /* Initialized at reset for WDT */
#define GENERIC_CLOCK_GENERATOR_OSC8M     (3u)
// Constants for Clock multiplexers
#define GENERIC_CLOCK_MULTIPLEXER_DFLL48M (0u)

/* The main difference between this function and the one in the arduino code
 * is that we source 32khz from OSCULP, cos we aint got no external xtal
 * and I want to have an easy time going to deep sleep later
 */
void board_init(void)
{

  /* enable clocks for the power, sysctrl and gclk modules */
  PM->APBAMASK.reg = (PM_APBAMASK_PM | PM_APBAMASK_SYSCTRL |
                      PM_APBAMASK_GCLK);

  uint32_t calib_val = ((*((uint32_t *) SYSCTRL_FUSES_OSC32KCAL_ADDR)) & SYSCTRL_FUSES_OSC32KCAL_Msk) >> SYSCTRL_FUSES_OSC32KCAL_Pos;

  SYSCTRL->OSC32K.bit.CALIB = calib_val;
  SYSCTRL->OSC32K.bit.STARTUP = 6;
  SYSCTRL->OSC32K.bit.ONDEMAND = 0;
  SYSCTRL->OSC32K.bit.RUNSTDBY = 1;
  SYSCTRL->OSC32K.bit.EN32K = 1;
  SYSCTRL->OSC32K.bit.ENABLE = 1;

  /* Set 1 Flash Wait State for 48MHz, cf tables 20.9 and 35.27 in SAMD21 Datasheet */
  NVMCTRL->CTRLB.bit.RWS = NVMCTRL_CTRLB_RWS_HALF_Val;

  /* reset the GCLK module so it is in a known state */
  GCLK->CTRL.reg = GCLK_CTRL_SWRST;
  while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) {}

  /* Setup OSC32K to feed Clock generator 1 with 32.768kHz */
  GCLK->GENDIV.reg  = (GCLK_GENDIV_ID(1)   | GCLK_GENDIV_DIV(0));
  GCLK->GENCTRL.reg = (GCLK_GENCTRL_ID(1)  | GCLK_GENCTRL_GENEN |
                       GCLK_GENCTRL_SRC_OSC32K);

  /* Setup Clock generator 1 to feed DFLL48M with 32.768kHz */
  GCLK->CLKCTRL.reg = (GCLK_CLKCTRL_GEN(1) | GCLK_CLKCTRL_CLKEN |
                       GCLK_CLKCTRL_ID(GCLK_CLKCTRL_ID_DFLL48_Val));

  /* Wait for sync */
  while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) {}

  /* Enable DFLL48M */
  SYSCTRL->DFLLCTRL.bit.ONDEMAND = 0; // Sleep in STANDBY mode
  SYSCTRL->DFLLCTRL.bit.RUNSTDBY = 0; // Sleep in STANDBY mode

  SYSCTRL->DFLLCTRL.bit.MODE   = 1;     // Closed loop mode
  SYSCTRL->DFLLCTRL.bit.QLDIS  = 1;     // Quick lock is enabled
  SYSCTRL->DFLLCTRL.bit.WAITLOCK = 1;
  SYSCTRL->DFLLCTRL.bit.CCDIS  = 0;	  // Chill cycle is enabled
  SYSCTRL->DFLLCTRL.bit.LLAW   = 0;     // Locks will not be lost after waking up from sleep modes
  SYSCTRL->DFLLMUL.bit.CSTEP   = 0x1f/4;
  SYSCTRL->DFLLMUL.bit.FSTEP   = 0xff/4;
//  SYSCTRL->DFLLCTRL.bit.USBCRM = 1;
//  SYSCTRL->DFLLMUL.bit.MUL = 48000;
  SYSCTRL->DFLLMUL.bit.MUL     = 1465; //For 48 Mhz from 32768
  SYSCTRL->DFLLCTRL.bit.ENABLE = 1;     // Enable DFLL

  /* Wait for DFLL */
  while ( (SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLLCKC) == 0 ||
          (SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLLCKF) == 0 ) {}
  while (!(SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY)) {}


  /* Setup DFLL48M to feed Clock generator 0 (CPU core clock) */
  GCLK->GENDIV.reg  = (GCLK_GENDIV_ID(0)  | GCLK_GENDIV_DIV(0)); //CLOCK_PLL_DIV) |
  GCLK->GENCTRL.reg = (GCLK_GENCTRL_ID(0) | GCLK_GENCTRL_GENEN |
                       GCLK_GENCTRL_SRC_DFLL48M);

  /* make sure we synchronize clock generator 0 before we go on */
  while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) {}

  /*
   * Now that all system clocks are configured, we can set CPU and APBx BUS clocks.
   * These values are normally the ones present after Reset.
   */
  PM->CPUSEL.reg  = PM_CPUSEL_CPUDIV_DIV1;
  PM->APBASEL.reg = PM_APBASEL_APBADIV_DIV1_Val;
  PM->APBBSEL.reg = PM_APBBSEL_APBBDIV_DIV1_Val;
  PM->APBCSEL.reg = PM_APBCSEL_APBCDIV_DIV1_Val;
}

//If we know we are in bootloader mode, enable USB clock recovery
//on the DFLL to compensate for the shoddy accuracy of the OSCULP
void enable_clock_recovery()
{
  //Enable clock recovery mode. Our clock will initially be slow if it
  //switches from the 32khz clock to the 1khz SOF
  SYSCTRL->DFLLCTRL.bit.USBCRM = 1;

  //Lets wait for it to lock, so we don't overclock during the transition
  while (!(SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY)) {}

  //We then adust our multiplier to bring us back up to 48Mhz
  SYSCTRL->DFLLMUL.bit.MUL = 48000;

  //Lets wait for it to lock, so we don't overclock during the transition
  while (!(SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY)) {}

  //The DFLL should be stable at 48Mhz from a recovered USB clock now
}
