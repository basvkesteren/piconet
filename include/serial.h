/*
    Piconet RS232 ethernet interface

    serial.h
           
    Copyright (c) 2006,2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
\file
On-chip UART access function definitions
*/
#ifndef _SERIAL_H_
#define _SERIAL_H_

#include "config.h"

/* Baudrate is set with some defines:
    BAUDRATEx_BRG16: 16 bit mode
    BAUDRATEx_BRGH: high baudrate mode
    BAUDRATEx: baudrate timer value

    Calculate as follows:
     BAUDRATEx_BRG16 = 0, BAUDRATEx_BRGH = 0: BAUDRATEx = FOSC/[64 (n + 1)]
     BAUDRATEx_BRG16 = 0, BAUDRATEx_BRGH = 1: BAUDRATEx = FOSC/[16 (n + 1)]
     BAUDRATEx_BRG16 = 1, BAUDRATEx_BRGH = 0: BAUDRATEx = FOSC/[16 (n + 1)]
     BAUDRATEx_BRG16 = 1, BAUDRATEx_BRGH = 1: BAUDRATEx = FOSC/[4 (n + 1)]
*/

void serial1_init(void);
void serial1_putchar(const unsigned char c);
#define serial1_setbaudrate(x)  do { \
                                    SPBRG1 = x; \
                                    SPBRGH1 = x>>8; \
                                } while(0)
#define serial1_int_suspend()   do { \
                                    PIE1bits.RCIE = 0; \
                                } while(0)
#define serial1_int_resume()    do { \
                                    PIE1bits.RCIE = 1; \
                                } while(0)

void serial2_init(void);
void serial2_putchar(const unsigned char c);
#define serial2_setbaudrate(x)  do { \
                                    SPBRG2 = x; \
                                    SPBRGH2 = x>>8; \
                                } while(0)                               
#define serial2_int_suspend()   do { \
                                    PIE3bits.RC2IE = 0; \
                                } while(0)
#define serial2_int_resume()    do { \
                                    PIE3bits.RC2IE = 1; \
                                } while(0)

#endif /* _SERIAL_H_ */
