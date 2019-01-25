/*
    Piconet RS232 ethernet interface

    sd.c
           
    Copyright (c) 2007,2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*
 \file
SD-card SPI driver
*/
#include "sd.h"
#include <ssp.h>
#include <io.h>
#include <debug.h>
#include <delay.h>
#include <std.h>

/* To debug or not to debug.. */
//#define SD_DEBUG

/* Which ssp interface to use? */
#define sdspi_put(data)     ssp1_put(data)
#define sdspi_get()         ssp1_get()
#define sdspi_slow()        ssp2_clock_64()
#define sdspi_fast()        ssp2_clock_4()

cardinfo_t cardinfo;

/* Used to pass errorconditions from the low-level functions to the others */
bool sd_err;

static bool sd_getregister(unsigned char command, unsigned char *buffer);
static unsigned char sd_put(unsigned char command, unsigned long argument, unsigned char CRC);
static bool sd_waituntil(unsigned char mask, unsigned char response);

bool sd_init()
/*
  Initialise SD card
*/
{
    unsigned int response;
    unsigned short i;
    
    unsigned char C_SIZE_MULT;  //3 bit  (not used for CSD 2.0)
    unsigned long C_SIZE;       //12 bit (22 bit for CSD 2.0)
    unsigned char READ_BL_LEN;  //4 bit

    cardinfo.type = CARDTYPE_UNKNOWN;

    sdspi_slow();

    /* Card sync */
    sd_cs_deassert();
    for(i=0;i<20;i++) {
        sdspi_put(0xFF);
    }

    /* Send software reset command, puts card in SPI mode */
    i=100;
    while((response=sd_put(CMD0,0,0x95)) != 1 && i--) {
        delay_ms(1);
    }
    if(response != 1) {
        #ifdef SD_DEBUG
        dprint("SD: soft-reset failed, response: 0x%x\n\r", response, i);
        #endif
        return FALSE;
    }

    /* Figure out what type of card we're dealing with */
    if(sd_put(CMD8,0x1AA,0x87) == 1) {
        /* SDv2; read additional 32 bit response. 
           We don't need the upper 16 bits, so discard these right away */
        sdspi_get();
        sdspi_get();
        response = sdspi_get()<<8;
        response |= sdspi_get();
                
        if(((response>>8)&0xFF) == 0x01 && (response&0xFF) == 0xAA) {
            i=100;
            /* Wait for end of initialisation */
            while(sd_put(CMD55, 0, 0xFF) && sd_put(ACMD41&0x7F, 0x40000000, 0xFF) && i--) {
                delay_ms(1);
            }
            if(i == 0) {
                #ifdef SD_DEBUG
                dprint("SD: end-of-initialization timeout\n\r");
                #endif
                return FALSE;
            }
            /* Read OCR register */
            if(sd_put(CMD58,0,0xFF) == 0) {
                /* 32 bit response, we only need bit 30 */
                response = sdspi_get();
                sdspi_get();
                sdspi_get();
                sdspi_get();
                
                if(response & (1<<(30-24))) {
                    cardinfo.type = CARDTYPE_SDV2_BLOCKADDRESSING;
                }
                else {
                    cardinfo.type = CARDTYPE_SDV2;
                }
            }
            else {
                #ifdef SD_DEBUG
                dprint("SD: failed to read OCR register\n\r");
                #endif
                return FALSE;
            }
        }
        else {
            #ifdef SD_DEBUG
            dprint("SD: card does not support our voltage-range\n\r");
            #endif
            return FALSE;
        }
    }
    else {
        /* SDv1 or MMCv3 */
        sd_put(CMD55, 0, 0xFF);
        if (sd_put(ACMD41&0x7F, 0, 0xFF) <= 1)  {
            /* SDv1 */
            cardinfo.type = CARDTYPE_SDV1;
            i=100;
            /* Wait until card leaves idle-state */
            while(sd_put(CMD55, 0, 0xFF) && sd_put(ACMD41&0x7F, 0x40000000, 0xFF) && i--) {
                delay_ms(1);
            }
        } 
        else {
            /* MCCv3 */
            cardinfo.type = CARDTYPE_MMCV3;
            i=100;
            /* Wait untill card leaves idle-state */
            while(sd_put(CMD0,0,0xFF) && i--) {
                delay_ms(1);
            }
        }
        if(i==0) {
            #ifdef SD_DEBUG
            dprint("SD: fails to leave idle state\n\r");
            #endif
            return FALSE;
        }
        
        /* Set sectorlength */
        if(sd_put(CMD16, 512, 0xFF) != 0 || sd_err) {
            #ifdef SD_DEBUG
            dprint("SD: failed to set sectorlength\n\r");
            #endif
            return FALSE;
        }
    }
    
    sdspi_fast();
    
    /* Get drivesize; first we read the CSD */
    if(sd_getregister(CMD9, cardinfo.CSD) == FALSE) {
        #ifdef SD_DEBUG
        dprint("SD: could not read CSD register\n\r");
        #endif
        return FALSE;
    }
    
    if(cardinfo.CSD[0] == 0) {
        /* CSD version 1.0 */
        
        /* Card capacity is calculated as follows:
            capacity (bytes) = BLOCKNR * BLOCKLEN
           Where
            BLOCKNR = (C_SIZE+1)*(2^(C_SIZE_MULT+2))
            BLOCKLEN = 2^(READ_BL_LEN)
         */
            
        /* C_SIZE_MULT */
        C_SIZE_MULT = ((cardinfo.CSD[9]&0x03)<<1) | ((cardinfo.CSD[10])>>7);
        /* C_SIZE */
        C_SIZE = ((cardinfo.CSD[6]&0x03)<<10) | (cardinfo.CSD[7]<<2) | ((cardinfo.CSD[8]&0xC0));
        /* READ_BL_LEN */
        READ_BL_LEN = (cardinfo.CSD[5]&0x0F);
        
        #ifdef SD_DEBUG
        dprint("SD: CSD register version 0x%x\n\r", cardinfo.CSD[0]);
        dprint("    C_SIZE_MULT = %d\n\r",C_SIZE_MULT);
        dprint("    C_SIZE = %ld\n\r",C_SIZE);
        dprint("    READ_BL_LEN = %d\n\r",READ_BL_LEN);
        #endif
        
        /* Now do the math */
        cardinfo.size = ((C_SIZE+1) * fpow(2, C_SIZE_MULT+2) * fpow(2,READ_BL_LEN)) / 1024 / 1024;
    }
    else if(cardinfo.CSD[0] == 0x40) {
        /* CSD version 2.0 */
        
        /* Card capacity is calculated as follows:
            capacity (kbytes) = BLOCKNR * BLOCKLEN
           Where
            BLOCKNR = (C_SIZE+1)
            BLOCKLEN = 2^(READ_BL_LEN)
         */
         
        /* C_SIZE */
        C_SIZE = ((unsigned long)(cardinfo.CSD[7]&0x3F)<<16) | (cardinfo.CSD[8]<<8) | (cardinfo.CSD[9]);
        /* READ_BL_LEN */
        READ_BL_LEN = (cardinfo.CSD[5]&0x0F);
        
        #ifdef SD_DEBUG
        dprint("SD: CSD register version 0x%x\n\r", cardinfo.CSD[0]);
        dprint("    C_SIZE = %ld\n\r",C_SIZE);
        dprint("    READ_BL_LEN = %d\n\r",READ_BL_LEN);
        #endif

        /* Now do the math */
        cardinfo.size = ((C_SIZE+1) * fpow(2,READ_BL_LEN)) / 1024;
    }
    else {
        #ifdef SD_DEBUG
        dprint("SD: CSD register format undefined\n\r");
        #endif
        return FALSE;
    }

    return TRUE;
}

