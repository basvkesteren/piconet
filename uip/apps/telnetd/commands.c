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
#include "commands.h"
#include "shell.h"
#include "string.h"
#include "telnetd.h"
#include "std.h"
#include "settings.h"
#include "sd.h"
#include "ff.h"
#include "uip.h"
#include "../dhcpc/dhcpc.h"
#include "../seriald/seriald.h"

const commandentry_t commands[] = {
    {"ls",      command_ls},
    {"cat",     command_cat},

    {"seriald", command_seriald},

    {"ip",      command_ip},
    {"gw",      command_gw},
    {"netstat", command_netstat},

    {"write",   command_write},
    {"reboot",  command_reboot},
    {"version", command_version},
    {"?",       command_help},
    {"quit",    command_quit},

    /* Default action */
    {NULL,      command_unknown}
};

void command_ls(char *str)
{
    extern FATFS fatfs;
    FF_DIR dir;
    FRESULT result;
    FILINFO fno;
   
    if(f_mount(&fatfs, "", 0) != FR_OK) {
        shell_output("Filesystem error\n\r");
    }
    else {
        if(strlen(str) <= 3) {
            /* Show contents of root */
            result = f_opendir(&dir, "/");
        }
        else {
            /* Show contents of given directory */
            result = f_opendir(&dir, &str[3]);
        }
       
        if(result == FR_OK) {
            while(f_readdir(&dir, &fno) == FR_OK && fno.fname[0] != 0) {
                shell_output("%s\n\r", fno.fname);
            }
            f_closedir(&dir);
        }
        else {
            shell_output("No such directory\n\r");
        }
    }
}

void command_cat(char *str)
{
    extern FATFS fatfs;
    FIL file;
    unsigned int read;
    char *line;

    if(f_mount(&fatfs, "", 0) != FR_OK) {
        shell_output("Filesystem error\n\r");
    }
    else {
        if(strlen(str) > 4) {
            if(f_mount(&fatfs, "", 0) != FR_OK) {
                shell_output("Filesystem error\n\r");
            }
            else if(f_open(&file, &str[4], FA_READ) == FR_OK) {
                while ((line = telnetd_getline()) != NULL && f_read(&file, line, TELNETD_CONF_LINELEN, &read) == FR_OK && read > 0) {
                    line[read] = '\0';
                    telnetd_sendline(line);
                }
                f_close(&file);
                shell_output("\n\r--end-of-file--\n\r");
            }
            else {
                shell_output("Cannot open file\n\r");
            }
        }
        else {
            shell_output("Use 'cat x' to show contents of file 'x'\n\r");
        }
    }
}

