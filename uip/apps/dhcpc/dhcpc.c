/*
    Piconet RS232 ethernet interface

    dhcpc.c
           
    Copyright (c) 2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
\file
DHCP client
*/
#include "dhcpc.h"
#include "uip_arp.h"
#include <string.h>

#if ! UIP_CONF_UDP
#error "DHCPC requires UDP; set UIP_CONF_UDP in uip-conf.h"
#endif

#if ! UIP_CONF_BROADCAST
#error "DHCPC requires that broadcast IP packets are accepted; set UIP_CONF_BROADCAST in uip-conf.h"
#endif

#if UIP_FIXEDADDR
#error "DHCPC cannot do it's thing when the addresses are hardcoded; unset UIP_FIXEDADDR in uipopt.h"
#endif

/* DHCP protocol constants */

// Message opcodes
#define BOOTREQUEST                 1
#define BOOTREPLY                   2
// Hardware types
#define HARDWARE_TYPE_ETHER         1
// Flag bits
#define BOOTP_BROADCAST             0x8000
// DHCP options
#define DHCP_OPTION_SUBNET_MASK     1
#define DHCP_OPTION_ROUTER          3
#define DHCP_OPTION_NTP             42
#define DHCP_OPTION_REQ_IPADDR      50
#define DHCP_OPTION_LEASE_TIME      51
#define DHCP_OPTION_MSG_TYPE        53
#define DHCP_OPTION_SERVER_ID       54
#define DHCP_OPTION_REQ_LIST        55
#define DHCP_OPTION_END             255
// DHCP message types
#define DHCPDISCOVER                1
#define DHCPOFFER                   2
#define DHCPREQUEST                 3
#define DHCPACK                     5
#define DHCPRELEASE                 7

static const unsigned char magic_cookie[4] = {99, 130, 83, 99};

/* Our 'random' transmission identifier. Well, no.. it's really not random at all... */
static const unsigned char xid[4] = {0,0,0,0};

/* DHCP packet layout */
typedef struct {
    unsigned char op, htype, hlen, hops;
    unsigned char xid[4];
    unsigned short secs, flags;
    unsigned char ciaddr[4];
    unsigned char yiaddr[4];
    unsigned char siaddr[4];
    unsigned char giaddr[4];
    unsigned char chaddr[16];
    unsigned char sname[64];
    unsigned char file[128];
    unsigned char options_magic[4];
    unsigned char options[308];
} dhcp_msg_t;

/* Length of packet, without the options field */
#define DHCP_MESSAGE_LENGTH         240

/* Parameters received */
dhcp_parameters_t parameters;

/* Application state */
static unsigned char state;

/* States we can be in */
#define STATE_INITIAL               0
#define STATE_GETLEASE              1
#define STATE_SENDING               2
#define STATE_OFFER_RECEIVED        3
#define STATE_CONFIG_RECEIVED       4
#define STATE_RENEWAL_PENDING       5
#define STATE_RELEASE_PENDING       6
#define STATE_RELEASE_SEND          7

/* The UDP connection used */
static struct uip_udp_conn *conn;

/* Local functions */
static void create_msg(dhcp_msg_t *message);
static void send_discovery(void);
static void send_request(void);
static void send_release(void);
static char parse_message(void);

extern void network_dhcpupdate(dhcp_parameters_t *parameters);

void dhcpc_init(void)
{
    const uip_ipaddr_t addr = {0xFFFF, 0xFFFF};

    /* Set state to initial */
    state = STATE_INITIAL;

    /* Make a new UDP connection, which sends to 255.255.255.255 at port 67 */
    conn = uip_udp_new(&addr, HTONS(67));
    if(conn != NULL) {
        /* Connection was created successfully; start listening on the DHCPC client port */
        uip_udp_bind(conn, HTONS(68)); 
    }
    else {
        UIP_LOG("could not create an UDP connection for DHCPC");   
    }  
}

