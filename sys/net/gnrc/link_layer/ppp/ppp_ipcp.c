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
#include "net/udp.h"
#include "net/icmp.h"
#include <errno.h>

#define ENABLE_DEBUG    (1)
#include "debug.h"

#if ENABLE_DEBUG
/* For PRIu16 etc. */
#include <inttypes.h>
#endif

typedef struct __attribute__((packed))
{
	ipv4_addr_t src;
	ipv4_addr_t dst;
	uint8_t zero;
	uint8_t protocol;
	network_uint16_t udp_len;
} udp_phdr_t;

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

static void ipcp_config_init(ppp_fsm_t *ipcp)
{
	ipcp->conf = IPCP_NUMOPTS ? ((ipcp_t*) ipcp)->ipcp_opts : NULL;

	ipcp->conf[IPCP_IPADDRESS].type = 3;
	ipcp->conf[IPCP_IPADDRESS].default_value = byteorder_htonl(0);
	ipcp->conf[IPCP_IPADDRESS].size = 4;
	ipcp->conf[IPCP_IPADDRESS].flags = OPT_ENABLED;
	ipcp->conf[IPCP_IPADDRESS].next = NULL;
	ipcp->conf[IPCP_IPADDRESS].is_valid = &ipcp_ipaddress_is_valid;
	ipcp->conf[IPCP_IPADDRESS].build_nak_opts = &ipcp_ipaddress_build_nak_opts;
	ipcp->conf[IPCP_IPADDRESS].set = &ipcp_ipaddress_set;
}

int ipcp_init(gnrc_pppdev_t *ppp_dev, ppp_fsm_t *ipcp)
{
	fsm_init(ppp_dev, ipcp);
	ipcp_config_init(ipcp);

	((ppp_protocol_t*) ipcp)->pppdev = ppp_dev;
	ipcp->supported_codes = FLAG_CONF_REQ | FLAG_CONF_ACK | FLAG_CONF_NAK | FLAG_CONF_REJ | FLAG_TERM_REQ | FLAG_TERM_ACK | FLAG_CODE_REJ;
	((ppp_protocol_t*) ipcp)->id = ID_IPCP;
	ipcp->prottype = GNRC_NETTYPE_IPCP;
	ipcp->restart_timer = IPCP_RESTART_TIMER;
	ipcp->get_conf_by_code = &ipcp_get_conf_by_code;
	ipcp->prot.handler = &fsm_handle_ppp_msg;
	ipcp->targets = ((ID_LCP << 8) & 0xffff) | (ID_IPV4 & 0xffff);
	((ipcp_t*) ipcp)->ip_id = 123420;
	return 0;
}

/** BEGIN OF TEST SECTION. PLEASE REMOVE LATER **/

static uint32_t _c1_sum(uint32_t acc, uint8_t *data, size_t chunks)
{
	network_uint16_t u16;
	for(int i=0;i<chunks;i++)
	{
		memcpy(&u16, data+2*i, 2);
		acc+=byteorder_ntohs(u16);
		if(acc > 0xffff)
			acc-=0xffff;
	}
	return acc;
}


static uint16_t checksum(gnrc_pktsnip_t *pkt)
{
	uint32_t acc=0;
	network_uint16_t u16 = byteorder_htons(0);
	uint8_t even=0;
	gnrc_pktsnip_t *p = pkt;
	while(p)
	{
		for(int i=0;i<p->size;i++)
		{
			memcpy(((uint8_t*) &u16)+even, ((uint8_t*) p->data)+i, 1);
			if(even)
			{
				acc+=byteorder_ntohs(u16);
				if(acc > 0xffff)
					acc-=0xffff;
				u16 = byteorder_htons(0);
			}
			even ^= 1;
		}
		p = p->next;
	}
	if(even)
	{
		acc+=byteorder_ntohs(u16);
		if(acc > 0xffff)
			acc-=0xffff;

	}
	uint16_t ret = acc ^ 0xffff;
	return ret;
}


static uint16_t pchecksum(uint8_t *data, size_t chunks)
{
	uint32_t acc=0;
	acc = _c1_sum(acc, data, chunks);
	uint16_t ret = acc ^ 0xffff;
	return ret;
}

