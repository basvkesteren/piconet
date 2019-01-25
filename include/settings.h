/*
    Piconet RS232 ethernet interface

    settings.h
           
    Copyright (c) 2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
\file
Piconet settings
*/
#ifndef SETTINGS_H
#define SETTINGS_H

#include "config.h"

/* Settings data */
typedef struct settings {
    unsigned char network_ip[4];        // Set to 0.0.0.0 for dhcp
    unsigned char network_mask[4];
    unsigned char network_gw[4];
    unsigned int network_port;
    unsigned char network_mode;         // TCP, UDP, whatnot -> bitmask
    unsigned int serial_baudrate;       // predefined values, 1200, 2400, 4800, and so on, and so forth
    unsigned char serial_mode;          // start and stopbits, parity, flowcontrol
} settings_t;

extern settings_t settings;

/* network_mode is a bitmask;
   Bit 0, TCP or UDP */
#define NETWORK_MODE_TCP                (1<<0)
#define network_mode_tcp()              (settings.network_mode & NETWORK_MODE_TCP)
#define network_mode_udp()              ((settings.network_mode & NETWORK_MODE_TCP) == 0)
/* Bit 1, */

/* serial_baudrate uses predefined values */
#if BAUDRATE2_BRG16 != 1 || BAUDRATE2_BRGH != 1 || CCLK != 48000000
#error Need to recalculate SERIAL_BAUDRATE_ defines for chosen CCLK and BAUDRATE_BRG values
#endif
#define SERIAL_BAUDRATE_300             39999
#define SERIAL_BAUDRATE_1200            9999
#define SERIAL_BAUDRATE_2400            4999
#define SERIAL_BAUDRATE_4800            2499
#define SERIAL_BAUDRATE_9600            1249
#define SERIAL_BAUDRATE_19200           624
#define SERIAL_BAUDRATE_38400           312
#define SERIAL_BAUDRATE_57600           207
#define SERIAL_BAUDRATE_115200          103
#define SERIAL_BAUDRATE_230400          51

/* serial_mode is a bitmask;
   Bit 0-1, flowcontrol */
#define SERIAL_MODE_FLOWCONTROL         0x03
#define SERIAL_MODE_FLOWCONTROL_NONE    0x00
#define SERIAL_MODE_FLOWCONTROL_RTSCTS  0x01
#define SERIAL_MODE_FLOWCONTROL_XONXOFF 0x02
#define serial_flowcontrol_none()       ((settings.serial_mode & SERIAL_MODE_FLOWCONTROL) == SERIAL_MODE_FLOWCONTROL_NONE)
#define serial_flowcontrol_rtscts()     ((settings.serial_mode & SERIAL_MODE_FLOWCONTROL) == SERIAL_MODE_FLOWCONTROL_RTSCTS)
#define serial_flowcontrol_xonxoff()    ((settings.serial_mode & SERIAL_MODE_FLOWCONTROL) == SERIAL_MODE_FLOWCONTROL_XONXOFF)
/* Bit 2, tx idle mode */
#define SERIAL_MODE_TXIDLE_HIGH         (1<<2)
#define serial_txidle_high()            ((settings.serial_mode & SERIAL_MODE_TXIDLE_HIGH)
#define serial_txidle_low()             ((settings.serial_mode & SERIAL_MODE_TXIDLE_HIGH == 0)
/* Bit 3-4, parity */
#define SERIAL_MODE_PARITY              0x18
#define SERIAL_MODE_PARITY_NONE         0x00
#define SERIAL_MODE_PARITY_ODD          0x08
#define SERIAL_MODE_PARITY_EVEN         0x10
#define serial_parity_none()            ((settings.serial_mode & SERIAL_MODE_PARITY) == SERIAL_MODE_FLOWCONTROL_NONE)
#define serial_parity_odd()             ((settings.serial_mode & SERIAL_MODE_PARITY) == SERIAL_MODE_PARITY_ODD)
#define serial_parity_even()            ((settings.serial_mode & SERIAL_MODE_PARITY) == SERIAL_MODE_PARITY_EVEN)

void settings_init(void);
bool settings_load(void);
void settings_default(void);
bool settings_store(void);
bool settings_usedhcp(void);
void settings_loadnetworkparameters(unsigned char ip[4], unsigned char mask[4], unsigned char gw[4]);

#endif /* SETTINGS_H */