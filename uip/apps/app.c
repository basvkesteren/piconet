/*
    Piconet RS232 ethernet interface

    app.c
           
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
#include "app.h"
#include "uip.h"
#include "settings.h"
#include "debug.h"

//#define APP_DEBUG

void uipapp_init(void)
{
    #ifdef DHCPC
    dhcpc_init();
    #endif
    #ifdef TELNETD
    telnetd_init();
    #endif
    #ifdef SERIALD
    seriald_init();
    #endif
}

void uip_appcall(void)
{
    #ifdef APP_DEBUG
    if(uip_newdata()) {
        dprint("Incoming TCP data at port %d from %d.%d.%d.%d\n\r", HTONS(uip_conn->lport),
                                                                    uip_ipaddr1(uip_conn->ripaddr), uip_ipaddr2(uip_conn->ripaddr), uip_ipaddr3(uip_conn->ripaddr), uip_ipaddr4(uip_conn->ripaddr));
    }
    #endif

    #ifdef TELNETD
    if(uip_conn->lport == HTONS(23)) {
        telnetd_appcall();
    }
    #endif

    #ifdef SERIALD
    if(uip_conn->lport == HTONS(settings.network_port)) {
        if(network_mode_tcp()) {
            seriald_appcall();
        }
    }
    #endif
}

void uipudp_appcall(void)
{ 
    #ifdef APP_DEBUG
    if(uip_newdata()) {
        dprint("Incoming UDP data at port %d from %d.%d.%d.%d\n\r", HTONS(uip_udp_conn->lport), 
                                                                    uip_ipaddr1(uip_udp_conn->ripaddr), uip_ipaddr2(uip_udp_conn->ripaddr), uip_ipaddr3(uip_udp_conn->ripaddr), uip_ipaddr4(uip_udp_conn->ripaddr));
    }
    #endif

    #ifdef SERIALD
    if(uip_conn->lport == HTONS(settings.network_port)) {
        if(network_mode_udp()) {
            seriald_appcall();
        }
    }
    #endif

    #ifdef DHCPC
    dhcpc_appcall();
    #endif
}

void uipapp_disconnect(void)
{
    #ifdef TELNETD
    telnetd_disconnect();
    #endif
    #ifdef SERIALD
    seriald_disconnect();
    #endif
}