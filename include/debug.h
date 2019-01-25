/*
    Piconet RS232 ethernet interface

    debug.h
           
    Copyright (c) 2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
\file
Global DEBUG definition
*/
#ifndef _DEBUG_H_
#define _DEBUG_H_

#include "serial.h"
#ifdef __SDCC
#include <pic16/stdio.h>
#endif
#ifdef __XC8
#include <stdio.h>
#endif

#if DEBUG
#define dprint printf
#else
#define dprint //
#endif

#endif /* _DEBUG_H_ */
