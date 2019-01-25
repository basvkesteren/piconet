/*
    Piconet RS232 ethernet interface

    seriald.h
           
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
#ifndef SERIALD_H
#define SERIALD_H

#include "config.h"

void seriald_init(void);
void seriald_shutdown(void);
void seriald_disconnect(void);
bool seriald_shouldtransfer(void);
void seriald_switchbuffers(void);
void seriald_transfer(void);
void seriald_appcall(void);

void seriald_incoming(const unsigned char c);
void seriald_dropped(void);

void seriald_connected(void);
void seriald_disconnected(void);
void seriald_received(const char* data, const unsigned int length);

typedef struct {
    unsigned int retransmitted,
                 net_dropped,
                 uart_dropped,
                 controller_full;
} seriald_statistics_t;

#endif /* SERIALD_H */

