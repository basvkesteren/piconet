/*
    Piconet RS232 ethernet interface

    25aa02e48.h
           
    Copyright (c) 2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
\file
25AA02E48 driver
*/
#ifndef _25AA02E48_H_
#define _25AA02E48_H_

#include "config.h"

bool eeprom_25aa02e48_writebyte(const unsigned char address, const unsigned char data);
unsigned char eeprom_25aa02e48_readbyte(const unsigned char address);
void eeprom_25aa02e48_read(unsigned char *pointer, const unsigned char address, unsigned char length);
bool eeprom_25aa02e48_getEUI48(unsigned char *pointer);

#endif /* _25AA02E48_H_ */
