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

void lcp_mru_handle_nak(struct cp_conf_t *conf, ppp_option_t *opt)
{
	uint8_t *payload = ppp_opt_get_payload(opt);
	uint16_t suggested_u16 = ((*payload)<<8) + *(payload+1);
	if(suggested_u16 <= LCP_MAX_MRU)
		conf->value = byteorder_htonl(suggested_u16);
	else
		conf->flags &= ~OPT_ENABLED;
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
}


uint8_t lcp_accm_is_valid(ppp_option_t *opt)
{
	/*Flags are always valid*/
	return true;
}

void lcp_accm_handle_nak(struct cp_conf_t *conf, ppp_option_t *opt)
{
	network_uint32_t *suggested_u32 = (network_uint32_t*) ppp_opt_get_payload(opt);
	conf->value = *suggested_u32;
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

int lcp_init(gnrc_pppdev_t *ppp_dev, ppp_fsm_t *lcp)
{
	cp_init(ppp_dev, lcp);

	lcp->conf = ppp_dev->lcp_opts;


	lcp->conf[LCP_MRU].type = LCP_OPT_MRU;
	lcp->conf[LCP_MRU].value = byteorder_htonl(3500);
	lcp->conf[LCP_MRU].size = 2;
	lcp->conf[LCP_MRU].flags = OPT_ENABLED;
	lcp->conf[LCP_MRU].next = &lcp->conf[LCP_ACCM];
	lcp->conf[LCP_MRU].is_valid = &lcp_mru_is_valid;
	lcp->conf[LCP_MRU].handle_nak = &lcp_mru_handle_nak;
	lcp->conf[LCP_MRU].build_nak_opts = &lcp_mru_build_nak_opts;
	lcp->conf[LCP_MRU].set = &lcp_mru_set;

	lcp->conf[LCP_ACCM].type = LCP_OPT_ACCM;
	lcp->conf[LCP_ACCM].value = byteorder_htonl(0xFFFFFFFF);
	lcp->conf[LCP_ACCM].size = 4;
	lcp->conf[LCP_ACCM].flags = 0;
	lcp->conf[LCP_ACCM].next = NULL;
	lcp->conf[LCP_ACCM].is_valid = &lcp_accm_is_valid;
	lcp->conf[LCP_ACCM].handle_nak = &lcp_accm_handle_nak;
	lcp->conf[LCP_ACCM].build_nak_opts = &lcp_accm_build_nak_opts;
	lcp->conf[LCP_ACCM].set = &lcp_accm_set;


	lcp->supported_codes = FLAG_CONF_REQ | FLAG_CONF_ACK | FLAG_CONF_NAK | FLAG_CONF_REJ | FLAG_TERM_REQ | FLAG_TERM_ACK | FLAG_CODE_REJ | FLAG_ECHO_REQ | FLAG_ECHO_REP | FLAG_DISC_REQ;
	((ppp_protocol_t*)lcp)->id = ID_LCP;
	lcp->prottype = GNRC_NETTYPE_LCP;
	lcp->restart_timer = LCP_RESTART_TIMER;
	lcp->get_conf_by_code = &lcp_get_conf_by_code;
	lcp->prot.handler = &fsm_handle_ppp_msg;
	return 0;
}
