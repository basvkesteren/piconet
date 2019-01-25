/*
    Piconet RS232 ethernet interface

    enc28j60.c
           
    Copyright (c) 2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
\file
ENC28J60 driver
*/
#include "enc28j60.h"
#include "enc28j60_registers.h"
#include <delay.h>
#include <debug.h>
#include <string.h> //memcpy
#include <io.h>
#include <ssp.h>

/* To debug or not to debug.. */
//#define ENC28J60_DEBUG

//#define ENC28J60_DEBUG_TRANSFERS

/* Which ssp interface to use? */
#define encspi_put(data)     ssp2_put(data)
#define encspi_get()         ssp2_get()

/* SPI command set */
/* Read/Write any of the ETH, MAC and MII registers */
#define CMD_RCR         0x00    /* Read Control Register; bit 0-4 contain the register address */
#define CMD_WCR         0x40    /* Write Control Register; bit 0-4 contain the register address */
/* Read/write to/from the internal transmit/receive buffer */
#define CMD_RBM         0x3A    /* Read Buffer Memory */
#define CMD_WBM         0x7A    /* Write Buffer Memory */
/* Set/clear bits in any of the ETH registers */
#define CMD_BFS         0x80    /* Bit Field Set; bit 0-4 contain the register address */
#define CMD_BFC         0xA0    /* Bit Field Clear; bit 0-4 contain the register address */
/* Software reset */
#define CMD_SC          0xFF    /* Soft Reset */

/* Local functions */
static void bankselect(const unsigned char bank);
static void setcontrolbit(const unsigned char address, const unsigned char bank, const unsigned char bitmask);
static void clearcontrolbit(const unsigned char address, const unsigned char bank, const unsigned char bitmask);
static unsigned char readcontrolregister(const unsigned char address);
static void writecontrolregister(const unsigned char address, const unsigned char value);
static unsigned short readphyregister(const unsigned char address);
static void writephyregister(const unsigned char address, const unsigned short value);
static void readbuffermemory(unsigned char *data, unsigned short length);

/* Local variables */
static unsigned char currentbank;               /* The currently selected register-bank */
static bool halfduplex;                         /* TRUE when in half duplex mode, FALSE when in full duplex mode; see enc28j60_setduplex() */
static unsigned short txstatusvectorlocation;   /* Where to read the statusvector when the transmission is completed; see enc28j60_put() */
static bool linkstate;                          /* Link up or down? Updated in enc28j60_int() */
static unsigned char pendingpackets;            /* No. of pending packets. Incremented in enc28j60_int(), decremented in enc28j60_get() */
static bool bufferfull;                         /* TRUE when buffer is full; set in enc28j60_int(), cleared in enc28j60_get() */
static volatile bool transmitting;              /* Set by enc28j60_put(), cleared in enc28j60_int() when the transmission completes. NOTE: volatile, because optimization breaks the logic in enc28j60_put() */
static unsigned char txstatusvector[7];         /* The last received TX-statusvector is stored here */
static unsigned char rxstatusvector[6];         /* The last received RX-statusvector is stored here */

