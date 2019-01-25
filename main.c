/*
    Piconet RS232 ethernet interface

    main.c
           
    Copyright (c) 2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
\file
Main code
*/
#include "cpusettings.h"
#include "config.h"
#include "io.h"
#include "delay.h"
#include "serial.h"
#include "ssp.h"
#include "enc28j60.h"
#include "enc28j60_freebuffer.h"
#include "25aa02e48.h"
#include "console.h"
#include "sd.h"
#include "settings.h"

#include "uip/uip.h"
#include "uip/uip_arp.h"
#include "uip/uip_split.h"
#include "uip/apps/dhcpc/dhcpc.h"
#include "uip/apps/seriald/seriald.h"

#include "fatfs/ff.h"

#ifdef __SDCC
#include <pic16/signal.h>
#include <pic16/string.h>
#endif

#include <stdlib.h>

/* Uptime in minutes, increased in the RTC minute interrupt handler */
unsigned long system_uptime;

static bool uip_periodic, uip_periodicarp;

#define BUF ((struct uip_eth_hdr *)&uip_buf[0])

FATFS fatfs;

static inline void init(void)
/*!
  Setup oscillator and I/O
*/
{
    /* Select 8MHz for the internal clock */
    OSCCONbits.IRCF0 = 1;
    OSCCONbits.IRCF1 = 1;
    OSCCONbits.IRCF2 = 1;
    /* Use primary clock source */
    OSCCONbits.SCS0 = 0;
    OSCCONbits.SCS1 = 0;

    #if defined(__SDCC_PIC18F27J53) || defined (__18F27J53)
    /* Wait until INTOSC is stable */
    while (!OSCCONbits.FLTS);
    #endif

    /* Setup PLL */
    OSCTUNEbits.PLLEN = 1;
    delay_ms(2);                // Wait for Trc, PLL start-up/lock time

    #if WATCHDOG != 0
    #warning Watchdog enabled
    WDTCONbits.SWDTEN = 1;
    #endif

    ANCON0 = 0xFF;              // All AD inputs..
    ANCON1 = 0x1F;              // ..as digital I/O

    UCONbits.USBEN = 0;	        // Disable USB module
    UCFGbits.UTRDIS = 1;        // Disable USB transceiver

    /* PORT A I/O usage:
        0       i       /INT
        1       o       /RESET
        2       i       CTS
        3       o       RTS
        4       i       - (not available)
        5       o       TX
        6       o       /CS_EEPROM
        7       o       /CS_SD  */
    LATA = 0xE1;                // INT and CS_* are active low, TX is high when idle. We actually assert RESET here!
    TRISA = 0x15;               // Set = input, clear = output

    /* PORT B I/O usage:
        0       i       MISO
        1       o       MOSI
        2       o       SCK
        3       o       /CS_ETH
        4       o       SCK_SD
        5       i       MISO_SD
        6       o       PGC/LED_RED
        7       o       PGD/LED_GREEN */
    LATB = 0x48;                // CS_* is active low. We turn LED_RED on here!
    TRISB = 0x21;               // Set = input, clear = output
    INTCON2bits.RBPU = 1;       // Disable pullups

    /* PORT C I/O usage:
        0       i       RX       
        1       i       CONSOLE_DIN
        2       o       CONSOLE_DOUT
        3       o       - (not available)
        4       o       D-       
        5       i       D+
        6       i       VBUS
        7       o       MOSI_SD */
    LATC = 0x04;                // CONSOLE_DOUT is high when idle
    TRISC = 0x63;               // Set = input, clear = output

    /* Set up PPS pins:
       Unlocking sequence */
    EECON2 = 0x55;
    EECON2 = 0xAA;
    PPSCONbits.IOLOCK = 0;

    /* Change settings */
    RPINR1 = 0;                 // Connect INT1 input to RP0 (RA0)
    RPINR16 = 11;               // Connect USART2 RX input (RX2) to RP11 (RC0)
    RPINR21 = 3;                // Connect SPI2 MISO input (SDI2) to RP3 (RB0)
    RPINR22 = 5;                // Connect SPI2 clock input (SCK2IN) to RP5 (RB2)

    #if defined(__SDCC_PIC18F27J53) || defined (__18F27J53)
    RPOR2 = 6;                  // Connect USART2 TX output (TX2) to RP2 (RA5)
    RPOR4 = 10;                 // Connect SPI2 MOSI output (SDO2) to RP4 (RB1)
    RPOR5 = 11;                 // Connect SPI2 clock output (SCK2) to RP5 (RB2)
    #else
    RPOR2 = 5;                  // Connect USART2 TX output (TX2) to RP2 (RA5)
    RPOR4 = 9;                  // Connect SPI2 MOSI output (SDO2) to RP4 (RB1)
    RPOR5 = 10;                 // Connect SPI2 clock output (SCK2) to RP5 (RB2)
    #endif

    /* Locking sequence */
    EECON2 = 0x55;
    EECON2 = 0xAA;
    PPSCONbits.IOLOCK = 1;

    /* Console */
    serial2_init();
    #ifdef __SDCC
    stdout = STREAM_USER;
    #endif

    /* RTC, setup alarm for a tick every minute */
    ALRMCFGbits.AMASK3 = 0;
    ALRMCFGbits.AMASK2 = 0;
    ALRMCFGbits.AMASK1 = 1;
    ALRMCFGbits.AMASK0 = 1;
    ALRMCFGbits.CHIME = 1;      // Interrupt roll-over enabled
    ALRMCFGbits.ALRMEN = 1;     // Alarm set
    EECON2 = 0x55;              // Clear write lock
    EECON2 = 0xAA;
    RTCCFGbits.RTCWREN = 1;     // Make RTCCFG register writeable, so that we can..
    RTCCFGbits.RTCEN = 1;       // ..enable the RTC
    RTCCFGbits.RTCWREN = 0;
    IPR3bits.RTCCIP = 0;        // Low priority interrupt..
    PIE3bits.RTCCIE = 1;        // ..enabled
    system_uptime = 0;

    /* Timer1, used as the overall system tick
       Using Fosc/4 as clock with an 1:2 prescaler means a timer-increment each 0.167uS, 
       60000 increments to get a 10ms interrupt-interval */
    T1CONbits.RD16 = 1;         // Read/write timer as one 16 bits value
    T1CONbits.T1CKPS1 = 0;      // Use a 1:2 prescaler
    T1CONbits.T1CKPS0 = 1;
    T1CONbits.T1OSCEN = 0;      // Disable timer1 oscillator
    T1CONbits.TMR1CS0 = 0;      // Use instruction clock..
    T1CONbits.TMR1CS1 = 0;      // ..as source
    TMR1H = 5536>>8;
    TMR1L = 5536&0xFF;          // Set counter; timer counts up, this value gives us 60000 increments before an interrupt
    IPR1bits.TMR1IP = 0;        // Low priority interrupt..
    PIE1bits.TMR1IE = 1;        // ..enabled
    T1CONbits.TMR1ON = 1;       // Start timer

    /* SSP1 interface, storage */
    ssp1_init();

    /* SSP2 interface, ethernet controller and EEPROM */
    ssp2_init();

    /* INT1, ethernet controller interrupt */
    INTCON2bits.INTEDG1 = 0;    // Interrupt on falling edge
    INTCON3bits.INT1IP = 0;     // Low priority interrupt..
    INTCON3bits.INT1IE = 1;     // ..enabled
}

