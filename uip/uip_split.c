/*
 * Copyright (c) 2004, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/*
 * Ported to uIP 1.0, reworked, addition of '_incontroller' logic by
 * Bastiaan van Kesteren <bas@edeation.nl>
 */

#include "uip_split.h"
#include "uip.h"
#include "uip_arp.h"
#include "uip_arch.h"
#include <string.h>

#if UIP_SPLIT

#define BUF ((struct uip_tcpip_hdr *)&uip_buf[UIP_LLH_LEN])

extern u16_t chksum(u16_t sum, const u8_t *data, u16_t len);

static void uip_split_ip(u16_t length)
{
#if UIP_CONF_IPV6
    /* For IPv6, the IP length field does not include the IPv6 IP header length. */
    BUF->len[0] = ((length - UIP_IPH_LEN) >> 8);
    BUF->len[1] = ((length - UIP_IPH_LEN) & 0xff);
#else /* UIP_CONF_IPV6 */
    BUF->len[0] = length >> 8;
    BUF->len[1] = length & 0xff;
#endif /* UIP_CONF_IPV6 */

#if !UIP_CONF_IPV6
    /* Recalculate the IP checksum. */
    BUF->ipchksum = 0;
    BUF->ipchksum = ~(uip_ipchksum());
#endif /* UIP_CONF_IPV6 */
}

static void uip_split_tcp(u16_t checksum_payload)
{
    u16_t upper_layer_len, sum;

    /* Recalculate the TCP checksum. */
    BUF->tcpchksum = 0;
#if UIP_CONF_IPV6
    upper_layer_len = (((u16_t)(BUF->len[0]) << 8) + BUF->len[1]);
#else /* UIP_CONF_IPV6 */
    upper_layer_len = (((u16_t)(BUF->len[0]) << 8) + BUF->len[1]) - UIP_IPH_LEN;
#endif /* UIP_CONF_IPV6 */
    /* Sum pseudoheader */
    sum = upper_layer_len + UIP_PROTO_TCP;
    sum = chksum(sum, (u8_t *)&BUF->srcipaddr[0], 2 * sizeof(uip_ipaddr_t));
    /* Sum TCP header. */
    sum = chksum(sum, &uip_buf[UIP_IPH_LEN + UIP_LLH_LEN], UIP_TCPH_LEN);
    /* Add already calculated payload sum. */
    sum += checksum_payload;
    if(sum < checksum_payload) {
        sum++;
    }
    BUF->tcpchksum = ~( (sum == 0) ? 0xffff : htons(sum) );
}

void uip_split_output(void)
{
    extern u16_t uip_tcpchksum_incontroller_1, uip_tcpchksum_incontroller_2;
    extern u16_t uip_chksum_singlebytes;
    extern void *uip_appdata;
    u16_t tcplen;

    if(uip_appdata == NULL) {
        /* appdata is a NULL-pointer, meaning it's already in the ethernet controller RAM.
           TODO: we assume all 'incontroller' packets are TCP!!! */
        tcplen = uip_len - UIP_TCPIP_HLEN - sizeof(struct uip_eth_hdr);

        if(tcplen > UIP_SPLIT_SIZE) {
            /* Create first packet */
            uip_split_ip(UIP_SPLIT_SIZE + UIP_TCPIP_HLEN);
            uip_split_tcp(uip_tcpchksum_incontroller_1);
			
            /* Transmit first package */
            uip_output_incontroller(0,
			                        UIP_IPTCPH_LEN + UIP_LLH_LEN,
									UIP_SPLIT_SIZE);

            /* Create second package */
            uip_split_ip((tcplen - UIP_SPLIT_SIZE) + UIP_TCPIP_HLEN);
            /* update the TCP sequence number */
            uip_add32(BUF->seqno, UIP_SPLIT_SIZE);
            BUF->seqno[0] = uip_acc32[0];
            BUF->seqno[1] = uip_acc32[1];
            BUF->seqno[2] = uip_acc32[2];
            BUF->seqno[3] = uip_acc32[3];
			uip_split_tcp(uip_tcpchksum_incontroller_2);

            /* Transmit second package */
            uip_output_incontroller(1 + (UIP_IPTCPH_LEN + UIP_LLH_LEN) + UIP_SPLIT_SIZE,
                                    UIP_IPTCPH_LEN + UIP_LLH_LEN,
                                    tcplen - UIP_SPLIT_SIZE);
        }
        else {
            uip_split_tcp(uip_tcpchksum_incontroller_1);

            /* Transmit package */
            uip_output_incontroller(0, UIP_IPTCPH_LEN + UIP_LLH_LEN, tcplen);
        }
        uip_chksum_singlebytes = 0;  
    }
    else {
        /* We only split TCP segments that are larger than or equal to UIP_SPLIT_SIZE, which is configurable through UIP_SPLIT_CONF_SIZE. */
        if(BUF->proto == UIP_PROTO_TCP && uip_len >= UIP_SPLIT_SIZE + UIP_TCPIP_HLEN + sizeof(struct uip_eth_hdr)) {
            tcplen = uip_len - UIP_TCPIP_HLEN - sizeof(struct uip_eth_hdr);

            /* Create first packet */
            uip_split_ip(UIP_SPLIT_SIZE + UIP_TCPIP_HLEN);
            /* Recalculate the TCP checksum. */
            BUF->tcpchksum = 0;
            BUF->tcpchksum = ~(uip_tcpchksum());
			
            /* Transmit first package */
            uip_len = UIP_SPLIT_SIZE + UIP_TCPIP_HLEN + sizeof(struct uip_eth_hdr);
            uip_output();
    
            /* Create second package */
            uip_split_ip((tcplen - UIP_SPLIT_SIZE) + UIP_TCPIP_HLEN);
            /* Reshuffle payload */
            memcpy(uip_appdata, (u8_t *)uip_appdata + UIP_SPLIT_SIZE, tcplen - UIP_SPLIT_SIZE);
            /* update the TCP sequence number */
            uip_add32(BUF->seqno, UIP_SPLIT_SIZE);
            BUF->seqno[0] = uip_acc32[0];
            BUF->seqno[1] = uip_acc32[1];
            BUF->seqno[2] = uip_acc32[2];
            BUF->seqno[3] = uip_acc32[3];
			/* Recalculate the TCP checksum. */
            BUF->tcpchksum = 0;
            BUF->tcpchksum = ~(uip_tcpchksum());

            /* Transmit second package */
            uip_len += sizeof(struct uip_eth_hdr);
            uip_output();
        }
        else {
            /* Packet not big enough (or not a TCP packet!) to make a split worthwhile, send as is. */
            uip_output();
        }
    }
}

#endif /* UIP_SPLIT */