bool enc28j60_init(const unsigned char MAC[8])
/*!
  Initialize the ENC28J60 in half-duplex mode with the given MAC address.
  
  Function will return TRUE when the initialization ran without errors, FALSE otherwise
*/
{
    unsigned char i;

    /* We start in bank 0 */
    currentbank = BANK0;

    /* Reset chip with a soft-reset */
    enc28j60_reset_deassert();
    enc28j60_cs_assert();
    encspi_put(CMD_SC);
    enc28j60_cs_deassert();

    /* Wait for at least 1 ms (See Erreta rev. B7, note 2) */
    delay_ms(1);

    /* Read revision ID */
    bankselect(BANK3);
    i = readcontrolregister(EREVID);
    #ifdef ENC28J60_DEBUG
    dprint("enc28j60_init(): REVID=0x%x\n\r", i);
    #endif
    if(i == 0 || i == 0xFF) {
        #ifdef ENC28J60_DEBUG
        dprint("enc28j60_init(): Invalid REVID\n\r");
        #endif
        return FALSE;
    }

    /* Initialize the ethernet buffer
       Start of reception buffer */
    bankselect(BANK0);
    writecontrolregister(ERXSTL, RXSTART & 0xFF);
    writecontrolregister(ERXSTH, (RXSTART>>8) & 0xFF);
    /* End of reception buffer */
    writecontrolregister(ERXNDL, RXEND & 0xFF);
    writecontrolregister(ERXNDH, (RXEND>>8) & 0xFF);
    /* Reception read pointer */
    writecontrolregister(ERXRDPTL, RXSTART & 0xFF);
    writecontrolregister(ERXRDPTH, (RXSTART>>8) & 0xFF);
    /* Receive filter; only accept packets with our MAC or the broadcast MAC as destination and a valid CRC */
    writecontrolregister(ERXFCON, ERXFCON_ANDOR | ERXFCON_BCEN | ERXFCON_UCEN | ERXFCON_CRCEN);

    /* Initialize the MAC
       First get it out of reset */
    bankselect(BANK2);
    writecontrolregister(MACON2, MACON2_MARST);
    /* Enable reception */
    writecontrolregister(MACON1, MACON1_MARXEN);
    /* Short frames padded to 60 bytes and CRC appended, enable frame length checking */
    writecontrolregister(MACON3, MACON3_PADCFG0 | MACON3_TXCRCEN | MACON3_FRMLNEN);
    /* Enable preamble checking of received packets */
    writecontrolregister(MACON4, MACON4_PUREPRE);
    /* Set max. framelength */
    writecontrolregister(MAMXFLL, NETWORK_MAXPACKETLENGTH & 0xFF);
    writecontrolregister(MAMXFLH, (NETWORK_MAXPACKETLENGTH>>8) & 0xFF);
    /* Back-to-back inter-packet gap */
    writecontrolregister(MABBIPG, 0x12);
    /* Non Back-to-back inter-packet gap */
    writecontrolregister(MAIPGL, 0x12);
    writecontrolregister(MAIPGH, 0x0C);
    /* Set MAC address */
    bankselect(BANK3);
    writecontrolregister(MAADR0, MAC[5]);
    writecontrolregister(MAADR1, MAC[4]);
    writecontrolregister(MAADR2, MAC[3]);
    writecontrolregister(MAADR3, MAC[2]);
    writecontrolregister(MAADR4, MAC[1]);
    writecontrolregister(MAADR5, MAC[0]);
    
    /* Read back MAC address */
    if(readcontrolregister(MAADR0) != MAC[5] ||
       readcontrolregister(MAADR1) != MAC[4] ||
       readcontrolregister(MAADR2) != MAC[3] ||
       readcontrolregister(MAADR3) != MAC[2] ||
       readcontrolregister(MAADR4) != MAC[1] ||
       readcontrolregister(MAADR5) != MAC[0]) {
        #ifdef ENC28J60_DEBUG
        dprint("enc28j60_init(): MAC readback verification failed\n\r");
        #endif
        return FALSE;
    }

    /* Initialize the PHY
       Half-duplex mode */
    writephyregister(PHCON1, 0);
    halfduplex = TRUE;
    /* Disable loopback */
    writephyregister(PHCON2, PHCON2_HDLDIS);

    /* We don't have a link, yet */
    linkstate = FALSE;

    /* No packets pending either */
    pendingpackets = 0;
    bufferfull = FALSE;
    transmitting = FALSE;

    /* Enable interrupts (all except for WOLIE) */
    writephyregister(PHIE, PHIE_PLNKIE | PHIE_PGEIE);
    #if ENC28J60_RX_POLL
    writecontrolregister(EIE, EIE_INTIE | EIE_DMAIE | EIE_LINKIE | EIE_TXIE | EIE_TXERIE | EIE_RXERIE);
    #else
    writecontrolregister(EIE, EIE_INTIE | EIE_PKTIE | EIE_DMAIE | EIE_LINKIE | EIE_TXIE | EIE_TXERIE | EIE_RXERIE);
    #endif

    /* Enable reception */
    writecontrolregister(ECON1, ECON1_RXEN);

    return TRUE;
}

