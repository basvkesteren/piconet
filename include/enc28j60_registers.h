/*
    Piconet RS232 ethernet interface

    enc28j60_registers.h
           
    Copyright (c) 2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
\file
ENC28J60 driver, register definitions
*/

/*
    From the ENC28J60 datasheet, page 12:

    "The Control Register memory is partitioned into four banks, selectable by
     the bank select bits BSEL1:BSEL0 in the ECON1 register. Each bank is 32
     bytes long and addressed by a 5-bit address value.
     The last five locations (1Bh to 1Fh) of all banks point to a common set of
     registers: EIE, EIR, ESTAT, ECON2 and ECON1. These are key registers used in
     controlling and monitoring the operation of the device. Their common mapping
     allows easy access without switching the bank."

    "Some of the available addresses are unimplemented. Any attempts to write to
     these locations are ignored while reads return ‘0’s. The register at
     address 1Ah in each bank is reserved; read and write operations should not
     be performed on this register. All other reserved registers may be read,
     but their contents must not be changed. When reading and writing to
     registers which contain reserved bits, any rules stated in the register
     definition should be observed."

    "Control registers for the ENC28J60 are generically grouped as ETH, MAC and
     MII registers. Register names starting with “E” belong to the ETH group.
     Similarly, registers names starting with “MA” belong to the MAC group and
     registers prefixed with “MI” belong to the MII group."

*/
#ifndef ENC28J60_REGISTERS_H
#define ENC28J60_REGISTERS_H

/* Register addressing is from 0x00 to 0x1F; following is a mask to chop all
   invalid addresses to something that will fit in */
#define REGISTERMASK    0x1F

/* Register banks */
#define BANK0           0
#define BANK1           1
#define BANK2           2
#define BANK3           3
#define BANKDONTCARE    0xFF

/* Bank 0 registers */
#define ERDPTL          0x00
#define ERDPTH          0x01
#define EWRPTL          0x02
#define EWRPTH          0x03
#define ETXSTL          0x04
#define ETXSTH          0x05
#define ETXNDL          0x06
#define ETXNDH          0x07
#define ERXSTL          0x08
#define ERXSTH          0x09
#define ERXNDL          0x0A
#define ERXNDH          0x0B
#define ERXRDPTL        0x0C
#define ERXRDPTH        0x0D
#define ERXWRPTL        0x0E
#define ERXWRPTH        0x0F
#define EDMASTL         0x10
#define EDMASTH         0x11
#define EDMANDL         0x12
#define EDMANDH         0x13
#define EDMADSTL        0x14
#define EDMADSTH        0x15
#define EDMACSL         0x16
#define EDMACSH         0x17
/* unused               0x18 */
/* unused               0x19 */
/* reserved             0x1A */
#define EIE             0x1B
#define EIR             0x1C
#define ESTAT           0x1D
#define ECON2           0x1E
#define ECON1           0x1F

/* EIE bit definitions */
#define EIE_INTIE       (1<<7)  /*  1 = Allow interrupt events to drive the INT pin
                                    0 = Disable all INT pin activity (pin is continuously driven high) */
#define EIE_PKTIE       (1<<6)  /*  1 = Enable receive packet pending interrupt
                                    0 = Disable receive packet pending interrupt */
#define EIE_DMAIE       (1<<5)  /*  1 = Enable DMA interrupt
                                    0 = Disable DMA interrupt */
#define EIE_LINKIE      (1<<4)  /*  1 = Enable link change interrupt from the PHY
                                    0 = Disable link change interrupt */
#define EIE_TXIE        (1<<3)  /*  1 = Enable transmit interrupt
                                    0 = Disable transmit interrupt */
#define EIE_WOLIE       (1<<2)  /*  1 = Allow WOL interrupt events to drive the WOL pin
                                    0 = Disable all WOL pin activity (pin is continuously driven high) */
