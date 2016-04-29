/*
 * Copyright (C) 2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 * @ingroup     ppp_ipcp
 * @file
 * @brief       Implementation of PPP's IPCP protocol
 *
 * @author      José Ignacio Alamos <jialamos@uc.cl>
 * @}
 */

#include "net/gnrc/ppp/ipcp.h"
#include "net/gnrc/ppp/ppp.h"

#include "net/ppp/hdr.h"

#include "net/gnrc/ppp/opt.h"
#include "net/gnrc/ppp/fsm.h"
#include "net/gnrc/pkt.h"
#include "net/gnrc/pktbuf.h"
#include "net/gnrc/nettype.h"
#include "net/ipv4/hdr.h"
#include "net/icmp.h"
#include <errno.h>

#define ENABLE_DEBUG    (1)
#include "debug.h"

#if ENABLE_DEBUG
/* For PRIu16 etc. */
#include <inttypes.h>
#endif


static cp_conf_t *ipcp_get_conf_by_code(ppp_fsm_t *cp, uint8_t code)
{
	switch(code)
	{
		case 3:
			return &cp->conf[IPCP_IPADDRESS];
		default:
			return NULL;
	}
}

uint8_t ipcp_ipaddress_is_valid(ppp_option_t *opt)
{
	return true;
}

void ipcp_ipaddress_handle_nak(struct cp_conf_t *conf, ppp_option_t *opt)
{
	conf->value = *((network_uint32_t*) ppp_opt_get_payload(opt));
	if(!(conf->flags & OPT_ENABLED))
		conf->flags |= OPT_ENABLED;
}

uint8_t ipcp_ipaddress_build_nak_opts(uint8_t *buf)
{
	return 0;
}
void ipcp_ipaddress_set(ppp_fsm_t *ipcp, ppp_option_t *opt, uint8_t peer)
{
	if(!peer)
		((ipcp_t*) ipcp)->ip = *((ipv4_addr_t*) ppp_opt_get_payload(opt));
	else
		((ipcp_t*) ipcp)->local_ip = *((ipv4_addr_t*) ppp_opt_get_payload(opt));
}

int ppp_ipv4_handler(ppp_protocol_t *prot, uint8_t event, gnrc_pktsnip_t *pkt)
{
	DEBUG("Received an IPv4 packet!!\n");
	/*For now, just drop it*/
	gnrc_pktbuf_release(pkt);
	return 0;
}
static void ipcp_config_init(ppp_fsm_t *ipcp)
{
	ipcp->conf = IPCP_NUMOPTS ? ((ipcp_t*) ipcp)->ipcp_opts : NULL;

	ipcp->conf[IPCP_IPADDRESS].type = 3;
	ipcp->conf[IPCP_IPADDRESS].value = byteorder_htonl(0);
	ipcp->conf[IPCP_IPADDRESS].size = 4;
	ipcp->conf[IPCP_IPADDRESS].flags = OPT_ENABLED;
	ipcp->conf[IPCP_IPADDRESS].next = NULL;
	ipcp->conf[IPCP_IPADDRESS].is_valid = &ipcp_ipaddress_is_valid;
	ipcp->conf[IPCP_IPADDRESS].handle_nak = &ipcp_ipaddress_handle_nak;
	ipcp->conf[IPCP_IPADDRESS].build_nak_opts = &ipcp_ipaddress_build_nak_opts;
	ipcp->conf[IPCP_IPADDRESS].set = &ipcp_ipaddress_set;
}

int ipcp_init(gnrc_pppdev_t *ppp_dev, ppp_fsm_t *ipcp)
{
	cp_init(ppp_dev, ipcp);
	ipcp_config_init(ipcp);

	ipcp->supported_codes = FLAG_CONF_REQ | FLAG_CONF_ACK | FLAG_CONF_NAK | FLAG_CONF_REJ | FLAG_TERM_REQ | FLAG_TERM_ACK | FLAG_CODE_REJ;
	((ppp_protocol_t*) ipcp)->id = ID_IPCP;
	ipcp->prottype = GNRC_NETTYPE_IPCP;
	ipcp->restart_timer = IPCP_RESTART_TIMER;
	ipcp->get_conf_by_code = &ipcp_get_conf_by_code;
	ipcp->prot.handler = &fsm_handle_ppp_msg;
	ipcp->targets = ((ID_LCP << 8) & 0xffff) | (ID_IPV4 & 0xffff);
	return 0;
}

