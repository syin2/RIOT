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

static cp_conf_t *lcp_get_conf_by_code(ppp_cp_t *cp, uint8_t code)
{
	switch(code)
	{
		case LCP_OPT_MRU:
			return &cp->conf[LCP_MRU];
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

int lcp_init(gnrc_pppdev_t *ppp_dev, ppp_cp_t *lcp)
{
	cp_init(ppp_dev, lcp);

	lcp->conf = ppp_dev->lcp_opts;
	lcp->conf[LCP_MRU].type = 1;
	lcp->conf[LCP_MRU].value = byteorder_htonl(3500);
	lcp->conf[LCP_MRU].size = 2;
	lcp->conf[LCP_MRU].flags = OPT_ENABLED;
	lcp->conf[LCP_MRU].next = NULL;
	lcp->conf[LCP_MRU].is_valid = &lcp_mru_is_valid;
	lcp->conf[LCP_MRU].handle_nak = &lcp_mru_handle_nak;
	lcp->conf[LCP_MRU].build_nak_opts = &lcp_mru_build_nak_opts;


	lcp->supported_codes = FLAG_CONF_REQ | FLAG_CONF_ACK | FLAG_CONF_NAK | FLAG_CONF_REJ | FLAG_TERM_REQ | FLAG_TERM_ACK | FLAG_CODE_REJ | FLAG_ECHO_REQ | FLAG_ECHO_REP | FLAG_DISC_REQ;
	lcp->id = ID_LCP;
	lcp->prottype = GNRC_NETTYPE_LCP;
	lcp->restart_timer = LCP_RESTART_TIMER;
	lcp->get_conf_by_code = &lcp_get_conf_by_code;
	return 0;
}
