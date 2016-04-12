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
#include <inttypes.h>
#include "net/gnrc/ppp/cp.h"
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

#if 0
/* Negotiate local LCP options with NAK opts sent by peer */
void lcp_negotiate_nak(void *lcp_opt, cp_pkt_metadata_t *metadata)
{
	/* Cast lcp_opt to corresponding struct */
	lcp_opt_t *opts = (lcp_opt_t*) lcp_opt;

	void *curr_opt;
	uint8_t ctype;
	uint16_t suggested_value;
	uint8_t *payload;

	opt_metadata_t *opts_handler = &metadata->opts;

	curr_opt = ppp_opts_get_head(opts_handler);
	int num_opts = ppp_opts_get_num(opts_handler);
	/* Iterate through every pkt option */
	for(int i=0;i<num_opts;i++)
	{
		ctype = ppp_opt_get_type(curr_opt);
		payload = (uint8_t*) ppp_opt_get_payload(curr_opt);
		switch(ctype)
		{
			case LCP_OPT_MRU:
				suggested_value = ((*payload)<<8) + *(payload+1);

				if(suggested_value > LCP_MAX_MRU){
					opts->mru = LCP_MAX_MRU;
				}
				else
				{
					opts->mru = suggested_value;
				}
				break;
			default:
				break;
		}
		curr_opt = ppp_opts_next(opts_handler);
	}
}
#endif
static int lcp_get_opt_status(ppp_option_t *opt)
{
	uint8_t opt_type = ppp_opt_get_type(opt);
	uint8_t * payload = (uint8_t*) ppp_opt_get_payload(opt);
	uint8_t length = ppp_opt_get_length(opt);

	uint16_t u16;
	/* For the moment, only MRU option supported */
	switch(opt_type)
	{
		case LCP_OPT_MRU:
			if(length != 4) 
				return -EBADMSG; 
			u16 = ((*payload)<<8) + *(payload+1);
			if(u16 > LCP_MAX_MRU){
				return CP_CREQ_NAK;
			}
			return CP_CREQ_ACK;
			break;
		default:
			return CP_CREQ_REJ;
	}
	return -EBADMSG; /* Never reaches here. Something went wrong if that's the case */
}

static int lcp_handle_pkt(ppp_cp_t *lcp, gnrc_pktsnip_t *pkt)
{
	gnrc_pktsnip_t *hdr = gnrc_pktbuf_mark(pkt, sizeof(ppp_hdr_t), GNRC_NETTYPE_LCP);
	ppp_hdr_t *ppp_hdr = (ppp_hdr_t*) hdr->data;


	int type = ppp_hdr_get_code(ppp_hdr);
	int event;
	
	switch(type){
		case PPP_CONF_REQ:
			event = handle_rcr(lcp, pkt);
			break;
		case PPP_CONF_ACK:
			event = handle_rca(lcp, pkt);
			break;
		case PPP_CONF_NAK:
			event = handle_rcn_nak(lcp, pkt);
			break;
		case PPP_CONF_REJ:
			event = handle_rcn_rej(lcp, pkt);
			break;
		case PPP_TERM_REQ:
			event = E_RTR;
			break;
		case PPP_TERM_ACK:
			event = handle_term_ack(lcp, pkt);
			break;
		case PPP_CODE_REJ:
			event = handle_coderej(pkt);
			break;
		case PPP_ECHO_REQ:
		case PPP_ECHO_REP:
		case PPP_DISC_REQ:
			event = E_RXR;
			break;
		default:
			event = E_RUC;
			break;
	}

	trigger_event(lcp, event, pkt);
	return event;
}

int lcp_init(ppp_dev_t *ppp_dev, ppp_cp_t *lcp)
{
	cp_init(ppp_dev, lcp);
	lcp->prot = GNRC_NETTYPE_LCP;
	lcp->restart_timer = LCP_RESTART_TIMER;
	lcp->get_opt_status = &lcp_get_opt_status;
	lcp->handle_pkt = &lcp_handle_pkt;
	lcp->conf = &ppp_dev->lcp_opts;
	
	return 0;
}