/** BEGIN OF TEST SECTION. PLEASE REMOVE LATER **/

static uint16_t checksum(uint8_t *data, size_t chunks)
{
	uint32_t acc=0;
	network_uint16_t u16;
	for(int i=0;i<chunks;i++)
	{
		memcpy(&u16, data+2*i, 2);
		acc+=byteorder_ntohs(u16);
		if(acc > 0xffff)
			acc-=0xffff;
	}
	uint16_t ret = acc ^ 0xffff;
	return ret;
}
gnrc_pktsnip_t *gen_icmp_echo(void)
{
	gnrc_pktsnip_t *pkt = gnrc_pktbuf_add(NULL, NULL, sizeof(icmp_hdr_t), GNRC_NETTYPE_UNDEF);
	icmp_hdr_t *hdr = pkt->data;

	hdr->type = 8;
	hdr->code = 0;
	hdr->csum = byteorder_htons(0);
	hdr->id = byteorder_htons(0);
	hdr->sn = byteorder_htons(10);

	/* Calculate checksum */
	hdr->csum = byteorder_htons(checksum((uint8_t*) hdr, 4));
	return pkt;
}
gnrc_pktsnip_t *gen_ping_pkt(ipcp_t *ipcp)
{
	gnrc_pktsnip_t *echo = gen_icmp_echo();
	gnrc_pktsnip_t *pkt = gnrc_pktbuf_add(echo, NULL, sizeof(ipv4_hdr_t), GNRC_NETTYPE_IPV4);

	ipv4_hdr_t *hdr = pkt->data;	
	
	ipv4_addr_t dst;
	dst.u8[0] = 8;
	dst.u8[1] = 8;
	dst.u8[2] = 8;
	dst.u8[3] = 8;

	ipv4_addr_t src = ipcp->ip;

	ipv4_hdr_set_version(hdr);
	ipv4_hdr_set_ihl(hdr, 5);
	ipv4_hdr_set_ts(hdr, 0);
	ipv4_hdr_set_tl(hdr, sizeof(ipv4_hdr_t)+sizeof(icmp_hdr_t));
	ipv4_hdr_set_id(hdr, 0);
	ipv4_hdr_set_flags(hdr, 0);
	ipv4_hdr_set_fo(hdr, 0);
	ipv4_hdr_set_ttl(hdr, 64);
	ipv4_hdr_set_protocol(hdr, 1);
	ipv4_hdr_set_csum(hdr, 0);
	ipv4_hdr_set_src(hdr, src);
	ipv4_hdr_set_dst(hdr, dst);

	/*Calculate checkshum*/
	ipv4_hdr_set_csum(hdr, checksum((uint8_t*) hdr, 10));
	
	return pkt;
}

/** END OF TEST SECTION **/

int handle_ipv4(struct ppp_protocol_t *protocol, uint8_t ppp_event, void *args)
{
	ipcp_t *ipcp = ((ppp_ipv4_t*) protocol)->ipcp;
	pppdev_t *pppdev = ((ppp_ipv4_t*) protocol)->pppdev;
	gnrc_pktsnip_t *pkt;
	gnrc_pktsnip_t *recv_pkt = (gnrc_pktsnip_t*) args;
	(void) recv_pkt;

	switch(ppp_event)
	{
		case PPP_LINKUP:
			DEBUG("Msg: Obtained IP address! \n");
			DEBUG("Ip address is %i.%i.%i.%i\n",ipcp->ip.u8[0],ipcp->ip.u8[1],ipcp->ip.u8[2],ipcp->ip.u8[3]);	
			DEBUG("Send an ICMP pkt...\n");

			pkt = gen_ping_pkt(ipcp);
			puts("Now send...");
			for(int i=0;i<10;i++)
			{
				pkt = gen_ping_pkt(ipcp);
				gnrc_ppp_send(pppdev, pkt);
			}
			break;
		case PPP_RECV:
			DEBUG("Received IP packet!!\n");
			break;
		default:
			break;

	}
	return 0;
}


int ppp_ipv4_init(gnrc_pppdev_t *ppp_dev, ppp_ipv4_t *ipv4, ipcp_t *ipcp, pppdev_t *pppdev)
{
	((ppp_protocol_t*) ipv4)->handler = &handle_ipv4;
	ipv4->ipcp = ipcp;
	ipv4->pppdev = pppdev;
	return 0;
}