#define EIE_TXERIE      (1<<1)  /*  1 = Enable transmit error interrupt
                                    0 = Disable transmit error interrupt */
#define EIE_RXERIE      (1<<0)  /*  1 = Enable receive error interrupt
                                    0 = Disable receive error interrupt */

/* EIR bit definitions */
#define EIR_PKTIF       (1<<6)  /*  1 = Receive buffer contains one or more unprocessed packets; cleared when PKTDEC is set
                                    0 = Receive buffer is empty */
#define EIR_DMAIF       (1<<5)  /*  1 = DMA copy or checksum calculation has completed
                                    0 = No DMA interrupt is pending */
#define EIR_LINKIF      (1<<4)  /*  1 = PHY reports that the link status has changed; read PHIR register to clear
                                    0 = Link status has not changed */
#define EIR_TXIF        (1<<3)  /*  1 = Transmit request has ended
                                    0 = No transmit interrupt is pending */
#define EIE_WOLIF       (1<<2)  /*  1 = Wake-up on LAN interrupt is pending
                                    0 = No Wake-up on LAN interrupt is pending */
#define EIR_TXERIF      (1<<1)  /*  1 = A transmit error has occurred
                                    0 = No transmit error has occurred */
#define EIR_RXERIF      (1<<0)  /*  1 = A packet was aborted because there is insufficient buffer space or the packet count is 255
                                    0 = No receive error interrupt is pending */

/* ESTAT bit definitions */
#define ESTAT_INT       (1<<7)  /*  1 = INT interrupt is pending
                                    0 = No INT interrupt is pending */
#define ESTAT_LATECOL   (1<<4)  /*  1 = A collision occurred after 64 bytes have been transmitted
                                    0 = No collisions after 64 bytes have occurred */
#define ESTAT_RXBUSY    (1<<2)  /*  1 = Receive logic is receiving a data packet
                                    0 = Receive logic is Idle */
#define ESTAT_TXABRT    (1<<1)  /*  1 = The transmit request was aborted
                                    0 = No transmit abort error */
#define ESTAT_CLKRDY    (1<<0)  /*  1 = OST has expired; PHY is ready
                                    0 = OST is still counting; PHY is not ready */

/* ECON2 bit definitions */
#define ECON2_AUTOINC   (1<<7)  /*  1 = Automatically increment ERDPT and EWRPT when the SPI RBM/WBM command is used
                                    0 = Do not automatically change ERDPT and EWRPT after the buffer is accessed */
#define ECON2_PKTDEC    (1<<6)  /*  1 = Decrement the EPKTCNT register by one
                                    0 = Leave EPKTCNT unchanged */
#define ECON2_PWRSV     (1<<5)  /*  1 = MAC, PHY and control logic are in Low-Power Sleep mode
                                    0 = Normal operation */
#define ECON2_VRPS      (1<<3)  /* When PWRSV = 1:
                                    1 = Internal voltage regulator is in Low-Current mode
                                    0 = Internal voltage regulator is in Normal Current mode
                                   When PWRSV = 0:
                                    The bit is ignored; the regulator always outputs as much current as the device requires. */

/* ECON1 bit definitions */
#define ECON1_TXRST     (1<<7)  /*  1 = Transmit logic is held in Reset
                                    0 = Normal operation */
#define ECON1_RXRST     (1<<6)  /*  1 = Receive logic is held in Reset
                                    0 = Normal operation */
#define ECON1_DMAST     (1<<5)  /*  1 = DMA copy or checksum operation is in progress
                                    0 = DMA hardware is Idle */
#define ECON1_CSUMEN    (1<<4)  /*  1 = DMA hardware calculates checksums
                                    0 = DMA hardware copies buffer memory */
#define ECON1_TXRTS     (1<<3)  /*  1 = The transmit logic is attempting to transmit a packet
                                    0 = The transmit logic is Idle */
#define ECON1_RXEN      (1<<2)  /*  1 = Packets which pass the current filter configuration will be written into the receive buffer
                                    0 = All packets received will be ignored */
