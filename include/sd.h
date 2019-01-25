/*
    Piconet RS232 ethernet interface

    sd.h
           
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
#ifndef SD_H
#define SD_H

#include <config.h>

/* Start and stop data tokens for single and multiple block SD-card
   data operations */
#define     START_SBR      0xFE
#define     START_MBR      0xFE
#define     START_SBW      0xFE
#define     START_MBW      0xFC
#define     STOP_MBW       0xFD

#define     DATA_RESP_MASK 0x11   // Mask for data response token after an SD-card write.
#define     BUSY_BIT       0x80   // Mask for busy token in R1b response.

/* SD card SPI mode commands.
   Each command is followed by a response from the card, which can be any of the following:
    R1      8 bit response
    R1b     8 bit response whit busy indication (card will respond whit 0x00 until ready,
            when it will respond whit something > 0)
    R2      16 bit response
    R3      40 bit response (5 bytes)
   Bit descriptions for all responses are found in the MMC product manual, page 83 and further.

   The following statusregisters are accessable in SPI mode(MMC product manual, page 40):
    CID Card IDentification     16 bytes, contains identification info  MMC product manual, page 31
    CSD Card Specific Data      16 bytes, contains operating info   MMC product manual, page 31 */
#define CMD0 0x40   // software reset, response R1
#define CMD1 0x41   // brings card out of idle state, response R1
#define CMD2 0x42   // not used in SPI mode
#define CMD3 0x43   // not used in SPI mode
#define CMD4 0x44   // not used in SPI mode
#define CMD5 0x45   // Reserved
#define CMD6 0x46   // Reserved
#define CMD7 0x47   // not used in SPI mode
#define CMD8 0x48   // Reserved
#define CMD9 0x49   // ask card to send card speficic data (CSD), response R1
#define CMD10 0x4A  // ask card to send card identification (CID), response R1
#define CMD11 0x4B  // not used in SPI mode
#define CMD12 0x4C  // stop transmission on multiple block read, response R1
#define CMD13 0x4D  // ask the card to send it's status register, response R2
#define CMD14 0x4E  // Reserved
#define CMD15 0x4F  // not used in SPI mode
#define CMD16 0x50  // sets the block length used by the memory card, response R1
#define CMD17 0x51  // read single block, response R1
#define CMD18 0x52  // read multiple block, response R1
#define CMD19 0x53  // Reserved
#define CMD20 0x54  // not used in SPI mode
#define CMD21 0x55  // Reserved
#define CMD22 0x56  // Reserved
#define CMD23 0x57  // Reserved
#define CMD24 0x58  // writes a single block, response R1
#define CMD25 0x59  // writes multiple blocks, response R1
#define CMD26 0x5A  // not used in SPI mode
#define CMD27 0x5B  // change the bits in CSD, response R1
#define CMD28 0x5C  // sets the write protection bit, response R1b
#define CMD29 0x5D  // clears the write protection bit, response R1b
#define CMD30 0x5E  // checks the write protection bit, response R1
#define CMD31 0x5F  // Reserved
#define CMD32 0x60  // Sets the address of the first sector of the erase group, response R1
#define CMD33 0x61  // Sets the address of the last sector of the erase group, response R1
#define CMD34 0x62  // removes a sector from the selected group, response R1
#define CMD35 0x63  // Sets the address of the first group, response R1
#define CMD36 0x64  // Sets the address of the last erase group, response R1
#define CMD37 0x65  // removes a group from the selected section, response R1
#define CMD38 0x66  // erase all selected groups, response R1b
#define CMD39 0x67  // not used in SPI mode
#define CMD40 0x68  // not used in SPI mode
#define CMD41 0x69  // Reserved
#define CMD42 0x6A  // locks a block, response R1b
// CMD43 ... CMD57 are Reserved
#define CMD55 0x77  // Application specific command notify (next CMD is ACMD)
#define CMD58 0x7A  // reads the OCR register, response R3
#define CMD59 0x7B  // turns CRC off, response R1
// CMD60 ... CMD63 are not used in SPI mode
#define	ACMD41 0xE9 // SEND_OP_COND

#define CARDTYPE_UNKNOWN                0
#define CARDTYPE_SDV1                   1
#define CARDTYPE_SDV2                   2
#define CARDTYPE_MMCV3                  3
#define CARDTYPE_SDV2_BLOCKADDRESSING   4

typedef struct {
    unsigned char type;     // One of the CARDTYPE_ defines
    unsigned char CSD[16];  // The CSD register
    unsigned long size;     // Drivesize in MB, derived from the CSD register contents
} cardinfo_t;

bool sd_init(void);
bool sd_readsectors(unsigned long startsector, unsigned char sectorcount, unsigned char* buffer);
bool sd_writesectors(unsigned long startsector, unsigned char sectorcount, const unsigned char* buffer);

#endif /* SD_H */
