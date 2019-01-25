/*
    Piconet RS232 ethernet interface

    app.h
           
    Copyright (c) 2006,2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
\file
UIP_APPCALL multiplexer 
*/
#ifndef APP_H
#define APP_H

#ifdef TELNETD
#include "telnetd/telnetd.h"
#endif
#ifdef DHCPC
#include "dhcpc/dhcpc.h"
#endif
#ifdef SERIALD
#include "seriald/seriald.h"
#endif

/**
 * \var #define UIP_APPCALL
 *
 * The name of the application function that uIP should call in
 * response to TCP/IP events.
 *
 */
#define UIP_APPCALL     uip_appcall

/**
 * \var #define UIP_UDP_APPCALL
 *
 * The name of the application function that uIP should call in
 * response to UDP events.
 *
 */
#define UIP_UDP_APPCALL uipudp_appcall

/**
 * \var typedef uip_tcp_appstate_t
 *
 * The type of the application state that is to be stored in the
 * uip_conn structure. This usually is typedef:ed to a struct holding
 * application state information.
 */

/*
// We don't have UDP application's that require state-data
typedef struct {
    union {
    } _u;
} uip_udp_appstate_t;
*/
typedef char uip_tcp_appstate_t;

/**
 * \var typedef uip_udp_appstate_t
 *
 * The type of the application state that is to be stored in the
 * uip_conn structure. This usually is typedef:ed to a struct holding
 * application state information.
 */
/*
typedef struct uip_tcp_appstate {
    union _u {
        #ifdef TELNETD
        struct telnetd_state _telnetd;
        #endif
        char c;
    } _u;
} uip_tcp_appstate_t;
*/
typedef char uip_udp_appstate_t;

void uipapp_init(void);
void uip_appcall(void);
void uipudp_appcall(void);
void uipapp_disconnect(void);

#endif /* APP_H */
