/*
    Piconet RS232 ethernet interface

    ssp2.c
           
    Copyright (c) 2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
\file
On-chip SSP2 access
*/
#include "ssp.h"
#include "delay.h"

void ssp2_init(void)
/*!
  Configure SSP2
*/
{
    /* SSP2, setup for SPI mode 0,0 */
    SSP2STATbits.CKE = 1;       // Transmit on active to idle clock state
    SSP2STATbits.SMP = 0;       // Input data is sampled at the middle of data output time
    SSP2CON1bits.CKP = 0;       // Clock idle state is low
    SSP2CON1bits.SSPM3 = 0;     //
    SSP2CON1bits.SSPM2 = 0;     // Master mode..
    SSP2CON1bits.SSPM1 = 0;     // ..Clock=Fosc/4, 12MHz
    SSP2CON1bits.SSPM0 = 0;     //
    SSP2CON1bits.SSPEN = 1;     // Enable interface
    PIE3bits.SSP2IE = 0;        // Disable interrupt
}

bool ssp2_put(unsigned char x)
{
    /* Clear interrupt flag */
    PIR3bits.SSP2IF = 0;   
    /* Write data */
    SSP2BUF = x;    
    /* Test for a write collision */
    if(SSP2CON1bits.WCOL) {         
        return FALSE;
    }
    /* Wait untill transmission is complete */
    while(PIR3bits.SSP2IF == 0);
    return TRUE;
}

unsigned char ssp2_get(void)
{      
    /* Write single bogus byte */
    SSP2BUF = 0xFF;
    /* Now wait untill interface is ready */
    while(SSP2STATbits.BF == 0);
    /* Return received data */
    return SSP2BUF;
}
