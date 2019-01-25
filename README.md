# piconet

This are the firmware sources for 'Piconet', a small serial-to-network converter, build around an 8-bit PIC microcontroller. 
The plan is to do serial to TCP or UDP (and vice versa), and serial to SD.

![Piconet board rev. 1](piconet.jpg?raw=true "Piconet board rev. 1")

It uses uIP-1.0 (https://github.com/adamdunkels/uip/releases/tag/uip-1-0) with some tweaks which make it possible to write incoming serial data directly to the buffer of the network controller. Once this buffer is full, the packet is completed (by adding the right headers) and send out. This is done to work around the limited amount of RAM of the microcontroller.

For SD access FatFs (http://elm-chan.org/fsw/ff/00index_e.html) is used. An inserted SD-card is detected, and the telnet console includes a simple 'ls' and 'cat' command. Logging to SD is not yet implemented.

## Current status
The basics are in place; it compiles and runs. One can set a static IP, or get a DHCP lease. Furthermore there is a simple telnet console, used to modify some parameters, and serial-to-TCP works with reasonable peformance.

## Toolchain
The included Makefile is setup for two toolchains, both on Linux and Windows.
On Windows I use the GnuWin make (http://gnuwin32.sourceforge.net/packages/make.htm) and coreutils (http://gnuwin32.sourceforge.net/packages/coreutils.htm), a typical Linux system already includes all this.

### XC8
Microchip's own compiler, available at https://www.microchip.com/mplab/compilers. I use the free version, the paid version includes some more optimisations.
To use, set the 'CC' variable in the Makefile to 'xc8'.

### SDCC
The Small Device C Compiler, available at http://sdcc.sourceforge.net/. Note that the PIC-support is 'work in progress'; I've done several projects with it, all to good success. However, it seems the piconet sources are a tad too big; using the current snapshot, the code compiles, but the resulting binary has some issues.
