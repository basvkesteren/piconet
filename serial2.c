/*
    Piconet RS232 ethernet interface

    serial2.c
           
    Copyright (c) 2006,2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
\file
On-chip UART2 access
*/
#include "serial.h"
#include "delay.h"

void serial2_init(void)
/*!
  Configure UART2
*/
{
    /* Transmit control register */
    TXSTA2bits.TX9 = 0;   // 9-bit Transmit Enable Bit: 0 = 8-bit transmit
    TXSTA2bits.TXEN = 1;  // Transmit Enable Bit: 1 = transmit enabled
    TXSTA2bits.SYNC = 0;  // USART Mode select Bit: 0 = asynchronous
    TXSTA2bits.SENDB = 0; // Send break character bit: 0 = sync break transmission completed
#if BAUDRATE2_BRGH
    TXSTA2bits.BRGH = 1;  // High Baud Rate Select Bit: 1 = high speed
#else
    TXSTA2bits.BRGH = 0;  // High Baud Rate Select Bit: 0 = low speed
#endif

    /* Receive control register */
    RCSTA2bits.RX9 = 0;   // 9-bit Receive Enable Bit: 0 = 8-bit reception
    RCSTA2bits.CREN = 1;  // Continuous Receive Enable Bit: 1 = Enables receiver
    RCSTA2bits.ADDEN = 0; // Disable address detection

    /* Interrupts */
    PIE3bits.TX2IE = 0;   // USART Transmit Interupt Enable Bit: 0 = disabled
    PIE3bits.RC2IE = 1;   // USART Receive Interupt Enable Bit: 1 = enabled
    IPR3bits.RC2IP = 1;   // High priority

    /* Baudrate generation control Register */
#if BAUDRATE2_BRG16
    BAUDCON2bits.BRG16 = 1; // 16 bit baudrate generator
#else
    BAUDCON2bits.BRG16 = 0; // 16 bit baudrate generator
#endif
    BAUDCON2bits.WUE = 0;   // RX pin wakeup disabled
    BAUDCON2bits.ABDEN = 0; // baudrate measurment disabled

    /* Set baudrate (see config.h) */
    SPBRG2 = BAUDRATE2&0xFF;
    SPBRGH2 = BAUDRATE2>>8;

    /* And enable serial port */
    RCSTA2bits.SPEN = 1;
}

void serial2_putchar(const unsigned char c)
/*!
  Send a character on UART2
*/
{
    /* Wait untill TXREG has room */
    while(PIR3bits.TX2IF==0);
    /* Write character, this clears TXIF */
    TXREG2 = c;
}
