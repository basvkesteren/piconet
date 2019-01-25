/*
    Piconet RS232 ethernet interface

    types.h
           
    Copyright (c) 2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
\file
Global type definitions
*/
#ifndef TYPES_H
#define TYPES_H

/* Some (hopefully) usefull defines */
#ifndef FALSE
#define FALSE       0
#endif

#ifndef TRUE
#define TRUE        1
#endif

#ifndef NULL
#define NULL        ((void *)0)
#endif

#define ON      1
#define OFF     0

/*
  And some general stuff
*/
typedef unsigned char u8_t;
typedef unsigned short u16_t;
typedef unsigned char byte;
typedef unsigned int word;

typedef unsigned char bool;

#endif /* TYPES_H */
