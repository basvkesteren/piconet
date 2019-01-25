/*
    Piconet RS232 ethernet interface

    delay.h
           
    Copyright (c) 2006,2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
\file
Simple delay function definitions
*/
#ifndef DELAY_H
#define DELAY_H

#include "config.h"

#ifdef __SDCC
void delay_us(volatile unsigned char time);
void delay_ms(volatile unsigned int time);
#endif

#ifdef __XC8
#define _XTAL_FREQ CCLK
#include <xc.h>

#define delay_us(x) __delay_us(x)
#define delay_ms(x) __delay_ms(x)
#endif

void delay_s(volatile unsigned int time);

#endif /* DELAY_H */