void dhcpc_appcall(void)
{
    static char requestresend, timeout, prescaler;

    switch(state) {
        case STATE_INITIAL:
            /* Do nothing */
            break;
        case STATE_GETLEASE:
            /* Send initial discovery */
            state = STATE_SENDING;
            send_discovery();
            timeout=0;
            UIP_DEBUG("dhcpc: send discovery\n\r");
            break;
        case STATE_SENDING:
            if(uip_newdata()) {
                /* Response to our discovery?! */
                if(parse_message() == DHCPOFFER) {
                    state = STATE_OFFER_RECEIVED;
                    /* Send a request for this offer */
                    send_request();
                    requestresend = 0;
                    timeout = 0;
                    UIP_DEBUG("dhcpc: send request\n\r");
                }
                else {
                    UIP_DEBUG("dhcpc: unexpected message type, expected DHCPOFFER\n\r"); 
                    state = STATE_SENDING;
                    send_discovery();
                    timeout = 0;
                }
            }
            else if(uip_poll()) {
                timeout++;
                if(timeout == 6) { // 2.5~3 seconds
                    /* No response received, let's try it again */
                    send_discovery();
                    timeout=0;
                    UIP_DEBUG("dhcpc: resend discovery\n\r");
                }
            }
            break;
        case STATE_OFFER_RECEIVED:
            if(uip_newdata()) {
                /* Response to our discovery?! */
                if(parse_message() == DHCPACK) {
                    state = STATE_CONFIG_RECEIVED;
                    prescaler = 0;
                    /* Okay, we got a configuration! */
                    UIP_DEBUG("dhcpc: got configuration\n\r");
                    network_dhcpupdate(&parameters);
                }
                else {
                    UIP_DEBUG("dhcpc: unexpected message type, expected DHCPACK\n\r"); 
                    state = STATE_SENDING;
                    send_discovery();
                    timeout = 0;
                }
            }
            else if(uip_poll()) {
                timeout++;
                if(timeout == 6) { // 2.5~3 seconds
                    /* No response received, let's try it again */
                    requestresend++;
                    if(requestresend < 10) {
                        send_request();
                        timeout = 0;
                        UIP_DEBUG("dhcpc: resend request\n\r");
                    }
                    else {
                        UIP_DEBUG("dhcpc: reset\n\r");
                        state = STATE_SENDING;
                        send_discovery();
                        timeout = 0;
                    }
                }
            }
            break;
        case STATE_CONFIG_RECEIVED:
            if(uip_poll()) {
                prescaler++;
                if(prescaler == 2) {
                    if(parameters.leasetime[0]) {
                        parameters.leasetime[0]--;
                    }
                    else {
                        parameters.leasetime[1]--;
                    }
                    prescaler = 0;
                }

                if(parameters.leasetime[0] == 0 && parameters.leasetime[1] < 120) {
                    /* Time to refresh our lease */
                    send_request();
                    requestresend = 0;
                    timeout = 0;
                    UIP_DEBUG("dhcpc: send renewal request\n\r");
                    state = STATE_OFFER_RECEIVED;
                }
            }
            break;
        case STATE_RENEWAL_PENDING:
            /* User requested renewal */
            send_request();
            requestresend = 0;
            timeout = 0;
            UIP_DEBUG("dhcpc: send renewal request\n\r");
            state = STATE_OFFER_RECEIVED;
            break;
        case STATE_RELEASE_PENDING:
            /* User requested release */
            send_release();
            requestresend = 0;
            timeout = 0;
            UIP_DEBUG("dhcpc: send release request\n\r");
            state = STATE_RELEASE_SEND;
            break;
        case STATE_RELEASE_SEND:
            UIP_DEBUG("dhcpc: shutting down\n\r");
            parameters.serverid[0] = 0;
            parameters.serverid[1] = 0;
            parameters.serverid[2] = 0;
            parameters.serverid[3] = 0;
            parameters.ipaddr[0] = 0;
            parameters.ipaddr[1] = 0;
            parameters.ipaddr[2] = 0;
            parameters.ipaddr[3] = 0;
            parameters.netmask[0] = 0;
            parameters.netmask[1] = 0;
            parameters.netmask[2] = 0;
            parameters.netmask[3] = 0;
            parameters.router[0] = 0;
            parameters.router[1] = 0;
            parameters.router[2] = 0;
            parameters.router[3] = 0;
            parameters.leasetime[0] = 0;
            parameters.leasetime[1] = 0;
            network_dhcpupdate(&parameters);
            state = STATE_INITIAL;
            break;
    }
}

void dhcpc_getlease(void)
{
    if(state == STATE_INITIAL || state == STATE_RELEASE_SEND) {
        state = STATE_GETLEASE;
    }
}

void dhcpc_renew(void)
{
    if(state == STATE_CONFIG_RECEIVED) {
        state = STATE_RENEWAL_PENDING;
    }
}

void dhcpc_release(void)
{
    if(state == STATE_CONFIG_RECEIVED) {
        state = STATE_RELEASE_PENDING;
    }
}

bool dhcpc_running(void)
{
    if(state != STATE_INITIAL && state != STATE_RELEASE_SEND) {
        return TRUE;
    }
    return FALSE;
}

bool dhcpc_gotlease(void)
{
    if(state == STATE_CONFIG_RECEIVED) {
        return TRUE;
    }
    return FALSE;
}

dhcp_parameters_t* dhcpc_getparameters(void)
{
    return &parameters;
}