void enc28j60_setduplex(const bool full)
/*!
  Switch duplex mode (half or full)
*/
{
    enc28j60_int_suspend();

    bankselect(BANK2);
    if(full) {
        /* Switch to full duplex mode */
        writecontrolregister(MACON1, MACON1_MARXEN | MACON1_TXPAUS | MACON1_RXPAUS);
        writecontrolregister(MACON3, MACON3_PADCFG0 | MACON3_TXCRCEN | MACON3_FRMLNEN | MACON3_FULDPX);
        writecontrolregister(MABBIPG, 0x15);
        writecontrolregister(MAIPGL, 0x12);
        writephyregister(PHCON1, PHCON1_PDPXMD);
        halfduplex = FALSE;
    }
    else {
        /* Half duplex mode */
        writecontrolregister(MACON1, MACON1_MARXEN);
        writecontrolregister(MACON3, MACON3_PADCFG0 | MACON3_TXCRCEN | MACON3_FRMLNEN);
        writecontrolregister(MABBIPG, 0x12);
        writecontrolregister(MAIPGH, 0x0C);
        writephyregister(PHCON1, 0);
        halfduplex = TRUE;
    }

    enc28j60_int_resume();
}

bool enc28j60_link(void)
/*!
  Get current linkstate
*/
{
    return linkstate;
}

void enc28j60_put_transmit(const unsigned short location, const unsigned short length)
/*!
  Transmit packet of 'length' bytes from controller RAM, stored at 'location'
*/
{
    /* Set address of packet end */
    writecontrolregister(ETXNDL, (location + length) & 0xFF);
    writecontrolregister(ETXNDH, ((location + length)>>8) & 0xFF);

    /* Statusvector will be written here when transmission is done */
    txstatusvectorlocation = location + length + 1;

    /* Reset the internal transmit logic before attempting to transmit a packet
       (see Erreta rev. B7, note 12)
       I read that erreta-note as 'only reset if EIR_TXERIF is set', but it
       appears that's not the case (Tx-parts keeps locking up after about a
       day of continuous ping if I don't do this reset unconditionally) */
    setcontrolbit(ECON1, BANKDONTCARE, ECON1_TXRST);
    clearcontrolbit(ECON1, BANKDONTCARE, ECON1_TXRST);
    /* Reset may cause a new transmit error interrupt event, thus clear interrupt flag */
    clearcontrolbit(EIR, BANKDONTCARE, EIR_TXERIF);

    /* Start transmission */
    #ifdef ENC28J60_DEBUG
    dprint("enc28j60_put_transmit(): start, %d bytes from 0x%x\n\r", length, location);
    #endif
    transmitting = TRUE;
    setcontrolbit(ECON1, BANKDONTCARE, ECON1_TXRTS);
}

void enc28j60_put_copydata(unsigned char *data, const unsigned short length)
/*!
  Copy data to the controller, at location set with enc28j60_put_startofpacket() or
  optionally with enc28j60_put_setwritepointer()
  The writepointer is updated automatically, so one can call enc28j60_put_copydata()
  multiple times, each time with a part of the packet
*/
{
    unsigned short i=0;

    enc28j60_cs_assert();
    encspi_put(CMD_WBM);
    while(i < length) {
        encspi_put(*data);
        data++;
        i++;
    }
    enc28j60_cs_deassert();
}

