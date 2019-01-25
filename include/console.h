/*
    Piconet RS232 ethernet interface

    console.h
           
    Copyright (c) 2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
\file
Simple line console
*/
#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include "config.h"

void console_incoming(unsigned char incomingbyte);

#endif /* _CONSOLE_H_ */