static void create_msg(dhcp_msg_t *message)
{
    message->op = BOOTREQUEST;
    message->htype = HARDWARE_TYPE_ETHER;
    message->hlen = 6;
    message->hops = 0;
    message->xid[0] = xid[0];
    message->xid[1] = xid[1];
    message->xid[2] = xid[2];
    message->xid[3] = xid[3];
    message->secs = 0;
    message->flags = HTONS(BOOTP_BROADCAST);
    message->ciaddr[0] = uip_ipaddr1(uip_hostaddr);
    message->ciaddr[1] = uip_ipaddr2(uip_hostaddr);
    message->ciaddr[2] = uip_ipaddr3(uip_hostaddr);
    message->ciaddr[3] = uip_ipaddr4(uip_hostaddr);
    message->yiaddr[0] = 0;
    message->yiaddr[1] = 0;
    message->yiaddr[2] = 0;
    message->yiaddr[3] = 0;
    message->siaddr[0] = 0;
    message->siaddr[1] = 0;
    message->siaddr[2] = 0;
    message->siaddr[3] = 0;
    message->giaddr[0] = 0;
    message->giaddr[1] = 0;
    message->giaddr[2] = 0;
    message->giaddr[3] = 0;
    message->chaddr[0] = uip_ethaddr.addr[0];
    message->chaddr[1] = uip_ethaddr.addr[1];
    message->chaddr[2] = uip_ethaddr.addr[2];
    message->chaddr[3] = uip_ethaddr.addr[3];
    message->chaddr[4] = uip_ethaddr.addr[4];
    message->chaddr[5] = uip_ethaddr.addr[5];
    memset((void *)message->sname, 0, sizeof(message->sname));
    memset((void *)message->file, 0, sizeof(message->file));
    message->options_magic[0] = magic_cookie[0];
    message->options_magic[1] = magic_cookie[1];
    message->options_magic[2] = magic_cookie[2];
    message->options_magic[3] = magic_cookie[3];
}

static void send_discovery(void)
{
    dhcp_msg_t *message;

    /* Create new DHCP message header in the uip_appdata storage */    
    message = (dhcp_msg_t *)uip_appdata;
    create_msg(message);
    
    /* Make it a discover package */
    message->options[0] = DHCP_OPTION_MSG_TYPE;
    message->options[1] = 1;
    message->options[2] = DHCPDISCOVER;
    /* And tell the server what information we want to get */
    message->options[3] = DHCP_OPTION_REQ_LIST;
    message->options[4] = 3;
    message->options[5] = DHCP_OPTION_SUBNET_MASK;
    message->options[6] = DHCP_OPTION_ROUTER;
    message->options[7] = DHCP_OPTION_NTP;
    /* Close packet */
    message->options[8] = DHCP_OPTION_END;

    /* And send it */
    uip_udp_send(DHCP_MESSAGE_LENGTH + 9);
}

static void send_request(void)
{
    dhcp_msg_t *message;

    /* Create new DHCP message header in the uip_appdata storage */    
    message = (dhcp_msg_t *)uip_appdata;
    create_msg(message);

    /* Make it a request package */
    message->options[0] = DHCP_OPTION_MSG_TYPE;
    message->options[1] = 1;
    message->options[2] = DHCPREQUEST;
    /* Add server ID (got that from the server's discovery response) */
    message->options[3] = DHCP_OPTION_SERVER_ID;
    message->options[4] = 4;
    message->options[5] = parameters.serverid[0];
    message->options[6] = parameters.serverid[1];
    message->options[7] = parameters.serverid[2];
    message->options[8] = parameters.serverid[3];
    /* And the IP address we got offered */
    message->options[9] = DHCP_OPTION_REQ_IPADDR;
    message->options[10] = 4;
    message->options[11] = parameters.ipaddr[0];
    message->options[12] = parameters.ipaddr[1];
    message->options[13] = parameters.ipaddr[2];
    message->options[14] = parameters.ipaddr[3];
    /* Close packet */
    message->options[15] = DHCP_OPTION_END;

    /* And send it */
    uip_udp_send(DHCP_MESSAGE_LENGTH + 16);
}

static void send_release(void)
{
    dhcp_msg_t *message;

    /* Create new DHCP message header in the uip_appdata storage */    
    message = (dhcp_msg_t *)uip_appdata;
    create_msg(message);
 
    /* Make it a release package */
    message->options[0] = DHCP_OPTION_MSG_TYPE;
    message->options[1] = 1;
    message->options[2] = DHCPRELEASE;
    /* Add server ID (got that from the server's discovery response) */
    message->options[3] = DHCP_OPTION_SERVER_ID;
    message->options[4] = 4;
    message->options[5] = parameters.serverid[0];
    message->options[6] = parameters.serverid[1];
    message->options[7] = parameters.serverid[2];
    message->options[8] = parameters.serverid[3];
    /* Close packet */
    message->options[9] = DHCP_OPTION_END;

    /* And send it */
    uip_udp_send(DHCP_MESSAGE_LENGTH + 10);
}