void enc28j60_put_startofpacket(const unsigned short location) 
/*!
  Set start-of-packet
*/
{
    bankselect(BANK0);
    /* Set transmission start pointer.. */
    writecontrolregister(ETXSTL, location & 0xFF);
    writecontrolregister(ETXSTH, (location>>8) & 0xFF);
    /* ..and write pointer */
    writecontrolregister(EWRPTL, location & 0xFF);
    writecontrolregister(EWRPTH, (location>>8) & 0xFF);

    /* Write per packet control byte; we use the settings as defined in MACON3 */
    enc28j60_cs_assert();
    encspi_put(CMD_WBM);
    encspi_put(0);
    enc28j60_cs_deassert();

    #ifdef ENC28J60_DEBUG
    dprint("enc28j60_put_startofpacket(): 0x%x\n\r", location);
    #endif
}

void enc28j60_put_setwritepointer(const unsigned short location)
/*!
  Set writepointer, as used by enc28j60_put_copydata()
*/
{
    bankselect(BANK0);
    /* Set transmission write pointer */
    writecontrolregister(EWRPTL, location & 0xFF);
    writecontrolregister(EWRPTH, (location>>8) & 0xFF);

    #ifdef ENC28J60_DEBUG
    dprint("enc28j60_put_setwritepointer(): 0x%x\n\r", location);
    #endif
}

void enc28j60_put_wait(void)
/*!
  Wait for a previous transmission to complete
*/
{
    unsigned short i=0;

    if(transmitting) {
        #ifdef ENC28J60_DEBUG
        dprint("enc28j60_put_wait(): waiting for previous transmission to complete\n\r");
        #endif
        while(transmitting) {
            delay_us(1);
            i++;
            if(i == 0) {
                #ifdef ENC28J60_DEBUG
                dprint("enc28j60_put_wait(): previous transmission timed out\n\r");
                #endif
                break;
            }
        }
    }
}

unsigned char enc28j60_pendingpackets(void)
/*!
  Amount of pending packets in the Rx buffer
*/
{
    #if ENC28J60_RX_POLL
    if(pendingpackets == 0) {
        enc28j60_int_suspend();
        bankselect(BANK1);
        pendingpackets = readcontrolregister(EPKTCNT);
        enc28j60_int_resume();
        #ifdef ENC28J60_DEBUG
        if(pendingpackets) {
            dprint("enc28j60_pendingpackets(): %d pending packets\n\r", pendingpackets);
        }
        #endif
    }
    #endif
    return pendingpackets;
}

unsigned short enc28j60_get(unsigned char *packetbuffer)
/*!
  Read one packet from the Rx buffer
*/
{
    static unsigned short packetpointer = RXSTART;
    unsigned char packet_ok;

    if(pendingpackets == 0) {
        #ifdef ENC28J60_DEBUG
        dprint("enc28j60_get(): no packets to read\n\r");
        #endif
        return 0;
    }

    enc28j60_int_suspend();

    /* Set read pointer */
    bankselect(BANK0);
    writecontrolregister(ERDPTL, packetpointer & 0xFF);
    writecontrolregister(ERDPTH, (packetpointer>>8) & 0xFF);

    #ifdef ENC28J60_DEBUG
    dprint("enc28j60_get(): read from 0x%x\n\r",packetpointer);
    #endif

    /* Read next packet pointer and status vector */
    readbuffermemory(rxstatusvector,6);

    /* And store readpointer for the next packet */
    packetpointer = ( ((receptionvector_t *)&rxstatusvector[0]) )->nextpacketpointer;

    #ifdef ENC28J60_DEBUG
    dprint("enc28j60_get(): nextpacketpointer: 0x%x, bytecount: %d, rxstatus: 0x%x\n\r", ((receptionvector_t *)rxstatusvector)->nextpacketpointer,
                                                                                         ((receptionvector_t *)rxstatusvector)->bytecount,
                                                                                         ((receptionvector_t *)rxstatusvector)->rxstatus);
    #endif

    if(((receptionvector_t *)rxstatusvector)->rxstatus & RXSTATUS_OK) {
        /* Packet OK, read contents */
        readbuffermemory(packetbuffer, ((receptionvector_t *)rxstatusvector)->bytecount);
        packet_ok = TRUE;
    }
    else {
        #ifdef ENC28J60_DEBUG
        dprint("enc28j60_get(): RXSTATUS != OK\n\r");
        #endif
        packet_ok = FALSE;
    }

    /* Free memory by setting Rx packet pointer,
       making sure RXRDPT is an odd value (see Erreta rev. B7, note 14) */
    if(packetpointer == RXSTART) {
        writecontrolregister(ERXRDPTL, RXEND & 0xFF);
        writecontrolregister(ERXRDPTH, (RXEND>>8) & 0xFF);
    }
    else {
        writecontrolregister(ERXRDPTL, (packetpointer-1) & 0xFF);
        writecontrolregister(ERXRDPTH, ((packetpointer-1)>>8) & 0xFF);
    }

    /* Decrement packet counter */
    setcontrolbit(ECON2, BANKDONTCARE, ECON2_PKTDEC);
    pendingpackets--;
    
    /* If the recieve-buffer was full (set from the interrupt handler, EIR_RXERIF), it's not anymore */
    if(bufferfull) {
        /* Clear flag, re-enable interrupt */
        clearcontrolbit(EIR, BANKDONTCARE, EIR_RXERIF);
        setcontrolbit(EIE, BANKDONTCARE, EIE_RXERIE);
        bufferfull = FALSE;
    }

    #if ENC28J60_RX_POLL == 0
    /* Re-enable rx-interrupt (disabled from the interrupt handler) */
    setcontrolbit(EIE, BANKDONTCARE, EIE_PKTIE);
    #endif

    enc28j60_int_resume();

    if(packet_ok) {
        return ( ((receptionvector_t *)&rxstatusvector[0]) )->bytecount;
    }
    return 0;
}

