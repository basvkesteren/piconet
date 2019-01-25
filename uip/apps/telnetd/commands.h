/*
    Piconet RS232 ethernet interface

    commands.c
           
    Copyright (c) 2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
 * \file
 * Telnet server commands
 */
#ifndef COMMANDS_H
#define COMMANDS_H

#include "config.h"

typedef struct {
    const char *string;
    void (* function)(char *str);
} commandentry_t;

void command_parser(char *command);

void command_ls(char *str);
void command_cat(char *str);
void command_seriald(char *str);
void command_ip(char *str);
void command_gw(char *str);
void command_netstat(char *str);
void command_write(char *str);
void command_reboot(char *str);
void command_version(char *str);
void command_quit(char *str);
void command_help(char *str);
void command_unknown(char *str);

#endif /* COMMANDS_H */
 

