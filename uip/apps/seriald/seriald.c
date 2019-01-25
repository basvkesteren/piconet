/*
    Piconet RS232 ethernet interface

    seriald.c
           
    Copyright (c) 2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
\file
Serial-to-ethernet
*/
#include "seriald.h"
#include "settings.h"
#include "debug.h"
#include "enc28j60_freebuffer.h"
#include "uip.h"
#include "uip_arp.h"

/* Application state */
unsigned char state;
#define STATE_IDLE      0
#define STATE_CONNECTED 1
#define STATE_CLOSE     2
#define STATE_SHUTDOWN  3

/* Our client */
uip_ipaddr_t connected_to_ipaddr;
u16_t connected_to_port;

/* Incoming serial data, double buffered */
unsigned char buffer[2][100];
unsigned char pointer[2];
unsigned char write, read;
bool polled_without_transfer;

/* Bytes in transfer (that is, called uip_send(), no ack received yet) */
unsigned short bytesintransfer;

/* Checksum of outgoing packet(s), */
extern u16_t uip_tcpchksum_incontroller_1, uip_tcpchksum_incontroller_2;

seriald_statistics_t seriald_statistics;

void seriald_init(void)
/*!
  
*/
{
    if(network_mode_tcp()) {
        uip_listen(HTONS(settings.network_port));
        dprint("seriald_init(): listening on TCP port %d\n\r", settings.network_port);
    }

    state = STATE_IDLE;

    write = 0;
    read = 1;
    pointer[write] = 0;
    pointer[read] = 0;

    bytesintransfer = 0;

    seriald_statistics.retransmitted = 0;
    seriald_statistics.net_dropped = 0;
    seriald_statistics.uart_dropped = 0;
    seriald_statistics.controller_full = 0;

    polled_without_transfer = FALSE;
}

void seriald_shutdown(void)
/*!
  
*/
{
    if(state != STATE_IDLE) {
        state = STATE_SHUTDOWN;
    }
}

void seriald_disconnect(void)
/*!
  
*/
{
    if(state != STATE_IDLE) {
        state = STATE_CLOSE;
    }
}

void seriald_incoming(const unsigned char c)
/*!
  Store incoming byte in our write-buffer
*/
{
    if(pointer[write] < sizeof(buffer[write])) {
        buffer[write][pointer[write]] = c;
        pointer[write]++;
    }
    else {
        seriald_statistics.net_dropped++;
    }
}

void seriald_dropped(void)
{
    seriald_statistics.uart_dropped++;
}

bool seriald_shouldtransfer(void)
/*!
  
*/
{
    if(pointer[write] == 0) {
        /* No pending data, at all */
        return FALSE;
    }

    if(bytesintransfer) {
        /* Data currently in controller isn't ACK'd yet */
        return FALSE;
    }

    if(pointer[write] > (sizeof(buffer[write]) / 2) || polled_without_transfer) {
        /* We where polled without sending anything, or write buffer has reached it's threshold */
        return TRUE;
    }
    return FALSE;
}

void seriald_switchbuffers(void)
/*!
  
*/
{
    write = (!write) & 1;
    read = (!read) & 1;
}