static void bankselect(const unsigned char bank)
/*!
  Switch to given register-bank
*/
{
    /* Switch requested, and if so, needed? */
    if(bank != BANKDONTCARE && bank != currentbank) {
        /* Switch bank, keep other bits as they where while doing so */
        #ifdef ENC28J60_DEBUG_TRANSFERS
        dprint("bankselect(): switch from bank %i to bank %i\n\r",currentbank, (bank&0x03));
        #endif

        enc28j60_cs_assert();
        encspi_put(CMD_BFC | (ECON1 & REGISTERMASK));
        encspi_put(ECON1_BSEL0 | ECON1_BSEL1);
        enc28j60_cs_deassert();

        enc28j60_cs_assert();
        encspi_put(CMD_BFS | (ECON1 & REGISTERMASK));
        encspi_put(bank&0x03);
        enc28j60_cs_deassert();

        currentbank = (bank&0x03);
    }
}

static void setcontrolbit(const unsigned char address, const unsigned char bank, const unsigned char bitmask)
/*!
  Set bits in the given ETH register (indeed, this only works on ETH registers!)
*/
{
    bankselect(bank);

    #ifdef ENC28J60_DEBUG
    dprint("setcontrolbit(); set at address 0x%x: 0x%x\n\r",address,bitmask);
    #endif

    enc28j60_cs_assert();
    encspi_put(CMD_BFS | (address & REGISTERMASK));
    encspi_put(bitmask);
    enc28j60_cs_deassert();
}

static void clearcontrolbit(const unsigned char address, const unsigned char bank, const unsigned char bitmask)
/*!
  Clear bits in the given ETH register (indeed, this only works on ETH registers!)
*/
{
    bankselect(bank);

    #ifdef ENC28J60_DEBUG
    dprint("clearcontrolbit(); clear at address 0x%x: 0x%x\n\r",address,bitmask);
    #endif

    enc28j60_cs_assert();
    encspi_put(CMD_BFC | (address & REGISTERMASK));
    encspi_put(bitmask);
    enc28j60_cs_deassert();
}
                                