static unsigned char sd_put(unsigned char command, unsigned long argument, unsigned char CRC)
/*
  Send a single command to the SD card
*/
{
    unsigned char response;
    unsigned char i;

    /* Select card, wait untill it's ready */
    sd_cs_deassert();
    sd_cs_assert();
    i=0;
    while(sdspi_get() != 0xFF) {
        i++;
        if(i == 0) {
            #ifdef SD_DEBUG
            dprint("SD: command wait-ready timout; command CMD%i, argument %i\n\r", command&(~0x40), argument);
            #endif
            sd_err = TRUE;
            return 0;
        }
    }

    /* send command */
    sdspi_put(command);

    /* send data */
    sdspi_put(argument >> 24);
    sdspi_put(argument >> 16);
    sdspi_put(argument >> 8);
    sdspi_put(argument);

    /* send CRC */
    sdspi_put(CRC);

    /* Wait for it (max 10 clock units) */
    i=0;
    do {
        response = sdspi_get();
        i++;
        if(i>10) {
            /* Deselect card */
            sd_cs_deassert();
            #ifdef SD_DEBUG
            dprint("SD: command timout [0x%x]; command CMD%i, argument %i\n\r", response, command&(~0x40), argument);
            #endif
            sd_err = TRUE;
            return 0;
        }
    } while(response == 0xFF);

    sd_err = FALSE;
    return response;
}

