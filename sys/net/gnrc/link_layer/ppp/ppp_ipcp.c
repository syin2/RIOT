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
#include "net/gnrc/pkt.h"
#include "net/gnrc/pktbuf.h"
#include "net/gnrc/nettype.h"
#include <errno.h>

#define ENABLE_DEBUG    (1)
#include "debug.h"

#if ENABLE_DEBUG
/* For PRIu16 etc. */
#include <inttypes.h>
#endif


static cp_conf_t *ipcp_get_conf_by_code(ppp_cp_t *cp, uint8_t code)
{
	switch(code)
	{
		default:
			return NULL;
	}
}
static int ipcp_handle_pkt(ppp_cp_t *ipcp, ppp_hdr_t *hdr, gnrc_pktsnip_t *pkt)
{
	int type = ppp_hdr_get_code(hdr);
	int event;
	
	switch(type){
		case PPP_CONF_REQ:
			event = handle_rcr(ipcp, pkt);
			break;
		case PPP_CONF_ACK:
			event = handle_rca(ipcp, pkt);
			break;
		case PPP_CONF_NAK:
			event = handle_rcn_nak(ipcp, pkt);
			break;
		case PPP_CONF_REJ:
			event = handle_rcn_rej(ipcp, pkt);
			break;
		case PPP_TERM_REQ:
			event = E_RTR;
			break;
		case PPP_TERM_ACK:
			event = handle_term_ack(ipcp, pkt);
			break;
		case PPP_CODE_REJ:
			event = handle_coderej(pkt);
			break;
		default:
			event = E_RUC;
			break;
	}

	return event;
}

uint8_t ipcp_ipaddress_is_valid(ppp_option_t *opt)
{
	return true;
}

void ipcp_ipaddress_handle_nak(struct cp_conf_t *conf, ppp_option_t *opt)
{
		conf->flags &= ~OPT_ENABLED;
}

uint8_t ipcp_ipaddress_build_nak_opts(uint8_t *buf)
{
	return 0;
}

int ipcp_init(gnrc_pppdev_t *ppp_dev, ppp_cp_t *ipcp)
{
	cp_init(ppp_dev, ipcp);

	ipcp->conf = ppp_dev->ipcp_opts;
	ipcp->conf[IPCP_IPADDRESS].type = 4;
	ipcp->conf[IPCP_IPADDRESS].value = byteorder_htonl(0x0A000002);
	ipcp->conf[IPCP_IPADDRESS].size = 4;
	ipcp->conf[IPCP_IPADDRESS].flags = OPT_ENABLED;
	ipcp->conf[IPCP_IPADDRESS].next = NULL;
	ipcp->conf[IPCP_IPADDRESS].is_valid = &ipcp_ipaddress_is_valid;
	ipcp->conf[IPCP_IPADDRESS].handle_nak = &ipcp_ipaddress_handle_nak;
	ipcp->conf[IPCP_IPADDRESS].build_nak_opts = &ipcp_ipaddress_build_nak_opts;

	ipcp->id = ID_IPCP;
	ipcp->prottype = GNRC_NETTYPE_IPCP;
	ipcp->restart_timer = IPCP_RESTART_TIMER;
	ipcp->handle_pkt = &ipcp_handle_pkt;
	ipcp->get_conf_by_code = &ipcp_get_conf_by_code;
	return 0;
}