static char parse_message(void)
{
    dhcp_msg_t *message;
    unsigned char optionspointer = 0;
    unsigned char type = 0;
    
    message = (dhcp_msg_t *)uip_appdata;

    /* If XID and MAC address match, it's for us */
    if(message->op == BOOTREPLY && message->xid[0] == xid[0] &&
                                   message->xid[1] == xid[1] &&
                                   message->xid[2] == xid[2] &&
                                   message->xid[3] == xid[3] &&
                                   message->chaddr[0] == uip_ethaddr.addr[0] &&
                                   message->chaddr[1] == uip_ethaddr.addr[1] &&
                                   message->chaddr[2] == uip_ethaddr.addr[2] &&
                                   message->chaddr[3] == uip_ethaddr.addr[3] &&
                                   message->chaddr[4] == uip_ethaddr.addr[4] &&
                                   message->chaddr[5] == uip_ethaddr.addr[5]) {
        /* Now get information from package */
        parameters.ipaddr[0] = message->yiaddr[0];
        parameters.ipaddr[1] = message->yiaddr[1];
        parameters.ipaddr[2] = message->yiaddr[2];
        parameters.ipaddr[3] = message->yiaddr[3];
        UIP_DEBUG("dhcpc: got ip info, %i.%i.%i.%i\n\r",parameters.ipaddr[0],parameters.ipaddr[1],parameters.ipaddr[2],parameters.ipaddr[3]);
        
        while(optionspointer + DHCP_MESSAGE_LENGTH < uip_datalen()) {
            switch(message->options[optionspointer]) {
                case DHCP_OPTION_SUBNET_MASK:
                    parameters.netmask[0] = message->options[optionspointer+2];
                    parameters.netmask[1] = message->options[optionspointer+3];
                    parameters.netmask[2] = message->options[optionspointer+4];
                    parameters.netmask[3] = message->options[optionspointer+5];
                    UIP_DEBUG("dhcpc: got netmask info, %i.%i.%i.%i\n\r",parameters.netmask[0],parameters.netmask[1],parameters.netmask[2],parameters.netmask[3]);
                    break;
                case DHCP_OPTION_ROUTER:
                    parameters.router[0] = message->options[optionspointer+2];
                    parameters.router[1] = message->options[optionspointer+3];
                    parameters.router[2] = message->options[optionspointer+4];
                    parameters.router[3] = message->options[optionspointer+5];
                    UIP_DEBUG("dhcpc: got gateway info, %i.%i.%i.%i\n\r",parameters.router[0],parameters.router[1],parameters.router[2],parameters.router[3]);
                    break;
                /*case DHCP_OPTION_NTP:
                    parameters.ntp[0] = message->options[optionspointer+2];
                    parameters.ntp[1] = message->options[optionspointer+3];
                    parameters.ntp[2] = message->options[optionspointer+4];
                    parameters.ntp[3] = message->options[optionspointer+5];
                    UIP_DEBUG("dhcpc: got timeserver info, %i.%i.%i.%i\n\r",parameters.ntp[0],parameters.ntp[1],parameters.ntp[2],parameters.ntp[3]);
                    break;*/
                case DHCP_OPTION_MSG_TYPE:
                    type = message->options[optionspointer+2];
                    UIP_DEBUG("dhcpc: messagetype %i\n\r",type);
                    break;
                case DHCP_OPTION_SERVER_ID:
                    parameters.serverid[0] = message->options[optionspointer+2];
                    parameters.serverid[1] = message->options[optionspointer+3];
                    parameters.serverid[2] = message->options[optionspointer+4];
                    parameters.serverid[3] = message->options[optionspointer+5];
                    UIP_DEBUG("dhcpc: serverid %i.%i.%i.%i\n\r",parameters.serverid[0],parameters.serverid[1],parameters.serverid[2],parameters.serverid[3]);
                    break;
                case DHCP_OPTION_LEASE_TIME:
                    parameters.leasetime[0] = message->options[optionspointer+2]<<8 | message->options[optionspointer+3];
                    parameters.leasetime[1] = message->options[optionspointer+4]<<8 | message->options[optionspointer+5];
                    UIP_DEBUG("dhcpc: leasetime %li sec\n\r", (unsigned long)parameters.leasetime[0]<<16 | parameters.leasetime[1]);
                    break;
                case DHCP_OPTION_END:
                    return type;
            }
            optionspointer += message->options[optionspointer+1] + 2;
        }
    }
    return 0;
}