static unsigned char readcontrolregister(const unsigned char address)
/*!
  Read a single ETH, MAC or MII register
*/
{
    unsigned char value;

    /* Execute the command */
    enc28j60_cs_assert();
    encspi_put(CMD_RCR | (address & REGISTERMASK));
    if((currentbank == BANK2 && address < 0x1B) || (currentbank == BANK3 && address < 0x06)) {
        /* MAC and MII registers require a dummy byte before the read */
        encspi_put(0xFF);
    }
    value = encspi_get();
    enc28j60_cs_deassert();

    #ifdef ENC28J60_DEBUG_TRANSFERS
    dprint("readcontrolregister(); read from address 0x%x: 0x%x\n\r",address,value);
    #endif

    /* And return the value just read */
    return value;
}

static void writecontrolregister(const unsigned char address, const unsigned char value)
/*!
  Write a single ETH, MAC or MII register
*/
{
    #ifdef ENC28J60_DEBUG_TRANSFERS
    dprint("writecontrolregister(); write to address 0x%x: 0x%x\n\r",address,value);
    #endif

    /* Execute the command */
    enc28j60_cs_assert();
    encspi_put(CMD_WCR | (address & REGISTERMASK));
    encspi_put(value);
    enc28j60_cs_deassert();
}

static unsigned short readphyregister(const unsigned char address)
/*!
  Read a single PHY register
*/
{
    unsigned char timeout;
    unsigned short value;

    bankselect(BANK2);
    writecontrolregister(MIREGADR, address);
    writecontrolregister(MICMD, MICMD_MIIRD);
    timeout=0;
    bankselect(BANK3);
    while(readcontrolregister(MISTAT) & MISTAT_BUSY) {
        delay_ms(1);
        timeout++;
        if(timeout == 255) {
            #ifdef ENC28J60_DEBUG
            dprint("readphyregister(); timeout reading from address 0x%x\n\r",address);
            #endif
            return 0;
        }
    }
    bankselect(BANK2);
    writecontrolregister(MICMD, 0);

    value = (readcontrolregister(MIRDH)<<8) | readcontrolregister(MIRDL);

    #ifdef ENC28J60_DEBUG_TRANSFERS
    dprint("readphyregister(); read from address 0x%x: 0x%x\n\r",address,value);
    #endif

    return value;
}

static void writephyregister(const unsigned char address, const unsigned short value)
/*!
  Write a single PHY register
*/
{
    #ifdef ENC28J60_DEBUG_TRANSFERS
    dprint("writephyregister(); write to address 0x%x: 0x%x\n\r",address,value);
    #endif

    bankselect(BANK2);
    writecontrolregister(MIREGADR, address);
    writecontrolregister(MIWRL, value & 0xFF);
    writecontrolregister(MIWRH, (value>>8) & 0xFF);
}

static void readbuffermemory(unsigned char *data, unsigned short length)
/*!
  Read data from the reception buffer
*/
{
    #ifdef ENC28J60_DEBUG
    dprint("readbuffermemory(): read %i bytes, writing to 0x%x\n\r",length,data);
    #endif

    enc28j60_cs_assert();
    encspi_put(CMD_RBM);
    while(length) {
        *data = encspi_get();
        data++;
        length--;
    }
    enc28j60_cs_deassert();
}