void command_seriald(char *str)
{
    extern seriald_statistics_t seriald_statistics;
    unsigned char i;
    char *line;

    if(strncmp(str, "seriald baud ", 13) == 0) {
        if(strcmp(&str[13], "300") == 0) {
            settings.serial_baudrate = SERIAL_BAUDRATE_300;
        }
        else if(strcmp(&str[13], "1200") == 0) {
            settings.serial_baudrate = SERIAL_BAUDRATE_1200;
        }
        else if(strcmp(&str[13], "2400") == 0) {
            settings.serial_baudrate = SERIAL_BAUDRATE_2400;
        }
        else if(strcmp(&str[13], "4800") == 0) {
            settings.serial_baudrate = SERIAL_BAUDRATE_4800;
        }
        else if(strcmp(&str[13], "9600") == 0) {
            settings.serial_baudrate = SERIAL_BAUDRATE_9600;
        }
        else if(strcmp(&str[13], "19200") == 0) {
            settings.serial_baudrate = SERIAL_BAUDRATE_19200;
        }
        else if(strcmp(&str[13], "38400") == 0) {
            settings.serial_baudrate = SERIAL_BAUDRATE_38400;
        }
        else if(strcmp(&str[13], "57600") == 0) {
            settings.serial_baudrate = SERIAL_BAUDRATE_57600;
        }
        else if(strcmp(&str[13], "115200") == 0) {
            settings.serial_baudrate = SERIAL_BAUDRATE_115200;
        }
        else if(strcmp(&str[13], "230400") == 0) {
            settings.serial_baudrate = SERIAL_BAUDRATE_230400;
        }
        else {
            shell_output("Undefined baudrate\n\r");
        }
    }
    else if(strncmp(str, "seriald port ", 13) == 0) {
        seriald_shutdown();
        settings.network_port = strtoint(&str[13],10);
        seriald_init();
    }
    else if(strcmp(str, "seriald udp") == 0) {
        seriald_shutdown();
        settings.network_mode &= ~NETWORK_MODE_TCP;
        seriald_init();
    }
    else if(strcmp(str, "seriald tcp") == 0) {
        seriald_shutdown();
        settings.network_mode |= NETWORK_MODE_TCP;
        seriald_init();
    }
    else if(strncmp(str, "seriald parity ", 15) == 0 && strlen(str) == 16) {
        switch(str[15]) {
            case 'n':
                settings.serial_mode &= ~SERIAL_MODE_PARITY;
                settings.serial_mode |= SERIAL_MODE_PARITY_NONE;
                break;
            case 'o':
                settings.serial_mode &= ~SERIAL_MODE_PARITY;
                settings.serial_mode |= SERIAL_MODE_PARITY_ODD;
                break;
            case 'e':
                settings.serial_mode &= ~SERIAL_MODE_PARITY;
                settings.serial_mode |= SERIAL_MODE_PARITY_EVEN;
                break;
            default:
                shell_output("Undefined parity\n\r");
                break;
        }
    }
    else if(strncmp(str, "seriald flow ", 13) == 0 && strlen(str) == 14) {
        switch(str[13]) {
            case 'n':
                settings.serial_mode &= ~SERIAL_MODE_FLOWCONTROL;
                settings.serial_mode |= SERIAL_MODE_FLOWCONTROL_NONE;
                break;
            case 'h':
                settings.serial_mode &= ~SERIAL_MODE_FLOWCONTROL;
                settings.serial_mode |= SERIAL_MODE_FLOWCONTROL_RTSCTS;
                break;
            case 's':
                settings.serial_mode &= ~SERIAL_MODE_FLOWCONTROL;
                settings.serial_mode |= SERIAL_MODE_FLOWCONTROL_XONXOFF;
                break;
            default:
                shell_output("Undefined flowcontrol\n\r");
                break;
        }
    }
    else if(strcmp(str, "seriald statistics") == 0) {
        shell_output("ReTx:%d, Controller full:%d\n\r", seriald_statistics.retransmitted, seriald_statistics.controller_full);
        shell_output("Dropped uart:%d, net:%d\n\r", seriald_statistics.uart_dropped, seriald_statistics.net_dropped);
    } 
    else if(strlen(str) == 7) {
        if((line = telnetd_getline()) != NULL) {
            switch(settings.serial_baudrate) {
                case SERIAL_BAUDRATE_300:
                    i = sprintf(line, "300");
                    break;
                case SERIAL_BAUDRATE_1200:
                    i = sprintf(line, "1200");
                    break;
                case SERIAL_BAUDRATE_2400:
                    i = sprintf(line, "2400");
                    break;
                case SERIAL_BAUDRATE_4800:
                    i = sprintf(line, "4800");
                    break;
                case SERIAL_BAUDRATE_9600:
                    i = sprintf(line, "9600");
                    break;
                case SERIAL_BAUDRATE_19200:
                    i = sprintf(line, "19200");
                    break;
                case SERIAL_BAUDRATE_38400:
                    i = sprintf(line, "38400");
                    break;
                case SERIAL_BAUDRATE_57600:
                    i = sprintf(line, "57600");
                    break;
                case SERIAL_BAUDRATE_115200:
                    i = sprintf(line, "115200");
                    break;
                case SERIAL_BAUDRATE_230400:
                    i = sprintf(line, "230400");
                    break;
                default:
                    i = sprintf(line, "?");
                    break;
            }
            i += sprintf(&line[i], " baud, port %d ", settings.network_port);
            if(network_mode_tcp()) {
                i += sprintf(&line[i], "TCP");
            }
            else {
                i += sprintf(&line[i], "UDP");
            }
            i += sprintf(&line[i], ", flow ");
            if(serial_flowcontrol_none()) {
                i += sprintf(&line[i], "none");
            }
            else if(serial_flowcontrol_rtscts()) {
                i += sprintf(&line[i], "hw");
            }   
            else if(serial_flowcontrol_xonxoff() ) {
                i += sprintf(&line[i], "sw");
            }
            else {
                i += sprintf(&line[i], "?");
            }
            i += sprintf(&line[i], " parity ");
            if(serial_parity_none()) {
                i += sprintf(&line[i], "none");
            }
            else if(serial_parity_odd()) {
                i += sprintf(&line[i], "odd");
            }   
            else if(serial_parity_even() ) {
                i += sprintf(&line[i], "even");
            }
            else {
                i += sprintf(&line[i], "?");
            }
            i += sprintf(&line[i], "\n\r");

            telnetd_sendline(line);
        }
    }
    else {
        shell_output("Use one off 'seriald baud x', 'seriald port x',\n\r");
        shell_output("'seriald udp/tcp', 'seriald parity n/o/e' or\n\r");
        shell_output("'seriald flow n/h/s'.\n\r");
    }
}

