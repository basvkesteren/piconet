/*
    Piconet RS232 ethernet interface

    enc28j60.h
           
    Copyright (c) 2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
\file
ENC28J60 driver, the definitions
*/
#ifndef ENC28J60_H
#define ENC28J60_H

#include <config.h>

/* Maximum frame length; frames bigger than this cannot be received or transmitted
 */
#define NETWORK_MAXPACKETLENGTH     1518

/* Set this to '0' to do packet reception on interrupts, '1' otherwise
 */
#define ENC28J60_RX_POLL            1

/* Receive/transmit memory organization; There's 8 KBytes available to divide
   in two for a transmit and a receive buffer.
   Transmitbuffer has room for one full-size packet (NETWORK_MAXPACKETLENGTH
   with a 7 bytes txstatusvector), receivebuffer gets the rest.

   Keep the RX buffer first (see Erreta rev. B7, note 3), and make sure RXEND
   is an odd value (see Erreta rev. B7, note 14) */
#define RXSTART             0x0000  
#define RXEND               0x140D  // RX buffer includes RXEND!
#define TXSTART             0X1A09
#define TXEND               0x1FFF
#define RXBUFFERLENGTH      (RXEND - RXSTART)
#define TXBUFFERLENGTH      (TXEND - TXSTART)

/* Function prototypes */
bool enc28j60_init(const unsigned char MAC[8]);
void enc28j60_setduplex(const bool full);
bool enc28j60_link(void);
void enc28j60_put_transmit(const unsigned short location, const unsigned short length);
void enc28j60_put_copydata(unsigned char *data, const unsigned short length);
void enc28j60_put_startofpacket(const unsigned short location);
void enc28j60_put_setwritepointer(const unsigned short location);
void enc28j60_put_wait(void);
unsigned char enc28j60_pendingpackets(void);
unsigned short enc28j60_get(unsigned char *packetbuffer);
void enc28j60_int(void);

void network_linkchange(void);

#define enc28j60_int_suspend()      do { \
                                        INTCON3bits.INT1IE = 0; \
                                    } while (0)
#define enc28j60_int_resume()       do { \
                                        INTCON3bits.INT1IE = 1; \
                                    } while (0)

#define enc28j60_put(data, length)  do { \
                                        enc28j60_put_wait(); \
                                        enc28j60_int_suspend(); \
                                        enc28j60_put_startofpacket(TXSTART); \
                                        enc28j60_put_copydata(data, length); \
                                        enc28j60_put_transmit(TXSTART, length); \
                                        enc28j60_int_resume(); \
                                    } while (0)

#endif /* ENC28J60_H */