#define ECON1_BSEL1     (1<<1)  /*  11 = SPI accesses registers in Bank 3 */
#define ECON1_BSEL0     (1<<0)  /*  10 = SPI accesses registers in Bank 2 */
                                /*  01 = SPI accesses registers in Bank 1 */
                                /*  00 = SPI accesses registers in Bank 0 */

/* Bank 1 registers */
#define EHT0            0x00
#define EHT1            0x01
#define EHT2            0x02
#define EHT3            0x03
#define EHT4            0x04
#define EHT5            0x05
#define EHT6            0x06
#define EHT7            0x07
#define EPMM0           0x08
#define EPMM1           0x09
#define EPMM2           0x0A
#define EPMM3           0x0B
#define EPMM4           0x0C
#define EPMM5           0x0D
#define EPMM6           0x0E
#define EPMM7           0x0F
#define EPMCSL          0x10
#define EPMCSH          0x11
/* unused               0x12 */
/* unused               0x13 */
#define EPMOL           0x14
#define EPMOH           0x15
#define EWOLIE          0x16
#define EWOLIR          0x17
#define ERXFCON         0x18
#define EPKTCNT         0x19
/* reserved             0x1A */
/* #define EIE          0x1B */
/* #define EIR          0x1C */
/* #define ESTAT        0x1D */
/* #define ECON2        0x1E */
/* #define ECON1        0x1F */

/* EWOLIE bit definitions */
#define EWOLIE_UCWOLIE  (1<<7)  /*  1 = Enable Unicast Wake-up on LAN interrupt
                                    0 = Disable Unicast Wake-up on LAN interrupt */
#define EWOLIE_AWOLIE   (1<<6)  /*  1 = Enable Any Packet Wake-up on LAN interrupt
                                    0 = Disable Any Packet Wake-up on LAN interrupt */
#define EWOLIE_PMWOLIE  (1<<4)  /*  1 = Enable Pattern Match Wake-up on LAN interrupt
                                    0 = Disable Pattern Match Wake-up on LAN interrupt */
#define EWOLIE_MPWOLIE  (1<<3)  /*  1 = Enable Magic Packet Wake-up on LAN interrupt
                                    0 = Disable Magic Packet Wake-up on LAN interrupt */
#define EWOLIE_HTWOLIE  (1<<2)  /*  1 = Enable Hash Table Wake-up on LAN interrupt
                                    0 = Disable Hash Table Wake-up on LAN interrupt */
#define EWOLIE_MCWOLIE  (1<<1)  /*  1 = Enable Mulitcast Packet Wake-up on LAN interrupt
                                    0 = Disable Multicast Packet Wake-up on LAN interrupt */
#define EWOLIE_BCWOLIE  (1<<0)  /*  1 = Enable Broadcast Packet Wake-up on LAN interrupt
                                    0 = Disable Broadcast Packet Wake-up on LAN interrupt */

/* EWOLIR bit definitions */
#define EWOLIR_UCWOLIF  (1<<7)  /*  1 = A packet was received with a destination address matching the local MAC address
                                    0 = No unicast packets matching the local MAC address have been received */
#define EWOLIR_AWOLIF   (1<<6)  /*  1 = A packet was received
                                    0 = No packets have been received */
#define EWOLIR_PMWOLIF  (1<<4)  /*  1 = A packet was received which passed the pattern match filter critieria
                                    0 = No packets matching the pattern match criteria have been received */
#define EWOLIR_MPWOLIF  (1<<3)  /*  1 = A Magic Packet for the local MAC address was received
                                    0 = No Magic Packets for the local MAC address have been received */
#define EWOLIR_HTWOLIF  (1<<2)  /*  1 = A packet was received which passed the hash table filter criteria
                                    0 = No packets matching the hash table filter criteria have been received */
