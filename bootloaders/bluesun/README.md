# Blue Sun OFP Bootloader

## 0- Explanation

This is basically the same as the Arduino bootloader which is basically the
same as Atmel's SAM-BA bootloader but:

- I ripped out all the LED stuff
- I ripped out all the UART stuff
- I changed the enter-bootloader trigger to pin not double-reset
- I changed the pin to GP3 on the blue sun board

## 1- Prerequisites

The project build is based on Makefile system.
Makefile is present at project root and try to handle multi-platform cases. (Blue Sun note: I may have broken windows/OSX support, meh)

Multi-plaform GCC is provided by ARM here: https://launchpad.net/gcc-arm-embedded/+download

Atmel Studio contains both make and ARM GCC toolchain. You don't need to install them in this specific use case.

For all builds and platforms you will need to have the Arduino IDE installed and the board support
package for "Arduino SAMD Boards (32-bits ARM Cortex-M0+)". You can install the latter
from the former's "Boards Manager" UI.

### Windows

* Native command line
Make binary can be obtained here: http://gnuwin32.sourceforge.net/packages/make.htm

* Cygwin/MSys/MSys2/Babun/etc...
It is available natively in all distributions.

### Linux

Make is usually available by default.

### OS X

Make is available through XCode package.


## 2- Selecting available SAM-BA interfaces

Only the USB CDC interface is available on the Blue Sun board.

## 3- Behaviour

This version of the bootloader requires you to short PA12 (GP3) on the blue sun board to ground to enter the bootloader. There is no reset button and the double tap reset thing on Arduino Zeros won't work here.

## 4- Description

**Pinmap**

The following pins are used by the program :
PA25 : input/output (USB DP)
PA24 : input/output (USB DM)
PA17 : GP3 enter bootloader


**Clock system**

CPU runs at 48MHz from Generic Clock Generator 0 on DFLL48M.

Generic Clock Generator 1 is using internal Ultra Low Power 32 khz oscillator.

USB and USART are using Generic Clock Generator 0 also.

**Memory Mapping**

Bootloader code will be located at 0x0 and executed before any applicative code.

Applications compiled to be executed along with the bootloader will start at 0x2000 (see linker script bootloader_samd21x18.ld).

Before jumping to the application, the bootloader changes the VTOR register to use the interrupt vectors of the application @0x2000.<- not required as application code is taking care of this.

## 5- How to build

```
BOARD_ID=bluesun NAME=bluesun make clean all
```
