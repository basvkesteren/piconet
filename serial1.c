/*
    Piconet RS232 ethernet interface

    serial1.c
           
    Copyright (c) 2006,2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
\file
On-chip UART1 access
*/
#include "serial.h"
#include "delay.h"

void serial1_init(void)
/*!
  Configure UART1
*/
{
    /* Transmit control register */
    TXSTA1bits.TX9 = 0;   // 9-bit Transmit Enable Bit: 0 = 8-bit transmit
    TXSTA1bits.TXEN = 1;  // Transmit Enable Bit: 1 = transmit enabled
    TXSTA1bits.SYNC = 0;  // USART Mode select Bit: 0 = asynchronous
    TXSTA1bits.SENDB = 0; // Send break character bit: 0 = sync break transmission completed
#if BAUDRATE1_BRGH
    TXSTA1bits.BRGH = 1;  // High Baud Rate Select Bit: 1 = high speed
#else
    TXSTA1bits.BRGH = 0;  // High Baud Rate Select Bit: 0 = low speed
#endif

    /* Receive control register */
    RCSTA1bits.RX9 = 0;   // 9-bit Receive Enable Bit: 0 = 8-bit reception
    RCSTA1bits.CREN = 1;  // Continuous Receive Enable Bit: 1 = Enables receiver
    RCSTA1bits.ADDEN = 0; // Disable address detection

    /* Interrupts */
    PIE1bits.TXIE = 0;   // USART Transmit Interupt Enable Bit: 0 = disabled
    PIE1bits.RCIE = 1;   // USART Receive Interupt Enable Bit: 1 = enabled
    IPR1bits.RCIP = 0;   // Low priority

    /* Baudrate generation control Register */
#if BAUDRATE1_BRG16
    BAUDCON1bits.BRG16 = 1; // 16 bit baudrate generator
#else
    BAUDCON1bits.BRG16 = 0; // 16 bit baudrate generator
#endif
    BAUDCON1bits.WUE = 0;   // RX pin wakeup disabled
    BAUDCON1bits.ABDEN = 0; // baudrate measurment disabled

    /* Set baudrate (see config.h) */
    SPBRG1 = BAUDRATE1&0xFF;
    SPBRGH1 = BAUDRATE1>>8;

    /* And enable serial port */
    RCSTA1bits.SPEN = 1;
}

void serial1_putchar(const unsigned char c)
/*!
  Send a character on UART1
*/
{
    /* Wait untill TXREG has room */
    while(PIR1bits.TXIF==0);
    /* Write character, this clears TXIF */
    TXREG1 = c;
}
