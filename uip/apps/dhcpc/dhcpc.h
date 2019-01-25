/*
    Piconet RS232 ethernet interface

    dhcpc.h
           
    Copyright (c) 2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
\file
DHCP client
*/
#ifndef DHCPC_C
#define DHCPC_C

typedef struct {
    unsigned char serverid[4];
    unsigned char ipaddr[4];
    unsigned char netmask[4];
    unsigned char router[4];
    //unsigned char ntp[4];
    unsigned int leasetime[2];
} dhcp_parameters_t;

void dhcpc_init(void);
void dhcpc_getlease(void);
void dhcpc_appcall(void);
void dhcpc_renew(void);
void dhcpc_release(void);
char dhcpc_running(void);
char dhcpc_gotlease(void);
dhcp_parameters_t* dhcpc_getparameters(void);

#endif /* DHCPC_C */

