/*
    Piconet RS232 ethernet interface

    ssp1.c
           
    Copyright (c) 2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
\file
On-chip SSP1 access
*/
#include "ssp.h"
#include "delay.h"

void ssp1_init(void)
/*!
  Configure SSP1
*/
{
    /* SSP1, setup for SPI mode 0,0 */
    SSP1STATbits.CKE = 1;       // Transmit on active to idle clock state
    SSP1STATbits.SMP = 0;       // Input data is sampled at the middle of data output time
    SSP1CON1bits.CKP = 0;       // Clock idle state is low
    SSP1CON1bits.SSPM3 = 0;     //
    SSP1CON1bits.SSPM2 = 0;     // Master mode..
    SSP1CON1bits.SSPM1 = 0;     // ..Clock=Fosc/4, 12MHz
    SSP1CON1bits.SSPM0 = 0;     //
    SSP1CON1bits.SSPEN = 1;     // Enable interface
    PIE1bits.SSP1IE = 0;        // Disable interrupt
}

bool ssp1_put(unsigned char x)
{
    /* Clear interrupt flag */
    PIR1bits.SSP1IF = 0;   
    /* Write data */
    SSP1BUF = x;    
    /* Test for a write collision */
    if(SSP1CON1bits.WCOL) {         
        return FALSE;
    }
    /* Wait untill transmission is complete */
    while(PIR1bits.SSP1IF == 0);
    return TRUE;
}

unsigned char ssp1_get(void)
{
    /* Write single bogus byte */
    SSP1BUF = 0xFF;
    /* Now wait untill interface is ready */
    while(SSP1STATbits.BF == 0);
    /* Return received data */
    return SSP1BUF;
}