#define EWOLIR_MCWOLIF  (1<<1)  /*  1 = A packet was received with a multicast destination address
                                    0 = No packets with a multicast destination address have been received */
#define EWOLIR_BCWOLIF  (1<<0)  /*  1 = A packet was received with a destination address of FF-FF-FF-FF-FF-FF
                                    0 = No packets with a broadcast destination address have been received */

/* ERXFCON bit definitions */
#define ERXFCON_UCEN    (1<<7)  /* When ANDOR = 1:
                                    1 = Packets not having a destination address matching the local MAC address will be discarded
                                    0 = Filter disabled
                                   When ANDOR = 0:
                                    1 = Packets with a destination address matching the local MAC address will be accepted
                                    0 = Filter disabled */
#define ERXFCON_ANDOR   (1<<6)  /*  1 = AND: Packets will be rejected unless all enabled filters accept the packet
                                    0 = OR: Packets will be accepted unless all enabled filters reject the packet */
#define ERXFCON_CRCEN   (1<<5)  /*  1 = All packets with an invalid CRC will be discarded
                                    0 = The CRC validity will be ignored */
#define ERXFCON_PMEN    (1<<4)  /* When ANDOR = 1:
                                    1 = Packets must meet the pattern match criteria or they will be discarded
                                    0 = Filter disabled
                                   When ANDOR = 0:
                                    1 = Packets which meet the pattern match criteria will be accepted
                                    0 = Filter disabled */
#define ERXFCON_MPEN    (1<<3)  /* When ANDOR = 1:
                                    1 = Packets must be Magic Packets for the local MAC address or they will be discarded
                                    0 = Filter disabled
                                   When ANDOR = 0:
                                    1 = Magic Packets for the local MAC address will be accepted
                                    0 = Filter disabled */
#define ERXFCON_HTEN    (1<<2)  /* When ANDOR = 1:
                                    1 = Packets must meet the hash table criteria or they will be discarded
                                    0 = Filter disabled
                                   When ANDOR = 0:
                                    1 = Packets which meet the hash table criteria will be accepted
                                    0 = Filter disabled */
#define ERXFCON_MCEN    (1<<1)  /* When ANDOR = 1:
                                    1 = Packets must have the Least Significant bit set in the destination address or they will be discarded
                                    0 = Filter disabled
                                   When ANDOR = 0:
                                    1 = Packets which have the Least Significant bit set in the destination address will be accepted
                                    0 = Filter disabled */
#define ERXFCON_BCEN    (1<<0)  /* When ANDOR = 1:
                                    1 = Packets must have a destination address of FF-FF-FF-FF-FF-FF or they will be discarded
                                    0 = Filter disabled
                                   When ANDOR = 0:
                                    1 = Packets which have a destination address of FF-FF-FF-FF-FF-FF will be accepted
                                    0 = Filter disabled */

/* Bank 2 registers */
#define MACON1          0x00
#define MACON2          0x01
#define MACON3          0x02
#define MACON4          0x03
#define MABBIPG         0x04
/* unused               0x05 */
#define MAIPGL          0x06
#define MAIPGH          0x07
#define MACLCON1        0x08
#define MACLCON2        0x09
#define MAMXFLL         0x0A
#define MAMXFLH         0x0B
/* reserved             0x0C */
#define MAPHSUP         0x0D
/* reserved             0x0E */
/* unused               0x0F */
/* reserved             0x10 */
#define MICON           0x11
#define MICMD           0x12
/* unused               0x13 */
#define MIREGADR        0x14
/* reserved             0x15 */
#define MIWRL           0x16
#define MIWRH           0x17
#define MIRDL           0x18
#define MIRDH           0x19
/* reserved             0x1A */
/* #define EIE          0x1B */
/* #define EIR          0x1C */
/* #define ESTAT        0x1D */
/* #define ECON2        0x1E */
/* #define ECON1        0x1F */

/* MACON1 bit definitions */
#define MACON1_LOOPBK   (1<<4)  /*  1 = All data transmitted by the MAC will be looped back to the MAC
                                    0 = Normal operation */
