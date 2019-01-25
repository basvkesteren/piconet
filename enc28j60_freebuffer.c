/*
    Piconet RS232 ethernet interface

    enc28j60_freebuffer.c
           
    Copyright (c) 2019 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
\file
ENC28J60 driver, the definitions
*/

#include "enc28j60.h"
#include "enc28j60_freebuffer.h"

unsigned short enc28j60_freebuffer_written;

void enc28j60_put_freebuffer_payload(const unsigned short offset, const unsigned char *payload, const unsigned short payload_length)
/*!
  Copy 'payload_length' bytes from 'payload' to the 'free buffer', add address FREESTART + offset.
*/ 
{
    enc28j60_int_suspend();
    /* Set writepointer; start of buffer 'FREESTART', plus the given offset,
       plus the no. of bytes already written since enc28j60_put_freebuffer_restart() was called */
    enc28j60_put_setwritepointer(FREESTART + offset + enc28j60_freebuffer_written);
    enc28j60_put_copydata(payload, payload_length);
    enc28j60_int_resume();  

    enc28j60_freebuffer_written += payload_length;
}
