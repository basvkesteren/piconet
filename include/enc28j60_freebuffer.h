/*
    Piconet RS232 ethernet interface

    enc28j60_freebuffer.h
           
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
#ifndef ENC28J60_FREEBUFFER_H
#define ENC28J60_FREEBUFFER_H

#include "enc28j60.h"

/* The 'free buffer'; the ENC28J60 has a 8Kbytes RAM buffer; 
   in enc28j60.h we've defined what part of that buffer is used for reception,
   and what part is used for transmission.
   The gap in between the reception and transmission region, if any, is what this
   header-file is about: the 'free buffer'. */
#define FREESTART           (RXEND+1)
#define FREEEND             (TXSTART-1)
#define FREEBUFFERLENGTH    (FREEEND - FREESTART)

/* This will transmit the given header-data to the controller at address 'FREESTART + offset',
   then start a transmission for a packet with a total length of 'header_length + payload_length'.
   It is thus assumed that the payload of this packet is already in the controller's RAM at
   the right location (that is, right after the header we're copying here) */
#define enc28j60_put_freebuffer(offset, header, header_length, payload_length) \
                                        do { \
                                            enc28j60_put_wait(); \
                                            enc28j60_int_suspend(); \
                                            enc28j60_put_startofpacket(FREESTART + offset); \
                                            enc28j60_put_copydata(header, header_length); \
                                            enc28j60_put_transmit(FREESTART + offset, header_length + payload_length); \
                                            enc28j60_int_resume(); \
                                        } while(0)

/* No. of bytes transferred with 'enc28j60_put_freebuffer_payload()'; it's incremented by that function,
   but it's up to the user-code to clear it */
extern unsigned short enc28j60_freebuffer_written;

#define enc28j60_put_freebuffer_restart() \
                                        do { \
                                            enc28j60_freebuffer_written = 0; \
                                        } while(0)

/* No. of bytes for a complete TCP header; 1 controlbyte (see enc28j60_put_startofpacket()), 14 bytes Ethernet header,
   20 bytes IPv4 header, 20 bytes TCP header */
#define TCPIP4_HEADER_LENGTH   (1 + 14 + 20 + 20)

void enc28j60_put_freebuffer_payload(const unsigned short offset, const unsigned char *payload, const unsigned short payload_length);

#endif /* ENC28J60_FREEBUFFER_H */