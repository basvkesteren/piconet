/*
    Piconet RS232 ethernet interface

    std.h
           
    Copyright (c) 2006,2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
\file
Some all-around functions
*/
#ifndef STD_H
#define STD_H

#include "config.h"

int abs(int j);
int fpow(const int ground, const int n);
unsigned char ctoi(const unsigned char c, const unsigned char base);
bool isnumber(const unsigned char c, const unsigned char base);
unsigned char inttostrn(unsigned int num, char *c, unsigned char limit, const unsigned char base);
unsigned int strtoint(const char *c, const unsigned char base);

#endif /* STD_H */