#define MACON1_TXPAUS   (1<<3)  /*  1 = Allow the MAC to transmit pause control frames (needed for flow control in full duplex)
                                    0 = Disallow pause frame transmissions */
#define MACON1_RXPAUS   (1<<2)  /*  1 = Inhibit transmissions when pause control frames are received (normal operation)
                                    0 = Ignore pause control frames which are received */
#define MACON1_PASSALL  (1<<1)  /*  1 = Control frames received by the MAC will be wirtten into the receive buffer if not filtered out
                                    0 = Control frames will be discarded after being processed by the MAC (normal operation) */
#define MACON1_MARXEN   (1<<0)  /*  1 = Enable packets to be received by the MAC
                                    0 = Disable packet reception */

/* MACON2 bit definitions */
#define MACON2_MARST    (1<<7)  /*  1 = Entire MAC is held in Reset
                                    0 = Normal operation */
#define MACON2_RNDRST   (1<<6)  /*  1 = Random number generator used in transmit function is held in Reset
                                    0 = Normal operation */
#define MACON2_MARXRST  (1<<3)  /*  1 = MAC control sublayer and receiver logic are held in Reset
                                    0 = Normal operation */
#define MACON2_RFUNRST  (1<<2)  /*  1 = MAC receive logic is held in Reset
                                    0 = Normal operation */
#define MACON2_MATXRST  (1<<1)  /*  1 = MAC control sublayer and transmit logic are held in Reset
                                    0 = Normal operation */
#define MACON2_TFUNRST  (1<<0)  /*  1 = MAC transmit logic is held in Reset
                                    0 = Normal operation */

/* MACON3 bit definitions */
#define MACON3_PADCFG2  (1<<7)  /*  111 = All short frames will be zero padded to 64 bytes and a valid CRC will then be appended */
#define MACON3_PADCFG1  (1<<6)  /*  110 = No automatic padding of short frames */
#define MACON3_PADCFG0  (1<<5)  /*  101 = MAC will automatically detect VLAN Protocol frames which have a 8100h type field and */
                                /*        automatically pad to 64 bytes. If the frame is not a VLAN frame, it will be padded to */
                                /*        60 bytes. After padding, a valid CRC will be appended. */
                                /*  100 = No automatic padding of short frames */
                                /*  011 = All short frames will be zero padded to 64 bytes and a valid CRC will then be appended */
                                /*  010 = No automatic padding of short frames */
                                /*  001 = All short frames will be zero padded to 60 bytes and a valid CRC will then be appended */
                                /*  000 = No automatic padding of short frames */
#define MACON3_TXCRCEN  (1<<4)  /*  1 = MAC will apend a valid CRC to all frames transmitted regardless of PADCFG. TXCRCEN
                                        must be set if PADCFG specifies that a valid CRC will be appended.
                                    0 = MAC will not append a CRC. The last 4 bytes will be checked and if it is an invalid CRC, it
                                        will be reported in the transmit status vector. */
#define MACON3_PHDRLEN  (1<<3)  /*  1 = Frames presented to the MAC contain a 4-byte proprietary header which will not be used
                                        when calculating the CRC
                                    0 = No proprietary header is present. The CRC will cover all data (normal operation). */
#define MACON3_HFRMEN   (1<<2)  /*  1 = Frames of any size will be allowed to be transmitted and receieved
                                    0 = Frames bigger than MAMXFL will be aborted when transmitted or received */
#define MACON3_FRMLNEN  (1<<1)  /*  1 = The type/length field of transmitted and received frames will be checked. If it represents a
                                        length, the frame size will be compared and mismatches will be reported in the
                                        transmit/receive status vector.
                                    0 = Frame lengths will not be compared with the type/length field */
#define MACON3_FULDPX   (1<<0)  /*  1 = MAC will operate in full-duplex. PHCON1.PDPXMD must also be set.
                                    0 = MAC will operate in half-duplex. PHCON1.PDPXMD must also be clear. */