static uint16_t udp_checksum(ipv4_addr_t src, ipv4_addr_t dst, uint8_t protocol, uint16_t udp_length, gnrc_pktsnip_t *pkt)
{
	uint8_t pseudo_hdr[12];
	udp_phdr_t *phdr= (udp_phdr_t*) pseudo_hdr;

	phdr->src = src;
	phdr->dst = dst;
	phdr->zero = 0;
	phdr->protocol = protocol;
	phdr->udp_len = byteorder_htons(udp_length);

	gnrc_pktsnip_t *chk = gnrc_pktbuf_add(pkt, pseudo_hdr, sizeof(pseudo_hdr), GNRC_NETTYPE_UNDEF);
	uint16_t chksum = checksum(chk);
	chk = gnrc_pktbuf_remove_snip(chk, chk);
	return chksum;
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
	hdr->csum = byteorder_htons(checksum(pkt));
	return pkt;
}

gnrc_pktsnip_t *gen_dummy_pkt(void)
{
	char dummy_data[] = "test";
	return gnrc_pktbuf_add(NULL, dummy_data, sizeof(dummy_data), GNRC_NETTYPE_UNDEF);
}
gnrc_pktsnip_t *gen_ip_pkt(ppp_ipv4_t *ipv4, gnrc_pktsnip_t *payload, uint8_t protocol)
{
	gnrc_pktsnip_t *pkt = gnrc_pktbuf_add(payload, NULL, sizeof(ipv4_hdr_t), GNRC_NETTYPE_IPV4);
	ipv4_hdr_t *hdr = pkt->data;	
	
	ipv4_addr_t dst = ipv4->tunnel_addr;
	ipv4_addr_t src = ((ppp_protocol_t*) ipv4)->pppdev->l_ipcp.ip;

	ipv4_hdr_set_version(hdr);
	ipv4_hdr_set_ihl(hdr, 5);
	ipv4_hdr_set_ts(hdr, 0);
	ipv4_hdr_set_tl(hdr, gnrc_pkt_len(pkt));
	ipv4_hdr_set_id(hdr, ++((ppp_protocol_t*) ipv4)->pppdev->l_ipcp.ip_id);
	ipv4_hdr_set_flags(hdr, 0);
	ipv4_hdr_set_fo(hdr, 0);
	ipv4_hdr_set_ttl(hdr, 64);
	ipv4_hdr_set_protocol(hdr, protocol);
	ipv4_hdr_set_csum(hdr, 0);
	ipv4_hdr_set_src(hdr, src);
	ipv4_hdr_set_dst(hdr, dst);

	/*Calculate checkshum*/
	ipv4_hdr_set_csum(hdr, pchecksum(pkt->data,10));
	
	return pkt;
}

/** END OF TEST SECTION **/

gnrc_pktsnip_t *_build_udp(ppp_ipv4_t *ipv4, gnrc_pktsnip_t *pkt)
{
	/* Add UDP header */
	gnrc_pktsnip_t *udp = gnrc_pktbuf_add(pkt, NULL, sizeof(udp_hdr_t), GNRC_NETTYPE_UNDEF);

	ipv4_addr_t dst = ipv4->tunnel_addr;
	ipv4_addr_t src = ((ppp_protocol_t*) ipv4)->pppdev->l_ipcp.ip;

	udp_hdr_t *udp_hdr = (udp_hdr_t*) udp->data;
	udp_hdr->src_port = byteorder_htons(53209);
	udp_hdr->dst_port = byteorder_htons(ipv4->tunnel_port);
	udp_hdr->length = byteorder_htons(gnrc_pkt_len(udp));
	udp_hdr->checksum = byteorder_htons(0);
	
	udp_hdr->checksum = byteorder_htons(udp_checksum(src, dst, 17, byteorder_ntohs(udp_hdr->length), udp));
	return udp;
}