static void systick(void)
/*!
  Periodic system tick, 10ms interval
*/
{
    static unsigned char uipcounter=0;
    static unsigned char uiparpcounter=0;

    uipcounter++;
    if(uipcounter == 50) {
        uipcounter = 0;
        /* IP/UDP tasks, once per 1/2 second */
        uip_periodic = TRUE;
        uiparpcounter++;
        if(uiparpcounter == 20) {
            uiparpcounter = 0;
            /* ARP tasks, once per 10 seconds */
            uip_periodicarp = TRUE;
        }
    }
}

unsigned int ticks(void)
/*!
  Get periodic system tick
*/
{
    unsigned int i;

    i = TMR1L;
    i |= TMR1H<<8;

    return i;
}

extern int crctest(void);

void main(void)
/*!
  Startpoint of C-code
*/
{
    char blink=0;
    char limit=15;
    char direction=1;
    char prescaler=15;
    short offscaler=0;
    unsigned int ledblink=0;

    unsigned char mac[6];
    unsigned char i;

    FRESULT ffres;

    extern cardinfo_t cardinfo;

    /* Low level init; oscillator, watchdog, I/O */
    init();

    /* Hello world */
    dprint("PicoNet, software rev. %s\n\r", SVN_REV);
    #ifdef __SDCC
    dprint("Build %s, %s with SDCC rev %d\n\r", __DATE__, __TIME__, __SDCC_REVISION);
    #endif
    #ifdef __XC8
    dprint("Build %s, %s with XC8 version %d\n\r", __DATE__,__TIME__, __XC8_VERSION);
    #endif

    /* Initialize Ethernet controller hardware */
    if(eeprom_25aa02e48_getEUI48(mac)) {
        dprint("EUI48: %2X:%2X:%2X:%2X:%2X:%2X\n\r", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    else {
        dprint("Could not read EUI48\n\r");
    }
    if(enc28j60_init(mac) == FALSE) {
        dprint("Could not initialise network controller\n\r");
    }

    /* Load settings */
    if(settings_load() == FALSE) {
        dprint("Could not load settings, using defaults\n\r");
    }

    /* Set baudrate as configured */
    serial2_setbaudrate(settings.serial_baudrate);
    /* Keep interrupt disabled untill needed */
    serial2_int_suspend();

    /* Setup network stack */
    uip_periodic = FALSE;
    uip_periodicarp = FALSE;
    uip_setethaddr(mac);
    uip_init();
    uipapp_init();
    #ifdef DHCPC
    if(settings_usedhcp()) {
        /* Using DHCP */
        dhcpc_getlease();
    }
    else 
    #endif
    {
        /* Using fixed network-settings */
        settings_loadnetworkparameters(settings.network_ip, settings.network_mask, settings.network_gw);
    }

    /* Setup filesystem */
    ffres = f_mount(&fatfs, "", 1);
    if(ffres == FR_OK) {
        dprint("SD-card mount OK\n\r");
    }
    else {
        dprint("SD-card mount failed\n\r");
        if(ffres == FR_NO_FILESYSTEM) {
            dprint("No valid filesystem found\n\r");
        }
    }
    if(ffres == FR_OK || ffres == FR_NO_FILESYSTEM) {
        dprint("type %d, %ldMB, filesystem %d\n\r", cardinfo.type, cardinfo.size, fatfs.fs_type);
    }
    
    /* Enable global interrupts */
    RCONbits.IPEN = 1;          /* enable priority interrupts */
    INTCONbits.GIE = 1;         /* enable global interrupts */
    INTCONbits.PEIE = 1;        /* enable peripheral interrupts */

    led_red_off();

    while(1) {
        /* Status-LED, do some fancy blinking */
        if(abs(ticks() - ledblink) & 0x2000) {
            ledblink = ticks();

            if(offscaler == 0) {
                /* Update counter */
                prescaler++;
                if(prescaler == 31) {
                    prescaler = 0;
                }

                /* Update LED state */
                if(prescaler == 0 && blink == 0) {
                    led_green_on();
                    blink = 1;
                }
                else if(prescaler == limit && blink == 1) {
                    led_green_off();
                    blink = 0;

                    /* Move limit */
                    if(direction) {
                        limit++;
                        if(limit == 29) {
                            direction = 0;
                        }
                    }
                    else {
                        limit--;
                        if(limit == 1) {
                            direction = 1;
                            /* Wait a bit */
                            offscaler = 750;
                        }
                    }
                }
            }
            else {
                /* We're in the off-state */
                offscaler--;
            }
        }

        /* Incoming data from the network controller */
        while(enc28j60_pendingpackets()) {
            /* Read new packet from the ethernet controller */
            if((uip_len = enc28j60_get(uip_buf))) {
                /* And let the network stack do it's magic */
                if(BUF->type == htons(UIP_ETHTYPE_IP)) {
                    uip_input();
                    if(uip_len > 0) {
                        uip_arp_out();
                        uip_split_output();
                    }
                }
                else if(BUF->type == htons(UIP_ETHTYPE_ARP)) {
                    uip_arp_arpin();
                    if(uip_len > 0) {
                        uip_output();
                    }
                }
            }
        }

        if(seriald_shouldtransfer()) {
            serial2_int_suspend();
            seriald_switchbuffers();
            serial2_int_resume();

            seriald_transfer();
        }
        
        /* Periodic network tasks */
        if(uip_periodic) {
            /* Once every 1/2 second */
            for(i = 0; i < UIP_CONNS; i++) {
                uip_periodic(i);
                if(uip_len > 0) {
                    uip_arp_out();
                    uip_split_output();
                }
            }

            #if UIP_UDP
            for(i = 0; i < UIP_UDP_CONNS; i++) {
                uip_udp_periodic(i);
                if(uip_len > 0) {
                    uip_arp_out();
                    uip_output();
                }
            }
            #endif
            uip_periodic = FALSE;

            if(uip_periodicarp) {
                /* Once every 10 seconds */
                uip_arp_timer();
                uip_periodicarp = FALSE;
            }
        }

        CLEAR_WATCHDOG();
    }
}

void network_linkchange(void)
/*!
  Network controller detected a link change
*/
{
    dprint("Link ");
    if(enc28j60_link()) {
        dprint("up");
        #ifdef DHCPC
        if(settings_usedhcp()) {
            dhcpc_getlease();
        }
        #endif
    }
    else {
        dprint("down");
        uipapp_disconnect();
        #ifdef DHCPC
        if(settings_usedhcp()) {
            dhcpc_release();
        }
        #endif
    }
    dprint("\n\r");
}

#ifdef DHCPC
void network_dhcpupdate(dhcp_parameters_t *parameters)
/*!
  DHCP client received an offer
*/
{
    if(parameters->ipaddr[0] && parameters->ipaddr[1] && parameters->ipaddr[2] && parameters->ipaddr[3]) {
        settings_loadnetworkparameters(parameters->ipaddr, parameters->netmask, parameters->router);       
    }
}
#endif

void uip_output(void)
{
    enc28j60_put(uip_buf, uip_len);
}

void uip_output_incontroller(const u16_t offset, const u8_t header_length, const u16_t payload_length)
{
    enc28j60_put_freebuffer(offset, uip_buf, header_length, payload_length);
}

void seriald_connected(void)
/*!
  uIP seriald application has a client
*/
{
    serial2_int_resume();
}

void seriald_disconnected(void)
/*!
  uIP seriald application does not have a client
*/
{
    serial2_int_suspend();
}

void seriald_received(const char *data, const unsigned int length)
/*
  uIP seriald application received data
*/
{
    unsigned int i;

    for(i=0;i<length;i++) {
        serial2_putchar(data[i]);
    }
}

void uip_log(const char *msg)
/*!
  Network stack logging; set UIP_CONF_LOGGING in uip-conf.h to enable this
*/
{
    dprint("uIP: %s\n\r", msg);
}

#ifdef __SDCC
void putchar (char c) __wparam
#endif
#ifdef __XC8
void putch(char c)
#endif
/*!
  This function is called by printf to print a character
*/
{
    serial2_putchar(c);
}

#ifdef __SDCC
void isr_high(void) __interrupt 1
#endif
#ifdef __XC8
void high_priority interrupt isr_high(void)
#endif
/*!
  High priority interrupts
*/
{
    unsigned char clear;

    /* UART2 */
    if(RCSTA2bits.OERR) {
        /* Overrun error. Clear by toggeling RCSTA.CREN */
        RCSTA2bits.CREN = 0;
        RCSTA2bits.CREN = 1;
        seriald_dropped();
    }
    else if(RCSTA2bits.FERR) {
        /* Framing error. Clear by reading RCREG */
        clear = RCREG2;
    }
    else {
        seriald_incoming(RCREG2);
    }
}

#ifdef __SDCC
void isr_low(void) __interrupt 2
#endif
#ifdef __XC8
void low_priority interrupt isr_low(void)
#endif
/*!
  Low priority interrupts
*/
{
    if(INTCON3bits.INT1IF && INTCON3bits.INT1IE) {
        /* INT1, ENC28J60 interrupt pin */
        INTCON3bits.INT1IF = 0;
        enc28j60_int();
    }
    else if(PIR1bits.TMR1IF) {
        /* Timer1, setup for an interrupt each 10 ms */
        PIR1bits.TMR1IF = 0;
        TMR1H = 5536>>8;
        TMR1L = 5536&0xFF;
        systick();
    }
    else if(PIR3bits.RTCCIF) {
        /* RTC, setup for one interrupt per minute */
        system_uptime++;
        PIR3bits.RTCCIF = 0;
    }
    else if(PIR2bits.LVDIF) {
        /* Low voltage detect */
        RESET();
    }
    else if(PIR2bits.BCL1IF) {
        /* Bus-collision */
        RESET();
    }
    else {
        dprint("Unhandled low priority interrupt\n\r");
        RESET();
    }
}