/* MACON4 bit definitions */
#define MACON4_DEFER    (1<<6)  /*  1 = When the medium is occupied, the MAC will wait indefinately for it to become free
                                        when attempting to transmit
                                    0 = When the medium is occupied, the MAC will abort the transmission after the excessive
                                        deferral limit is reached (2.4287 ms) */
#define MACON4_BPEN     (1<<5)  /*  1 = After incidentally causing a collission during back pressure, the MAC will immediately
                                        begin retransmitting
                                    0 = After incidentally causing a collision during back pressure, the MAC will delay using the
                                        binary exponential backoff algorithm before attempting to retransmit (normal operation) */
#define MACON4_NOBKOFF  (1<<4)  /*  1 = After any collision, the MAC will immediately begin retransmitting
                                    0 = After any collision, the MAC will delay using the binary exponential backoff algorithm
                                        before attempting to retransmit (normal operation) */
#define MACON4_LONGPRE  (1<<1)  /*  1 = Received packets will be rejected if they are preceded by 12 or more bytes of preamble
                                    0 = Received packets will not be rejected if they have 12 or more bytes of preamble (normal
                                        operation) */
#define MACON4_PUREPRE  (1<<0)  /*  1 = The preamble of received packets will be checked against 55h. If it contains an error, the
                                        packet will be discarded.
                                    0 = The preamble of received packets will not be checked */

/* MICMD bit definitions */
#define MICMD_MIISCAN   (1<<1)  /*  1 = PHY register at MIREGADR is continously read and the data is placed in MIRD
                                    0 = No MII management scan operation is in progress */
#define MICMD_MIIRD     (1<<0)  /*  1 = PHY register at MIREGADR is read once and the data is placed in MIRD
                                    0 = No MII management read operation is in progress */

/* Bank 3 registers */
#define MAADR1          0x00
#define MAADR0          0x01
#define MAADR3          0x02
#define MAADR2          0x03
#define MAADR5          0x04
#define MAADR4          0x05
#define EBSTSD          0x06
#define EBSTCON         0x07
#define EBSTCSL         0x08
#define EBSTCSH         0x09
#define MISTAT          0x0A
/* unused               0x0B */
/* unused               0x0C */
/* unused               0x0D */
/* unused               0x0E */
/* unused               0x0F */
/* unused               0x10 */
/* unused               0x11 */
#define EREVID          0x12
/* unused               0x13 */
/* unused               0x14 */
#define ECOCON          0x15
/* reserved             0x16 */
#define EFLOCON         0x17
#define EPAUSL          0x18
#define EPAUSH          0x19
/* reserved             0x1A */
/* #define EIE          0x1B */
/* #define EIR          0x1C */
/* #define ESTAT        0x1D */
/* #define ECON2        0x1E */
/* #define ECON1        0x1F */

/* MISTAT bits */
#define MISTAT_NVALID   (1<<2)  /*  1 = The contents of MIRD are not valid yet
                                    0 = The MII management read cycle has completed and MIRD has been updated */
#define MISTAT_SCAN     (1<<1)  /*  1 = MII management scan operation is in progress
                                    0 = No MII management scan operation is in progress */
#define MISTAT_BUSY     (1<<0)  /*  1 = A PHY register is currently being read or written to
                                    0 = The MII management interface is Idle */

/* PHY registers; these are in a whole different memory range, and cannot be accessed directly. The MII registers are used for that. */
#define PHCON1          0x00
#define PHSTAT1         0x01
#define PHID1           0x02
#define PHID2           0x03
#define PHCON2          0x10
#define PHSTAT2         0x11
#define PHIE            0x12
#define PHIR            0x13
#define PHLCON          0x14

/* PHCON1 bit definitions */
#define PHCON1_PRST     (1<<15) /*  1 = PHY is processing a Software Reset (automatically resets to ‘0’ when done)
                                    0 = Normal operation */