int handle_ipv4(struct ppp_protocol_t *protocol, uint8_t ppp_event, void *args)
{
	ipcp_t *ipcp = &protocol->pppdev->l_ipcp;
	(void) ipcp;

	gnrc_pppdev_t *pppdev = protocol->pppdev;
	gnrc_pktsnip_t *pkt;
	gnrc_pktsnip_t *recv_pkt = (gnrc_pktsnip_t*) args;
	(void) recv_pkt;

	switch(ppp_event)
	{
		case PPP_LINKUP:
			DEBUG("Msg: Obtained IP address! \n");
			DEBUG("Ip address is %i.%i.%i.%i\n",ipcp->ip.u8[0],ipcp->ip.u8[1],ipcp->ip.u8[2],ipcp->ip.u8[3]);	
			(void) pkt;
			break;
		case PPP_RECV:
			ppp_ipv4_recv(pppdev, recv_pkt);
			break;
		default:
			break;

	}
	return 0;
}


int ppp_ipv4_init(gnrc_pppdev_t *ppp_dev, ppp_ipv4_t *ipv4, ipcp_t *ipcp, gnrc_pppdev_t *pppdev)
{
	((ppp_protocol_t*) ipv4)->handler = &handle_ipv4;
	((ppp_protocol_t*) ipv4)->pppdev = ppp_dev;
	ipv4->tunnel_addr.u32 = byteorder_htonl(0);
	ipv4->tunnel_port = 0;
	return 0;
}

static gnrc_pktsnip_t *_encapsulate_pkt(ppp_ipv4_t *ipv4, gnrc_pktsnip_t *pkt)
{
	gnrc_pktsnip_t *sent_pkt;
	sent_pkt = _build_udp(ipv4, pkt);
	sent_pkt = gen_ip_pkt(ipv4, sent_pkt, 17);
	return sent_pkt;
}

int ppp_ipv4_send(gnrc_pppdev_t *ppp_dev, gnrc_pktsnip_t *pkt)
{
	int ipv4_ready = ((ppp_fsm_t*) &ppp_dev->l_ipcp)->state == S_OPENED;
	if(!ipv4_ready)
	{
		DEBUG("gnrc_ppp: IPCP down. Dropping packet.\n");
		gnrc_pktbuf_release(pkt);
		return -1;
	}

	/* Remove netif*/
	pkt = gnrc_pktbuf_remove_snip(pkt, pkt);

	gnrc_pktsnip_t *send_pkt = _encapsulate_pkt(&ppp_dev->l_ipv4, pkt);
	gnrc_ppp_send(ppp_dev, send_pkt);
	return 0;
}

int ppp_ipv4_recv(gnrc_pppdev_t *ppp_dev, gnrc_pktsnip_t *pkt)
{
	DEBUG("Received IP packet!!\n");
	gnrc_pktsnip_t *ipv4_hdr = gnrc_pktbuf_mark(pkt, sizeof(ipv4_hdr_t), GNRC_NETTYPE_UNDEF);
	gnrc_pktsnip_t *udp_hdr = gnrc_pktbuf_mark(pkt, sizeof(udp_hdr_t), GNRC_NETTYPE_UNDEF);

	gnrc_pktbuf_remove_snip(pkt, ipv4_hdr);
	gnrc_pktbuf_remove_snip(pkt, udp_hdr);

	/* create netif header */
	gnrc_pktsnip_t *netif_hdr;
	netif_hdr = gnrc_pktbuf_add(NULL, NULL,
			sizeof(gnrc_netif_hdr_t) + (2 * 6),
			GNRC_NETTYPE_NETIF);

	if (netif_hdr == NULL) {
		goto safe_out;
	}

	gnrc_netif_hdr_init(netif_hdr->data, 6, 6);
	((gnrc_netif_hdr_t *)netif_hdr->data)->if_pid = thread_getpid();

	LL_APPEND(pkt, netif_hdr);

	 /* throw away packet if no one is interested */
    if (!gnrc_netapi_dispatch_receive(GNRC_NETTYPE_IPV6, GNRC_NETREG_DEMUX_CTX_ALL, pkt)) {
        DEBUG("gnrc_netdev2: unable to forward packet of type %i\n", pkt->type);
        gnrc_pktbuf_release(pkt);
        return 0;
    }	

	return 0;
safe_out:
    gnrc_pktbuf_release(pkt);
    return 0;
}
