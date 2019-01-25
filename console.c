/*
    Piconet RS232 ethernet interface

    console.c
           
    Copyright (c) 2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
\file
Simple line console
*/
#include "console.h"
#include "settings.h"

void console_incoming(unsigned char c)
{
    extern unsigned long system_uptime;
    static unsigned char commandlength=0;
    static char command[1];

    /* Determine what to do next.. */
    if(c == 0x0A || c == 0x0D) {
        /* End-of-command. Figure out what it is */
        printf("\r");
        if(commandlength == 1) {
            switch(command[0]) {
                case 'd':
                    settings_default();
                    printf("ok\n\r");
                    break;
                case 'r':
                    RESET();
                    break;
                case 'v':
                    printf("software rev. %s\n\r", SVN_REV);
                    printf("uptime %ld minutes\n\r", system_uptime);
                    break;
                case 'w':
                    if(settings_store()) {
                        printf("ok\n\r");
                    }
                    else {
                        printf("failed\n\r");
                    }
                    break;
                case '?':
                    printf("d -> load [d]efaults\n\r");
                    printf("r -> [r]eboot\n\r");
                    printf("v -> show [v]ersion and uptime\n\r");
                    printf("w -> [w]rite settings to EEPROM\n\r");
                    printf("\n\r");
                    break;
                default:
                    printf("?\n\r");
                    break;
            }
        }
        else {
            printf("?\n\r");
        }
        commandlength = 0;
        printf(">");
    }
    else if(commandlength == sizeof(command)) {
        printf("too long\n\r>");
        commandlength = 0;
    }
    else {
        command[commandlength] = c;
        printf("%c", command[commandlength]);
        commandlength++;
    }
}