void seriald_transfer(void)
/*!
  
*/
{
    extern u16_t uip_chksum_singlebytes;
    u8_t len1, len2;

    // TODO: remaining-space, should be smarter about this
    if(enc28j60_freebuffer_written + pointer[read]  < FREEBUFFERLENGTH - (2 * TCPIP4_HEADER_LENGTH)) {
#if UIP_SPLIT     
        if(enc28j60_freebuffer_written > UIP_SPLIT_SIZE) {
            /* Already working on the second packet */
            uip_tcpchksum_incontroller_2 = uip_chksum_bytes(uip_tcpchksum_incontroller_2, buffer[read], pointer[read]);
            enc28j60_put_freebuffer_payload(2 * TCPIP4_HEADER_LENGTH, buffer[read], pointer[read]);
        }
        else if(enc28j60_freebuffer_written + pointer[read] == UIP_SPLIT_SIZE) {
            /* We'll fill the first packet */
            uip_tcpchksum_incontroller_1 = uip_chksum_bytes(uip_tcpchksum_incontroller_1, buffer[read], pointer[read]);
            /* Reset checksum calculation */
            uip_chksum_singlebytes = 0;
            enc28j60_put_freebuffer_payload(TCPIP4_HEADER_LENGTH, buffer[read], pointer[read]);
        }
        else if(enc28j60_freebuffer_written + pointer[read] > UIP_SPLIT_SIZE) {
            /* We'll write to the first AND the second packet */
            len1 = pointer[read] - ((enc28j60_freebuffer_written + pointer[read]) - UIP_SPLIT_SIZE);
            len2 = (enc28j60_freebuffer_written + pointer[read]) - UIP_SPLIT_SIZE;
            uip_tcpchksum_incontroller_1 = uip_chksum_bytes(uip_tcpchksum_incontroller_1, buffer[read], len1);
            /* Reset checksum calculation */
            uip_chksum_singlebytes = 0;
            uip_tcpchksum_incontroller_2 = uip_chksum_bytes(uip_tcpchksum_incontroller_2, &buffer[read][len1], len2);

            enc28j60_put_freebuffer_payload(TCPIP4_HEADER_LENGTH, buffer[read], len1);
            enc28j60_put_freebuffer_payload(2* TCPIP4_HEADER_LENGTH, &buffer[read][len1], len2);
        }
        else {
            /* Still working on the first packet */
            uip_tcpchksum_incontroller_1 = uip_chksum_bytes(uip_tcpchksum_incontroller_1, buffer[read], pointer[read]);
            enc28j60_put_freebuffer_payload(TCPIP4_HEADER_LENGTH, buffer[read], pointer[read]);
        }
#else
        uip_tcpchksum_incontroller = uip_chksum_bytes(uip_tcpchksum_incontroller, buffer[read], pointer[read]);
        enc28j60_put_freebuffer_payload(TCPIP4_HEADER_LENGTH, buffer[read], pointer[read]);
#endif
        pointer[read] = 0;
    }
    else {
        seriald_statistics.controller_full++;
    }
}

void seriald_appcall(void)
/*!
  
*/
{
    switch(state) {
        case STATE_IDLE:
            if(uip_connected()) {
                uip_ipaddr_copy(&connected_to_ipaddr, uip_conn->ripaddr);
                connected_to_port = uip_conn->rport;
                dprint("seriald bound to %d.%d.%d.%d:%d\n\r", uip_ipaddr1(connected_to_ipaddr),
                                                              uip_ipaddr2(connected_to_ipaddr),
                                                              uip_ipaddr3(connected_to_ipaddr),
                                                              uip_ipaddr4(connected_to_ipaddr),
                                                              connected_to_port);
                state = STATE_CONNECTED;
                pointer[write] = 0;
                seriald_connected();
             }
            break;
        case STATE_CONNECTED:
            if(uip_ipaddr_cmp(connected_to_ipaddr, uip_conn->ripaddr) && connected_to_port == uip_conn->rport) {
                if(uip_closed() || uip_aborted() || uip_timedout()) {
                    state = STATE_IDLE;
                    seriald_disconnected();
                }
                else if(uip_rexmit()) {
                    if(bytesintransfer) {
                        seriald_statistics.retransmitted += bytesintransfer;
                        uip_send(NULL, bytesintransfer);
                    }
                    else {
                        dprint("seriald_appcall(): cannot retransmit!\n\r");
                    }
                }
                else if(uip_poll() || uip_acked()) {
                    if(uip_acked()) {
                        bytesintransfer = 0;
                        uip_tcpchksum_incontroller_1 = 0;
                        uip_tcpchksum_incontroller_2 = 0;
                    }

                    if(enc28j60_freebuffer_written > 1000 || polled_without_transfer) {
                        if(bytesintransfer == 0) {
                            /* Calling uip_send with a NULL-pointer so that uIP knows the
                               packet is already in the ethernet controller RAM */
                            uip_send(NULL, enc28j60_freebuffer_written);
                            /* And cleanup */
                            bytesintransfer = enc28j60_freebuffer_written;
                            enc28j60_put_freebuffer_restart();
                        }
                        polled_without_transfer = FALSE;
                    }
                    else if(uip_poll()) {
                        polled_without_transfer = TRUE;
                    }

                    if(uip_newdata()) {
                        /* ACK and CLOSE can be combined! */
                        seriald_received(uip_appdata, uip_datalen());
                    }
                }
                else if(uip_newdata()) {
                    seriald_received(uip_appdata, uip_datalen());
                }
                else {
                    dprint("seriald unhandled state while connected, uip_flags=%d\n\r", uip_flags);
                }
            }
            else {
                 /* Not our client */
                if(uip_poll()) {
                    uip_close();
                }
            }
            break;
        case STATE_CLOSE:
            uip_close();
            state = STATE_IDLE;
            break;
        case STATE_SHUTDOWN:
            uip_close();
            uip_unlisten(HTONS(settings.network_port));
            state = STATE_IDLE;
            break;
    }
}
