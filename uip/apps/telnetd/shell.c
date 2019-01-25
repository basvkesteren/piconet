/*
    Piconet RS232 ethernet interface

    shell.c
           
    Copyright (c) 2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
 * \file
 * Telnet shell
 */
#include "shell.h"
#include "telnetd.h"
#include "commands.h"
#include <std.h>
#include <uip.h>
#include <string.h>
#include <stdarg.h> // For va_arg and friends

void shell_start(void)
{
    shell_output("Piconect shell\n\r");
    shell_output("Type '?' and return for help\n\r> ");
}

void shell_input(char *command)
{
    extern const commandentry_t commands[];
    unsigned char i=0;
    char *line;

    do{
        if(strncmp(commands[i].string, command, strlen(commands[i].string)) == 0) {
            break;
        }
        i++;
    } while(commands[i].string != NULL);

    commands[i].function(command);
    if((line = telnetd_getline()) != NULL) {
        line[0] = '>';
        line[1] = ' ';
        line[2] = '\0';
        telnetd_sendline(line);
    }
}

void shell_output(const char *fmt, ...)
{
    char *line;
    int outputcounter=0;
    char* string;
  
    if((line = telnetd_getline()) != NULL) {
        /* Create an object containing all given arguments */
        va_list argpointer;

        /* Initialize argpointer, so it points to the first argument given */
        va_start(argpointer, fmt);

        /* Walk through al the given arguments */
        for(;*fmt;fmt++) {
            /* Is it a variable? */
            if(*fmt != '%') {
                /* Nope, so just print it */
                line[outputcounter]=*fmt;
                outputcounter++;
            }
            else {
                /* It is.. What kind? */
                switch(*++fmt) {
                    case 's':
                        /* string */
                        for(string=va_arg(argpointer, char *); *string; string++) {
                            /* Walk through the entire string, printing each character */
                            line[outputcounter]=*string;
                            outputcounter++;
                        }
                        break;
                    case 'd':
                        /* base-10 number */
                        outputcounter+=inttostrn(va_arg(argpointer, int),&line[outputcounter],TELNETD_CONF_LINELEN-outputcounter, 10);
                        break;
                }
            }
            if(outputcounter+2 == TELNETD_CONF_LINELEN) {
                line[outputcounter]='\n';
                outputcounter++;
                break;
            }
        }
        /* Close string */
        line[outputcounter]='\0';

        /* free memory n' stuff.. */
        va_end(argpointer);

        telnetd_sendline(line);
    }
}
