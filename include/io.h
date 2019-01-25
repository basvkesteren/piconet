/*
    Piconet RS232 ethernet interface

    io.h
           
    Copyright (c) 2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
\file
I/O function definitions and macro's
*/
#ifndef IO_H
#define IO_H

#include "config.h"

/*
 * Status LED
 */

#define led_red_on()                do { \
                                        LATBbits.LATB6 = 1; \
                                    } while(0);

#define led_red_off()               do { \
                                        LATBbits.LATB6 = 0; \
                                    } while(0);
                                    
#define led_green_on()              do { \
                                        LATBbits.LATB7 = 1; \
                                    } while(0);
                                    
#define led_green_off()             do { \
                                        LATBbits.LATB7 = 0; \
                                    } while(0);

/*
 * ENC28J60 pins
 */
 
#define enc28j60_cs_assert()        do{ \
                                        LATBbits.LATB3 = 0; \
                                    } while(0)

#define enc28j60_cs_deassert()      do{ \
                                        LATBbits.LATB3 = 1; \
                                    } while(0)

#define enc28j60_reset_assert()     do{ \
                                        LATAbits.LATA1 = 0; \
                                    } while(0)

#define enc28j60_reset_deassert()   do{ \
                                        LATAbits.LATA1 = 1; \
                                    } while(0)

/*
 * 25AA02E48 EEPROM pins
 */
 
#define eeprom_cs_assert()          do{ \
                                        LATAbits.LATA6 = 0; \
                                    } while(0)

#define eeprom_cs_deassert()        do{ \
                                        LATAbits.LATA6 = 1; \
                                    } while(0)

/*
 * SD card pins
 */

#define sd_cs_assert()              do{ \
                                        LATAbits.LATA7 = 0; \
                                    } while(0)

#define sd_cs_deassert()            do{ \
                                        LATAbits.LATA7 = 1; \
                                    } while(0)

/*
 * RS232 flowcontrol pins
 */

#define serial_rts_assert()         do{ \
                                        LATAbits.LATA3 = 0; \
                                    } while(0)

#define serial_rts_deassert()       do{ \
                                        LATAbits.LATA3 = 1; \
                                    } while(0)

#define serial_cts()                PORTAbits.RA2

#endif /* IO_H */