void command_ip(char *str)
{
    extern dhcp_parameters_t parameters;
    extern uip_ipaddr_t uip_hostaddr, uip_netmask;

    if(strlen(str) == 34 && str[2] == ' ' && 
       str[6] == '.' && str[10] == '.' && str[14] == '.' && str[18] == '/' && 
       str[22] == '.' && str[26] == '.' && str[30] == '.') {
        /* command 'ip xxx.xxx.xxx.xxx/xxx.xxx.xxx.xxx' */
        if(settings_usedhcp()) {
            dhcpc_release();
        }
       
        /* Do a clean disconnect before changing our IP */
        telnetd_disconnect();

        settings.network_ip[0] = strtoint(&str[3], 10);
        settings.network_ip[1] = strtoint(&str[7], 10);
        settings.network_ip[2] = strtoint(&str[11], 10);
        settings.network_ip[3] = strtoint(&str[15], 10);
        settings.network_mask[0] = strtoint(&str[19], 10);
        settings.network_mask[1] = strtoint(&str[23], 10);
        settings.network_mask[2] = strtoint(&str[27], 10);
        settings.network_mask[3] = strtoint(&str[31], 10);

        if(settings_usedhcp()) {
            shell_output("DHCP request started\n\r");
            dhcpc_getlease();
        }
        else {
            settings_loadnetworkparameters(settings.network_ip, settings.network_mask, settings.network_gw);
        }
    }
    else if(strlen(str) == 2) {
        /* Show current configuration */
        if(settings_usedhcp()) {
            shell_output("Using DHCP, lease from %d.%d.%d.%d\n\r", parameters.serverid[0],
                                                                   parameters.serverid[1],
                                                                   parameters.serverid[2],
                                                                   parameters.serverid[3]);
        }
        else {
            shell_output("Using static address\n\r");
        }
        shell_output("Currently using ip %d.%d.%d.%d/%d.%d.%d.%d\n\r", uip_ipaddr1(uip_hostaddr),
                                                                       uip_ipaddr2(uip_hostaddr),
                                                                       uip_ipaddr3(uip_hostaddr),
                                                                       uip_ipaddr4(uip_hostaddr),
                                                                       uip_ipaddr1(uip_netmask),
                                                                       uip_ipaddr2(uip_netmask),
                                                                       uip_ipaddr3(uip_netmask),
                                                                       uip_ipaddr4(uip_netmask));
    }
    else {
        shell_output("Use 'ip xxx.xxx.xxx.xxx/xxx.xxx.xxx.xxx' to set,\n\r");
        shell_output("or 'ip' to get.\n\r");
    }
}

