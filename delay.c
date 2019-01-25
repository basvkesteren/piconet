/*
    Piconet RS232 ethernet interface

    delay.c
           
    Copyright (c) 2006, 2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
\file
Simple delay functions
*/
#include "delay.h"
#ifdef __SDCC
#include <pic16/delay.h>
#endif

#ifdef __SDCC
#if CCLK != 48000000
#warning Delay-functions assume a 24MHz clock
#endif

void delay_us(volatile unsigned char time)
/*!
  Delay of approximately 'time' microseconds
  48MHz clock = 0,083us/instruction -> 12 instructions per us
*/
{
    while(time>0) {
        NOP();
        time--;
    }
}

void delay_ms(volatile unsigned int time)
/*!
  Delay of approximately 'time' milliseconds
  48MHz clock = 0,083us/instruction -> 12 instructions per ms
*/
{
    while(time>0) {
        delay1ktcy(12);
        time--;
    }
}
#endif

void delay_s(volatile unsigned int time)
/*!
  Delay of approximately 'time' seconds
*/
{
    while(time>0) {
        delay_ms(250);
        delay_ms(250);
        delay_ms(250);
        delay_ms(250);
        time--;
    }
}