void enc28j60_int(void)
/*!
  Interrupt handler, to be called after a falling edge on the INT line
*/
{
    unsigned char flags;

    /* Clear global interrupt-enable bit, this'll cause the INT-pin to de-assert.
       We re-enable this before leaving the interrupt-handler */
    clearcontrolbit(EIE, BANKDONTCARE, EIE_INTIE);

    do {
        /* Read interrupt flag register */
        flags = readcontrolregister(EIR);

        #ifdef ENC28J60_DEBUG
        dprint("enc28j60_int(): 0x%x\n\r",flags);
        #endif

        /* And figure out what's happening */
        if(flags & EIR_DMAIF) {
            /* DMA copy or checksum calculation has completed */
            clearcontrolbit(EIR, BANKDONTCARE, EIR_DMAIF);
            #ifdef ENC28J60_DEBUG
            dprint("DMA ready\n\r");
            #endif
        }
        if(flags & EIR_LINKIF) {
            /* PHY reports that the link status has changed; read PHIR register to clear */
            readphyregister(PHIR);
            if(readphyregister(PHSTAT2) & PHSTAT2_LSTAT) {
                if(linkstate != TRUE) {
                    linkstate = TRUE;
                    network_linkchange();
                }
            }
            else {
                if(linkstate != FALSE) {
                    linkstate = FALSE;
                    network_linkchange();
                }
            }
        }
        if(flags & EIR_TXIF) {
            /* Transmit request has ended; clear flag */
            clearcontrolbit(EIR, BANKDONTCARE, EIR_TXIF);
            /* And read the statusvector. First set read pointer */
            bankselect(BANK0);
            writecontrolregister(ERDPTL, txstatusvectorlocation & 0xFF);
            writecontrolregister(ERDPTH, (txstatusvectorlocation>>8) & 0xFF);
            /* Now read data */
            readbuffermemory(txstatusvector, 7);
            #ifdef ENC28J60_DEBUG
            dprint("enc28j60_int(): Transmission complete, bytes in packet: %i, bytes on wire: %i\n\r",((transmissionvector_t *)txstatusvector)->bytecount, 
                                                                                                       ((transmissionvector_t *)txstatusvector)->bytecountraw);
            dprint("                txstatus1: 0x%x, txstatus2: 0x%x\n\r", ((transmissionvector_t *)txstatusvector)->txstatus1,
                                                                           ((transmissionvector_t *)txstatusvector)->txstatus2);
            #endif
            transmitting = FALSE;
        }
        // WOLIF is not enabled
        if(flags & EIR_TXERIF) {
            /* A transmit error has occurred */
            #ifdef ENC28J60_DEBUG
            dprint("enc28j60_int(): Transmission error, ESTAT = 0x%x\n\r", readcontrolregister(ESTAT));
            #endif
            /* Clear LATECOL and TXABRT flags */
            clearcontrolbit(ESTAT, BANKDONTCARE, ESTAT_LATECOL | ESTAT_TXABRT);
            /* Reset transmit logic */
            setcontrolbit(ECON1, BANKDONTCARE, ECON1_TXRST);
            clearcontrolbit(ECON1, BANKDONTCARE, ECON1_TXRST);
            /* Clear interrupt flag */
            clearcontrolbit(EIR, BANKDONTCARE, EIR_TXERIF);
        }
        if(flags & EIR_RXERIF) {
            /* A packet was aborted because there is insufficient buffer space or the packet count is 255 */
            if(bufferfull == FALSE) {
                bufferfull = TRUE;
                /* Disable interrupt until userspace has had time to read some packets */
                clearcontrolbit(EIE, BANKDONTCARE, EIE_RXERIE);
                #ifdef ENC28J60_DEBUG
                dprint("enc28j60_int(): buffer overflow\n\r");
                #endif
            }
        }
        #if ENC28J60_RX_POLL == 0
        /* EIR_PKTIF 'does not reliably/accurately report the status of pending packets' (see Erreta rev. B7, note 6).
           Fortunately, the INT-pin will reliably be asserted when a packet arrives. Workaround is to use EPKTCNT */
        bankselect(BANK1);
        if((pendingpackets = readcontrolregister(EPKTCNT))) {
            /* Disable interrupt until userspace has had time to read some packets */
            clearcontrolbit(EIE, BANKDONTCARE, EIE_PKTIE);
            #ifdef ENC28J60_DEBUG
            dprint("enc28j60_int(): %i pending packets in rx buffer\n\r", pendingpackets);
            #endif
        }
        #endif
    } while (readcontrolregister(ESTAT) & (1<<7));

    /* Set global interrupt-enable bit. If another interrupt-event is pending,
       this'll result in a new falling edge on the INT pin. */
    setcontrolbit(EIE, BANKDONTCARE, EIE_INTIE);
}