void command_gw(char *str)
{
    if(strlen(str) == 18 && str[2] == ' ' && str[6] == '.' && str[10] == '.' && str[14] == '.') {
        /* command 'gw xxx.xxx.xxx.xxx' */
        if(settings_usedhcp()) {
            shell_output("Using DHCP\n\r");
        }
        else {
            settings.network_gw[0] = strtoint(&str[8], 10);
            settings.network_gw[1] = strtoint(&str[12], 10);
            settings.network_gw[2] = strtoint(&str[16], 10);
            settings.network_gw[3] = strtoint(&str[20], 10);

            settings_loadnetworkparameters(settings.network_ip, settings.network_mask, settings.network_gw);
        }
    }
    else if(strlen(str) == 2) {
        /* Show current configuration */
        shell_output("Currently using gateway %d.%d.%d.%d\n\r", uip_ipaddr1(uip_draddr),
                                                                uip_ipaddr2(uip_draddr),
                                                                uip_ipaddr3(uip_draddr),
                                                                uip_ipaddr4(uip_draddr));
    }
    else {
        shell_output("Use 'gw xxx.xxx.xxx.xxx' to set, 'gw' to get.\n\r");
    }
}


void command_netstat(char *str)
{
    (void)str;

    #if UIP_STATISTICS
    extern struct uip_stats uip_stat;

    shell_output("IP packets received %d, send %d, dropped %d\n\r", uip_stat.ip.recv, uip_stat.ip.sent, uip_stat.ip.drop);
    shell_output("ICMP packets received %d, send %d, dropped %d\n\r", uip_stat.icmp.recv, uip_stat.icmp.sent, uip_stat.icmp.drop);
    shell_output("TCP packets received %d, send %d, dropped %d\n\r", uip_stat.tcp.recv, uip_stat.tcp.sent, uip_stat.tcp.drop);
    shell_output("TCP packets retransmitted %d, connection attempts %d\n\r", uip_stat.tcp.rexmit, uip_stat.tcp.synrst);
    #if UIP_UDP
    shell_output("UDP packets received %d, send %d, dropped %d\n\r", uip_stat.udp.recv, uip_stat.udp.sent, uip_stat.udp.drop);
    #endif
    #else
    shell_output("Statistics disabled\n\r");
    #endif
}

void command_write(char *str)
{
    (void)str;

    if(settings_store()) {
        shell_output("ok\n\r");
    }
    else {
        shell_output("failed\n\r");
    }
}

void command_reboot(char *str)
{
    (void)str;

    RESET();
}

void command_version(char *str)
{
    (void)str;
    extern unsigned long system_uptime;

    shell_output("PicoNet, software rev. %s\n\r", SVN_REV);
    #ifdef __SDCC
    shell_output("Build %s, %s with SDCC rev %d\n\r", __DATE__, __TIME__, __SDCC_REVISION);
    #endif
    #ifdef __XC8
    shell_output("Build %s, %s with XC8 version %d\n\r", __DATE__,__TIME__, __XC8_VERSION);
    #endif

    shell_output("Uptime %d minutes\n\r", system_uptime);
}

void command_help(char *str)
{
    (void)str;

    shell_output("ip\n\r");
    shell_output("gw\n\r");
    shell_output("netstat\n\r");

    shell_output("reboot\n\r");
    shell_output("version\n\r");
    shell_output("quit\n\r");
}

void command_quit(char *str)
{
    (void)str;

    shell_output("Bye");
    telnetd_disconnect();
}

void command_unknown(char *str)
{
    if(strlen(str)) {
        shell_output("Unknown command\n\r");
    }
}