static bool sd_getregister(unsigned char command, unsigned char *buffer)
/*
  Read a 16 bytes register (CID or CSD) from the SD card
*/
{
    char i;

    /* Send initial command */
    if(sd_put(command,0,0xFF)!=0) {
        return FALSE;
    }

    if(sd_waituntil(0xFF,START_SBR) == FALSE) {
        return FALSE;
    }

    /* Read the the register */
    for(i=0;i<16;i++) {
        *buffer = sdspi_get();
        buffer++;
    }

    sdspi_put(0xFF);

    /* deselect card */
    sd_cs_deassert();

    /* Send 8 wait clockcycles */
    sdspi_put(0xFF);

    return TRUE;
}

static bool sd_waituntil(unsigned char mask, unsigned char response)
/*
  Wait for a response from the SD card
*/
{
    unsigned short timeout=0;

    /* select card */
    sd_cs_assert();

    while((sdspi_get() & mask) != response) {
        timeout++;
        if(timeout==0) {
            /* deselect card */
            sd_cs_deassert();
            return FALSE;
        }
    }

    return TRUE;
}

bool sd_readsectors(unsigned long startsector, unsigned char sectorcount, unsigned char* buffer)
/*
  Read one or more sectors from the SD card, and store it in 'buffer'
*/
{
    unsigned short i, j;
    unsigned long sector;
    
    if(cardinfo.type == CARDTYPE_SDV2_BLOCKADDRESSING) {
        sector = startsector;
    }
    else {
        sector = startsector * 512;
    }

    if(sectorcount == 1) {
        /* Set startaddress for a single-block read */
        if(sd_put(CMD17,sector,0xFF) != 0 || sd_err) {
            return FALSE;
        }
    }
    else {
        /* Set startaddress for a multiple-block read */
        if(sd_put(CMD18,sector,0xFF) != 0 || sd_err) {
            return FALSE;
        }
    }

    /* Wait until the card has found the data we want */
    if(sd_waituntil(0xFF, START_SBR) == FALSE) {
        #ifdef SD_DEBUG
        dprint("SD: read timeout, sector %i\n\r", startsector);
        #endif
        return FALSE;
    }

    /* And read data */
    for(j=0;j<sectorcount;j++) {
        for(i=0;i<512;i++) {
            buffer[i] = sdspi_get();
        }
        /* Close transfer by reading the two byte CRC (and futher ignoring it's value) */
        sdspi_put(0xFF);
        sdspi_put(0xFF);
    }
    
    if(sectorcount > 1) {
        /* Stop multiple-block read transmission */
        if(sd_put(CMD12,0,0xFF) != 0 || sd_err) {
            return FALSE;
        }
    }

    /* deselect card */
    sd_cs_deassert();

    /* Send 8 wait clockcycles */
    sdspi_put(0xFF);

    return TRUE;
}

bool sd_writesectors(unsigned long startsector, unsigned char sectorcount, const unsigned char* buffer)
/*
  Write one or more sectors to the SD card, read data from 'buffer'
*/
{
    unsigned short i, j;    
    unsigned long sector;
    
    if(cardinfo.type == CARDTYPE_SDV2_BLOCKADDRESSING) {
        sector = startsector;
    }
    else {
        sector = startsector * 512;
    }

    /* Write sector to disk
       First set startaddress of writeaction */
    if(sectorcount == 1) {
        /* Set startaddress for a single-block write */
        if(sd_put(CMD24,sector,0xFF) != 0 || sd_err) {
            return FALSE;
        }
    
        /* select card */
        sd_cs_assert();
        /* Start block command */
        sdspi_put(START_SBW);
    }
    else {
        /* Set startaddress for a multiple-block write */
        if(sd_put(CMD25,sector,0xFF) != 0 || sd_err) {
            return FALSE;
        }
    
        /* select card */
        sd_cs_assert();
        /* Start block command */
        sdspi_put(START_MBW);
    }

    /* write data */
    for(j=0;j<sectorcount;j++) {
        for(i=0;i<512;i++) {
            sdspi_put(buffer[i]);
        }
        /* Close transfer by sending dummy CRC */
        sdspi_put(0xFF);
        sdspi_put(0xFF);
        
        /* Wait for data response token */
        if(sd_waituntil(DATA_RESP_MASK, 1) == FALSE) {
            #ifdef SD_DEBUG
            dprint("SD: write token timeout, sector %i\n\r", startsector);
            #endif
            return FALSE;
        }
        /* OK, now wait until card is done writing */
        if(sd_waituntil(0xFF,0xFF) == FALSE) {
            #ifdef SD_DEBUG
            dprint("SD: write busy timeout, sector %i\n\r", startsector);
            #endif
            return FALSE;
        }
    }
    
    if(sectorcount > 1) {
        /* Stop multiple-block write transmission */
        sdspi_put(STOP_MBW);
    }

    /* deselect card */
    sd_cs_deassert();

    /* Send 8 wait clockcycles */
    sdspi_put(0xFF);

    return FALSE;
}
