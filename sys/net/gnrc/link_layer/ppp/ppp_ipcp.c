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
}

uint8_t ipcp_ipaddress_build_nak_opts(uint8_t *buf)
{
	return 0;
}
void ipcp_ipaddress_set(ppp_fsm_t *ipcp, ppp_option_t *opt, uint8_t peer)
{
	DEBUG("Please implement ipaddress_set from IPCP\n");
	if(peer)
		((ipcp_t*) ipcp)->ip = *((ipv4_addr_t*) ppp_opt_get_payload(opt));
	else
		((ipcp_t*) ipcp)->local_ip = *((ipv4_addr_t*) ppp_opt_get_payload(opt));
}

int ipcp_init(gnrc_pppdev_t *ppp_dev, ppp_fsm_t *ipcp)
{
	cp_init(ppp_dev, ipcp);

	ipcp->conf = ((ipcp_t*) ipcp)->ipcp_opts;
	ipcp->conf[IPCP_IPADDRESS].type = 3;
	ipcp->conf[IPCP_IPADDRESS].value = byteorder_htonl(0);
	ipcp->conf[IPCP_IPADDRESS].size = 4;
	ipcp->conf[IPCP_IPADDRESS].flags = OPT_ENABLED;
	ipcp->conf[IPCP_IPADDRESS].next = NULL;
	ipcp->conf[IPCP_IPADDRESS].is_valid = &ipcp_ipaddress_is_valid;
	ipcp->conf[IPCP_IPADDRESS].handle_nak = &ipcp_ipaddress_handle_nak;
	ipcp->conf[IPCP_IPADDRESS].build_nak_opts = &ipcp_ipaddress_build_nak_opts;
	ipcp->conf[IPCP_IPADDRESS].set = &ipcp_ipaddress_set;

	ipcp->supported_codes = FLAG_CONF_REQ | FLAG_CONF_ACK | FLAG_CONF_NAK | FLAG_CONF_REJ | FLAG_TERM_REQ | FLAG_TERM_ACK | FLAG_CODE_REJ;
	((ppp_protocol_t*) ipcp)->id = ID_IPCP;
	ipcp->prottype = GNRC_NETTYPE_IPCP;
	ipcp->restart_timer = IPCP_RESTART_TIMER;
	ipcp->get_conf_by_code = &ipcp_get_conf_by_code;
	ipcp->prot.handler = &fsm_handle_ppp_msg;
	return 0;
}
