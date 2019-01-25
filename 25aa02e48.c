/*
    Piconet RS232 ethernet interface

    25aa02e48.c
           
    Copyright (c) 2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
 * \file
 * 25AA02E48 driver
 */
#include "25aa02e48.h"
#include <ssp.h>
#include <io.h>
#include <delay.h>

/* Instructions */
#define INST_READ       0x03    // Read data from memory array beginning at selected address
#define INST_WRITE      0x02    // Write data to memory array beginning at selected address
#define INST_WRDI       0x04    // Reset the write enable latch (disable write operations)
#define INST_WREN       0x06    // Set the write enable latch (enable write operations)
#define INST_RDSR       0x05    // Read Status Register
#define INST_WRSR       0x01    // Write Status Register

/* Statusregister bits */
#define STATUS_WIP      (1<<0)
#define STATUS_WEL      (1<<1)
#define STATUS_BP0      (1<<2)
#define STATUS_BP1      (1<<3)

static bool eeprom_25aa02e48_wait(void)
/*!
  Wait untill the Write-In-Process bit clears
*/
{
    unsigned char status;
    unsigned char timeout=0;

    do {
        delay_ms(1);
        eeprom_cs_assert();     // Select device
        ssp2_put(INST_RDSR);    // Send read statusregister instruction
        status = ssp2_get();    // Read statusregister
        eeprom_cs_deassert();
        timeout++;
    }
    while((status & STATUS_WIP) && timeout);

    if(timeout == 0) {
        return FALSE;
    }
    return TRUE;
}

bool eeprom_25aa02e48_writebyte(const unsigned char address, const unsigned char data)
/*!
 *  Write a single byte
 */
{
    eeprom_cs_assert();         // Select device
    ssp2_put(INST_WREN);        // Send write enable instruction
    eeprom_cs_deassert();       // Deselect device

    delay_us(1);                // Wait for a bit

    eeprom_cs_assert();         // Select device
    ssp2_put(INST_WRITE);       // Send write instruction
    ssp2_put(address);          // Send address
    ssp2_put(data);             // Send data
    eeprom_cs_deassert();       // Deselect device

    return eeprom_25aa02e48_wait(); // Wait until write is complete
}

unsigned char eeprom_25aa02e48_readbyte(const unsigned char address)
/*!
 *  Read a single byte
 */
{
    unsigned char data;

    eeprom_cs_assert();         // Select device
    ssp2_put(INST_READ);        // Send read instruction
    ssp2_put(address);          // Send address
    data = ssp2_get();          // Read databyte
    eeprom_cs_deassert();       // Deselect device

    return data;
}

void eeprom_25aa02e48_read(unsigned char *pointer, const unsigned char address, unsigned char length)
/*!
 *  Read data
 */
{
    eeprom_cs_assert();         // Select device
    ssp2_put(INST_READ);        // Send read instruction
    ssp2_put(address);          // Send address
    while(length > 0) {
        *pointer = ssp2_get();  // Read databyte
        pointer++;
        length--;
    }
    eeprom_cs_deassert();       // Deselect device
}

bool eeprom_25aa02e48_getEUI48(unsigned char *pointer)
/*!
 *  Write the EUI-48 address to the given 6 bytes array
 */
{
    eeprom_25aa02e48_read(pointer,0xFA,6);
    
    if(pointer[0] == 0 && pointer[1] == 0 && pointer[2] == 0) {
        return FALSE;
    }
    return TRUE;
}
