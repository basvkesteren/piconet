/*
    Piconet RS232 ethernet interface

    cpusettings.h
           
    Copyright (c) 2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
\file
CPU configuration (config bits)
*/
#ifndef _CPUSETTINGS_H_
#define _CPUSETTINGS_H_

#include "config.h"

/* Make sure we know about this CPU */
#if defined __SDCC && ( defined(__SDCC_PIC18F24J50) || defined (__SDCC_PIC18F26J50) || __SDCC_PIC18F27J53 ) || \
    defined __XC8  && ( defined(__18F24J50)         || defined (__18F26J50)         || __18F27J53 )

/* Stack */
#ifdef __SDCC
/* Initialise a stack of 255 bytes at RAM address 0x300 */
//#pragma stack 0x300 0xFF
#endif

#ifdef __XC8
/* No stack-setting */
#endif

/* Watchdog */
#if WATCHDOG == 0
#pragma config WDTEN = OFF
#else
#pragma config WDTEN = ON
#if WATCHDOG == 1
#pragma config WDTPS = 1
#elif WATCHDOG == 2
#pragma config WDTPS = 2
#elif WATCHDOG == 4
#pragma config WDTPS = 4
#elif WATCHDOG == 8
#pragma config WDTPS = 8
#elif WATCHDOG == 16
#pragma config WDTPS = 16
#elif WATCHDOG == 32
#pragma config WDTPS = 32
#elif WATCHDOG == 64
#pragma config WDTPS = 64
#elif WATCHDOG == 128
#pragma config WDTPS = 128
#elif WATCHDOG == 256
#pragma config WDTPS = 256
#elif WATCHDOG == 512
#pragma config WDTPS = 512
#elif WATCHDOG == 1024
#pragma config WDTPS = 1024
#elif WATCHDOG == 2048
#pragma config WDTPS = 2048
#elif WATCHDOG == 4096
#pragma config WDTPS = 4096
#elif WATCHDOG == 8192
#pragma config WDTPS = 8192
#elif WATCHDOG == 16384
#pragma config WDTPS = 16384
#elif WATCHDOG == 32768
#pragma config WDTPS = 32768
#else
#error Invalid watchdog prescaler selected!
#endif
#endif

/* Deep-sleep watchdog */
#pragma config DSWDTEN = OFF        //Deep-sleep watchdog timer disabled
#pragma config DSWDTPS = 8192       //Deep-sleep timer (1:8192, 8.5 seconds)
#pragma config DSWDTOSC = INTOSCREF //Deep-sleep watchdog uses internal RC

/* Clock
   setup for a 48MHz clock from the internal RC oscillator:

    RC = 8MHz, divived by 2 (PLLDIV) gives 4MHz, which is the fixed input frequency of the PLL
    PLL generates 96MHz, which is divided in half (not a setting!), this is used for the USB module
    CPUDIV = 1, thus we get a 48MHz clock
 */
#pragma config PLLDIV = 2           //Divide by 2 (8 MHz oscillator input)
#pragma config CPUDIV = OSC1        //CPU system clock not divided (required on the j53 to make UART Rx work!)
#pragma config OSC = INTOSCPLL
#pragma config RTCOSC = INTOSCREF   //RTC uses internal RC
#pragma config FCMEN = OFF          //Fail-safe clock monitor (switch to internal RC if external clock fails)
#pragma config IESO = OFF           //Two-speed start-up disabled (run on internal RC untill external clock is up)

/* Code protection */
#pragma config CP0 = OFF
#pragma config WPCFG = OFF          //Configuration words page not erase/write-protected

/* Misc. */
#pragma config STVREN = ON          //Stack overflow/underflow reset
#pragma config XINST = OFF          //Extended instruction set disabled (both SDCC and XC8 lack support for this!)
#pragma config DSBOREN = ON         //Deep-sleep BOR enabled
#pragma config IOL1WAY = OFF        //The IOLOCK bit can be set and cleared
#pragma config MSSP7B_EN = MSK5     //5 bit MSSP address masking mode

#else
#error Unknown MCU
#endif

#endif /* _CPUSETTINGS_H_ */
