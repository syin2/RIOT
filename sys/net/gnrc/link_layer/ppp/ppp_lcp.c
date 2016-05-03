/*
 * Copyright (C) 2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 * @ingroup     ppp_lcp
 * @file
 * @brief       Implementation of PPP's LCP protocol
 *
 * @author      José Ignacio Alamos <jialamos@uc.cl>
 * @}
 */

#include "net/gnrc/ppp/lcp.h"
#include "net/gnrc/ppp/ppp.h"
#include <inttypes.h>
#include "net/gnrc/ppp/opt.h"
#include "net/gnrc/pkt.h"
#include "net/ppp/hdr.h"
#include "net/gnrc/pktbuf.h"
#include "net/gnrc/nettype.h"
#include <errno.h>
#include "net/gnrc/ppp/fsm.h"

#define ENABLE_DEBUG    (1)
#include "debug.h"

#if ENABLE_DEBUG
/* For PRIu16 etc. */
#include <inttypes.h>
#endif

static cp_conf_t *lcp_get_conf_by_code(ppp_fsm_t *cp, uint8_t code)
{
	switch(code)
	{
		case LCP_OPT_MRU:
			return &cp->conf[LCP_MRU];
		case LCP_OPT_ACCM:
			return &cp->conf[LCP_ACCM];
		default:
			return NULL;
	}
}

uint8_t lcp_mru_is_valid(ppp_option_t *opt)
{
	uint8_t *payload = ppp_opt_get_payload(opt);
	uint16_t u16 = ((*payload)<<8) + *(payload+1);
	if(u16 > LCP_MAX_MRU){
		return false;
	}
	return true;
}

uint8_t lcp_mru_build_nak_opts(uint8_t *buf)
{
	uint8_t len = 4;
	ppp_option_t *opt = (ppp_option_t*) buf;
	uint8_t *payload = ppp_opt_get_payload(opt);
	if(opt)
	{
		ppp_opt_set_type(opt, 1);	
		ppp_opt_set_length(opt, len);
		*payload = (LCP_DEFAULT_MRU & 0xFF00) >> 8;
		*(payload+1) = LCP_DEFAULT_MRU & 0xFF;
	}
	return len;
}


void lcp_mru_set(ppp_fsm_t *lcp, ppp_option_t *opt, uint8_t peer)
{
	DEBUG("Called LCP MRU set, but still not implemented!\n");
	if(peer)
	{
		((lcp_t*) lcp)->peer_mru = byteorder_ntohs(*((network_uint16_t*) ppp_opt_get_payload(opt)));
	}
	else
	{
		((lcp_t*) lcp)->mru = byteorder_ntohs(*((network_uint16_t*) ppp_opt_get_payload(opt)));
	}
}


uint8_t lcp_accm_is_valid(ppp_option_t *opt)
{
	/*Flags are always valid*/
	return true;
}

uint8_t lcp_accm_build_nak_opts(uint8_t *buf)
{
	/* Never called */
	return true;
}

void lcp_accm_set(ppp_fsm_t *lcp, ppp_option_t *opt, uint8_t peer)
{
	DEBUG("Setting ACCM\n");
	if(peer)
		lcp->dev->netdev->driver->set(lcp->dev->netdev, PPPOPT_ACCM_RX, (void*) ppp_opt_get_payload(opt), 4);
	else
		lcp->dev->netdev->driver->set(lcp->dev->netdev, PPPOPT_ACCM_TX, (void*) ppp_opt_get_payload(opt), 4);
}

static void lcp_config_init(ppp_fsm_t *lcp)
{
	lcp->conf = LCP_NUMOPTS ? ((lcp_t*) lcp)->lcp_opts : NULL;

	lcp->conf[LCP_MRU].type = LCP_OPT_MRU;
	lcp->conf[LCP_MRU].value = byteorder_htonl(3500);
	lcp->conf[LCP_MRU].size = 2;
	lcp->conf[LCP_MRU].flags = OPT_ENABLED;
	lcp->conf[LCP_MRU].next = &lcp->conf[LCP_ACCM];
	lcp->conf[LCP_MRU].is_valid = &lcp_mru_is_valid;
	lcp->conf[LCP_MRU].build_nak_opts = &lcp_mru_build_nak_opts;
	lcp->conf[LCP_MRU].set = &lcp_mru_set;

	lcp->conf[LCP_ACCM].type = LCP_OPT_ACCM;
	lcp->conf[LCP_ACCM].value = byteorder_htonl(0xFFFFFFFF);
	lcp->conf[LCP_ACCM].size = 4;
	lcp->conf[LCP_ACCM].flags = 0;
	lcp->conf[LCP_ACCM].next = NULL;
	lcp->conf[LCP_ACCM].is_valid = &lcp_accm_is_valid;
	lcp->conf[LCP_ACCM].build_nak_opts = &lcp_accm_build_nak_opts;
	lcp->conf[LCP_ACCM].set = &lcp_accm_set;
}

int lcp_init(gnrc_pppdev_t *ppp_dev, ppp_fsm_t *lcp)
{
	cp_init(ppp_dev, lcp);
	lcp_config_init(lcp);

	lcp->supported_codes = FLAG_CONF_REQ | FLAG_CONF_ACK | FLAG_CONF_NAK | FLAG_CONF_REJ | FLAG_TERM_REQ | FLAG_TERM_ACK | FLAG_CODE_REJ | FLAG_ECHO_REQ | FLAG_ECHO_REP | FLAG_DISC_REQ;
	((ppp_protocol_t*)lcp)->id = ID_LCP;
	((lcp_t*)lcp)->pr_id = 0;
	lcp->prottype = GNRC_NETTYPE_LCP;
	lcp->restart_timer = LCP_RESTART_TIMER;
	lcp->get_conf_by_code = &lcp_get_conf_by_code;
	lcp->prot.handler = &fsm_handle_ppp_msg;
	lcp->targets = ((ID_PPPDEV & 0xffff) << 8) | (BROADCAST_NCP & 0xffff);
	((lcp_t*) lcp)->mru = 1500;
	((lcp_t*) lcp)->peer_mru = 1500;
	return 0;
}