#define PHCON1_PLOOPBK  (1<<14) /*  1 = All data transmitted will be returned to the MAC. The twisted pair interface will be disabled.
                                    0 = Normal operation */
#define PHCON1_PPWRSV   (1<<11) /*  1 = PHY is shut-down
                                    0 = Normal operation */
#define PHCON1_PDPXMD   (1<<8)  /*  1 = PHY operates in Full-Duplex mode
                                    0 = PHY operates in Half-Duplex mode */

/* PHSTAT1 bit definitions */
#define PHSTAT1_LLSTAT  (1<<2)  /*  1 = Link is up and has been up continously since PHSTAT1 was last read
                                    0 = Link is down or was down for a period since PHSTAT1 was last read */
#define PHSTAT1_JBRSTAT (1<<1)  /*  1 = PHY has detected a transmission meeting the jabber criteria since PHYSTAT1 was last read
                                    0 = PHY has not detected any jabbering transmissions since PHYSTAT1 was last read */

/* PHCON2 bit definitions */
#define PHCON2_FRCLNK   (1<<14) /*  1 = Force linkup even when no link partner is detected
                                    0 = Normal operation */
#define PHCON2_TXDIS    (1<<13) /*  1 = Disable twisted pair transmitter
                                    0 = Normal operation */
#define PHCON2_JABBER   (1<<10) /*  1 = Disable jabber correction
                                    0 = Normal operation */
#define PHCON2_HDLDIS   (1<<8)  /* When PHCON1.PDPXMD = 1 or PHCON1.PLOOPBK = 1:
                                    This bit is ignored.
                                   When PHCON1.PDPXMD = 0 and PHCON1.PLOOPBK = 0:
                                    1 = Transmitted data will only be sent out on the twisted pair interface
                                    0 = Transmitted data will be looped back to the MAC and sent out the twisted pair interface */

/* PHSTAT2 bit definitions */
#define PHSTAT2_TXSTAT  (1<<13) /*  1 = PHY is transmitting data
                                    0 = PHY is not transmitting data */
#define PHSTAT2_RXSTAT  (1<<12) /*  1 = PHY is receiving data
                                    0 = PHY is not receiving data */
#define PHSTAT2_COLSTAT (1<<11) /*  1 = A collision is occuring
                                    0 = A collision is not occuring */
#define PHSTAT2_LSTAT   (1<<10) /*  1 = Link is up
                                    0 = Link is down */
#define PHSTAT2_DPXSTAT (1<<9)  /*  1 = PHY is configured for full-duplex operation (PHCON1.PDPXMD is set)
                                    0 = PHY is configured for half-duplex operation (PHCON1.PDPXMD is clear) */

#define PHSTAT2_PLRITY  (1<<4)  /*  1 = The polarity of the signal on TPIN+/TPIN- is reversed
                                    0 = The polarity of the signal on TPIN+/TPIN- is correct */

/* PHIE bit definitions */
#define PHIE_PLNKIE     (1<<4)  /*  1 = PHY link change interrupt is enabled
                                    0 = PHY link change interrupt is disabled */
#define PHIE_PGEIE      (1<<1)  /*  1 = PHY interrupts are enabled
                                    0 = PHY interrupts are disabled */

/* Struct used as an 'overlay' for the reception header */
typedef struct {
    unsigned short nextpacketpointer;       /* Bit 0-15 (16) */
    unsigned short bytecount;               /* Bit 16-31 (16) */
    unsigned short rxstatus;                /* Bit 32-47 (16) */
} receptionvector_t;

