/*
    Piconet RS232 ethernet interface

    shell.h
           
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
#ifndef SHELL_H
#define SHELL_H

void shell_start(void);
void shell_input(char *command);
void shell_output(const char *fmt, ...);

#endif /* SHELL_H */
