/*
    Piconet RS232 ethernet interface

    ssp.h
           
    Copyright (c) 2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
\file
On-chip SSP access function definitions
*/
#ifndef _SSP_H_
#define _SSP_H_

#include "config.h"

void ssp1_init(void);
bool ssp1_put(unsigned char x);
unsigned char ssp1_get(void);
/* Clock = Fosc/4 */
#define ssp1_clock_4()  do { \
                            SSP1CON1 &= ~0x0F; \
                        } while(0)
/* Clock = Fosc/8 */
#define ssp1_clock_8()  do { \
                            SSP1CON1 &= ~0x0F; \
                            SSP1CON1 |= 0x0A; \
                        } while(0)
/* Clock = Fosc/16 */
#define ssp1_clock_16() do { \
                            SSP1CON1 &= ~0x0F; \
                            SSP1CON1 |= 0x01; \
                        } while(0)
/* Clock = Fosc/64 */
#define ssp1_clock_64() do { \
                            SSP1CON1 &= ~0x0F; \
                            SSP1CON1 |= 0x02; \
                        } while(0)

void ssp2_init(void);
bool ssp2_put(unsigned char x);
unsigned char ssp2_get(void);
/* Clock = Fosc/4 */
#define ssp2_clock_4()  do { \
                            SSP2CON1 &= ~0x0F; \
                        } while(0)
/* Clock = Fosc/8 */
#define ssp2_clock_8()  do { \
                            SSP2CON1 &= ~0x0F; \
                            SSP2CON1 |= 0x0A; \
                        } while(0)
/* Clock = Fosc/16 */
#define ssp2_clock_16() do { \
                            SSP2CON1 &= ~0x0F; \
                            SSP2CON1 |= 0x01; \
                        } while(0)
/* Clock = Fosc/64 */
#define ssp2_clock_64() do { \
                            SSP2CON1 &= ~0x0F; \
                            SSP2CON1 |= 0x02; \
                        } while(0)

#endif /* _SSP_H_ */
