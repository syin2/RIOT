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

static int lcp_get_opt_status(ppp_option_t *opt, uint8_t suggested)
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
				if(suggested)
				{
					*(((uint8_t*) payload)) = (LCP_DEFAULT_MRU & 0xFF00)>>8;
					*(((uint8_t*) payload)+1) = LCP_DEFAULT_MRU & 0xFF;
				}
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


	/*TODO: Shouldn't be here*/
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

	if (event >= 0)
		trigger_event(lcp, event, pkt);
	return event;
}

int lcp_init(ppp_dev_t *ppp_dev, ppp_cp_t *lcp)
{
	cp_init(ppp_dev, lcp);

	lcp->num_opts = LCP_NUMOPTS;
	lcp->conf = ppp_dev->lcp_opts;
	lcp->conf[LCP_MRU].type = 1;
	lcp->conf[LCP_MRU].value[0] = 0xFF;
	lcp->conf[LCP_MRU].value[1] = 200;
	lcp->conf[LCP_MRU].size = 2;
	lcp->conf[LCP_MRU].flags = OPT_ENABLED;
	lcp->conf[LCP_MRU].next = NULL;



	lcp->prot = GNRC_NETTYPE_LCP;
	lcp->restart_timer = LCP_RESTART_TIMER;
	lcp->get_opt_status = &lcp_get_opt_status;
	lcp->handle_pkt = &lcp_handle_pkt;
	return 0;
}
