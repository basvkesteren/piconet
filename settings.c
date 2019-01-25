/*
    Piconet RS232 ethernet interface

    settings.c
           
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
#include "settings.h"
#include "25aa02e48.h"
#include "uip.h"

settings_t settings;

void settings_init(void)
{

}

bool settings_load(void)
{
    unsigned char checksum = 0;
    unsigned char i;

    /* Read configuration.. */
    for(i=0;i<sizeof(settings);i++) {
        ((unsigned char *)&settings)[i] = eeprom_25aa02e48_readbyte(i);
        checksum += ((unsigned char *)&settings)[i];
    }
    /* ..and checksum */
    if(eeprom_25aa02e48_readbyte(i) == checksum) {
        return TRUE;
    }
    else {
        /* No good.. Load defaults */
        settings_default();
    }

    return FALSE;
}

void settings_default(void)
{
    unsigned char i;

    /* Clear everything */
    for(i=0;i<sizeof(settings);i++) {
        ((unsigned char *)&settings)[i] = 0;
    }
    /* Load some defaults */
    settings.serial_baudrate = SERIAL_BAUDRATE_9600;
}

bool settings_store(void)
{
    unsigned char checksum = 0;
    unsigned char i;

    for(i=0;i<sizeof(settings);i++) {
        if(eeprom_25aa02e48_readbyte(i) != ((unsigned char *)&settings)[i]) {
            if(eeprom_25aa02e48_writebyte(i, ((unsigned char *)&settings)[i]) == FALSE) {
                /* Could not write byte */
                return FALSE;
            }
        }
        checksum += ((unsigned char *)&settings)[i];
    }

    /* Update checksum */
    if(eeprom_25aa02e48_readbyte(i) == checksum || eeprom_25aa02e48_writebyte(i, checksum)) {
        /* Checksum did not need updating, or was just sucessfully updated */
        return TRUE;
    }

    return FALSE;
}

bool settings_usedhcp(void)
{
    return settings.network_ip[0] == 0 && settings.network_ip[1] == 0 && settings.network_ip[2] == 0 && settings.network_ip[3] == 0;
}

void settings_loadnetworkparameters(unsigned char ip[4], unsigned char mask[4], unsigned char gw[4])
{
    uip_ipaddr_t ipaddr;

    if(uip_ipaddr1(uip_hostaddr) != ip[0] || 
       uip_ipaddr2(uip_hostaddr) != ip[1] || 
       uip_ipaddr3(uip_hostaddr) != ip[2] || 
       uip_ipaddr4(uip_hostaddr) != ip[3]) {
        dprint("DHCP update:\n\r");
        dprint(" addr:%i.%i.%i.%i\n\r", ip[0], ip[1], ip[2], ip[3]);
        dprint(" mask:%i.%i.%i.%i\n\r", mask[0], mask[1], mask[2], mask[3]);
        dprint(" gw:  %i.%i.%i.%i\n\r", gw[0], gw[1], gw[2], gw[3]); 
    }
   
    uip_ipaddr(ipaddr, ip[0], ip[1], ip[2], ip[3]);
    uip_sethostaddr(ipaddr);
    uip_ipaddr(ipaddr, mask[0], mask[1], mask[2], mask[3]);
    uip_setnetmask(ipaddr);
    uip_ipaddr(ipaddr, gw[0], gw[1], gw[2], gw[3]);
    uip_setdraddr(ipaddr);
}