/* rxstatus bit-definitions */
#define RXSTATUS_LONGDROPEVENT  (1<<(16-16))      /* Long Event/Drop Event */
//Reserved
#define RXSTATUS_CARRIERSEEN    (1<<(18-16))      /* Carrier Event Previously Seen */
//Reserved
#define RXSTATUS_CRCERROR       (1<<(20-16))      /* CRC Error */
#define RXSTATUS_LENGTHCHKERROR (1<<(21-16))      /* Length Check Error */
#define RXSTATUS_LENGTHOUTRANGE (1<<(22-16))      /* Length Out of Range */
#define RXSTATUS_OK             (1<<(23-16))      /* Received Ok */
#define RXSTATUS_MULTICAST      (1<<(24-16))      /* Receive Multicast Packet */
#define RXSTATUS_BROADCAST      (1<<(25-16))      /* Receive Broadcast Packet */
#define RXSTATUS_DRIBBLENIBBLE  (1<<(26-16))      /* Dribble Nibble */
#define RXSTATUS_CTRL           (1<<(27-16))      /* Receive Control Frame */
#define RXSTATUS_CTRLPAUSE      (1<<(28-16))      /* Receive Pause Control Frame */
#define RXSTATUS_CTRLUNKNOWN    (1<<(29-16))      /* Receive Unknown Opcode */
#define RXSTATUS_VLAN           (1<<(30-16))      /* Receive VLAN Type Detected */

/* And another one for the transmission status vector */
typedef struct {
    unsigned short bytecount;               /* Bit 0-15 (16) */
    unsigned short txstatus1;               /* Bit 16-31 (16) */
    unsigned short bytecountraw;            /* Bit 32-47 (16) */
    unsigned char txstatus2;                /* Bit 48-55 (8) */
} transmissionvector_t;

/* txstatus1 bit-definitions */
#define TXSTATUS1_COLLCNT_MASK   0x0F       /* First four bytes of txstatus1 contains the number of collisions the current packet
                                               incurred during transmission attempts. It applies to successfully transmitted packets
                                               and as such, will not show the possible maximum count of 16 collisions */
#define TXSTATUS1_CRCERROR       (1<<4)     /* The attached CRC in the packet did not match the internally generated CRC */
#define TXSTATUS1_LENGTHCHKERROR (1<<5)     /* Indicates that frame length field value in the packet does not match the
                                               actual data byte length and is not a type field. MACON3.FRMLNEN
                                               must be set to get this error*/
#define TXSTATUS1_LENGTHOUTRANGE (1<<6)     /* Indicates that frame type/length field was larger than 1500 bytes (type field) */
#define TXSTATUS1_DONE           (1<<7)     /* Transmission of the packet was completed */
#define TXSTATUS1_MULTICAST      (1<<8)     /* Packet’s destination address was a Multicast address */
#define TXSTATUS1_BROADCAST      (1<<9)     /* Packet’s destination address was a Broadcast address */
#define TXSTATUS1_PACKETDEFER    (1<<10)    /* Packet was deferred for at least one attempt but less than an excessive defer */
#define TXSTATUS1_EXSESDEFER     (1<<11)    /* Packet was deferred in excess of 24,287 bit times (2.4287 ms) */
#define TXSTATUS1_EXSESCOLLISION (1<<12)    /* Packet was aborted after the number of collisions exceeded the retransmission maximum (MACLCON1) */
#define TXSTATUS1_LATECOLLISION  (1<<13)    /* Collision occurred beyond the collision window (MACLCON2) */
#define TXSTATUS1_GIANT          (1<<14)    /* Byte count for frame was greater than MAMXFL */
/* txstatus2 bit-definitions */
#define TXSTATUS2_CTRL           (1<<0)     /* The frame transmitted was a control frame */
#define TXSTATUS2_CTRLPAUSE      (1<<1)     /* The frame transmitted was a control frame with a valid pause opcode */
#define TXSTATUS2_BACKPRESSURE   (1<<2)     /* Carrier sense method backpressure was previously applied */
#define TXSTATUS2_VLAN           (1<<3)     /* Frame’s length/type field contained 8100h which is the VLAN protocol identifier */

#endif /* ENC28J60_REGISTERS_H */
