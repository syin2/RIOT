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
#include "net/gnrc/ppp/cp.h"
#include "net/ppp/opt.h"

#define ENABLE_DEBUG    (0)
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

static int _lcp_handle_cp_rcr(ppp_cp_t *lcp)
{
	if (lcp->metadata.opts_status_content  & (OPT_HAS_NAK | OPT_HAS_REJ))
	{
		cp->event = E_RCRm;
	}
	else
	{
		cp->event = E_RCRp;
	}

return 0; /*TODO: Fix output*/
}
static int _lcp_handle_cp_rca(ppp_cp_t *lcp)
{
	cp_pkt_t *pkt = lcp->metadata.pkt;
	/* Identifier should match */
	uint8_t pkt_id = ppp_pkt_get_id(pkt);
	uint8_t pkt_length = ppp_pkt_get_length(pkt);

	if (pkt_id != lcp->cr_sent_identifier)
	{
		return -1; /* TODO: Fix error code*/
	}

	if (lcp->cr_sent_size != pkt_length || memcmp(lcp->cr_sent_opts,pkt->payload,pkt_length-sizeof(cp_hdr_t)))
	{
		return -1; /* TODO: Error code*/
	}

	lcp->event = E_RCA;
return 0; /*TODO: Fix output*/
}

/* Fix params for request */
static int _lcp_handle_cp_nak(ppp_cp_t *lcp)
{
	lcp->negotiate_nak(lcp->cp_options, &lcp->metadata);
	lcp->event = E_RCN;
	return 0; /*TODO: Fix output*/
}

static int _lcp_handle_cp_rej(ppp_cp_t *lcp)
{
	/* Turn off every option that is not allowed by peer */
	opt_metadata_t *opt_handler = lcp->metadata.opts;
	void *curr_opt = ppp_opts_get_head(opt_handler);
	int num_opts = ppp_opts_get_num(opt_handler);

	int curr_type;

	for(int i=0;i<num_opts;i++)
	{
		curr_type = ppp_opt_get_tye(curr_opt);

	}
	l_lcp->event = E_RCJ;
}


void lcp_handle_conf(cp_ppp_t *lcp, uint8_t type)
{
	
}
void lcp_handle_code(cp_ppp_t *lcp, uint8_t code)
{
}

static void _tlu(ppp_cp_t *cp)
{
	cp->l_upper_msg |= PPP_MSG_UP;
}

static void _tld(ppp_cp_t *cp)
{
	cp->l_upper_msg |= PPP_MSG_DOWN;
}

static void _tls(ppp_cp_t *cp)
{
	cp->l_lower_msg |= PPP_MSG_UP;
}

static void _tlf(ppp_cp_t *cp)
{
	cp->l_lower_msg |= PPP_MSG_DOWN;
}

static void _irc(ppp_cp_t *cp)
{
	/*Depending on the state, set the right value for restart counter*/
	/* TODO: Activate restart timer with corresponding time out */
	switch(cp->timer_select)
	{
		case RC_SEL_CONF:
			cp->counter_term = PPP_MAX_TERMINATE;
			break;
		case RC_SEL_TERM:
			cp->counter_conf = PPP_MAX_CONFIG;
			break;
		default:
			/* Shouldn't be here */
			break;
	}
}
static void _zrc(ppp_cp_t *cp)
{
	cp->restart_counter = 0;
	/* Set timer to appropiate value TODO*/
}

static void _src(ppp_cp_t *cp)
{
	/* Decrement configure counter */
	cp->counter_conf -= 1;

	int id=666; /* TODO */
	cp_pkt_t pkt;
	pkt.hdr.code = PPP_CP_REQUEST_CONFIGURE;
	pkt.hdr.id = id;
	cp->populate_opt_stack(cp->cp_options, &(pkt.opt_stack));
	
	send_cp(cp, &pkt);
	/* TODO: Set timeout for SRC */
}
static void _sca(ppp_cp_t *cp, cp_pkt_t *pkt)
{
	pkt->hdr->code = PPP_CP_REQUEST_ACK;
	send_cp(cp, pkt);
}
static void _scn(ppp_cp_t *cp, cp_pkt_t *pkt)
{
	/* Check the content of received options */
	if(pkt->opts->content_flag & OPT_HAS_REJ)
	{
		_remove_opts_by_status(PPP_CP_REQUEST_REJ, pkt->opts);
		pkt->hdr->code = PPP_CP_REQUEST_REJ;
		send_cp(cp, pkt);
	}
	else
	{
		_remove_opts_by_status(PPP_CP_REQUEST_NAK, pkt->opts);
		pkt->hdr->code = PPP_CP_REQUEST_NAK;
		send_cp(cp, pkt);
	}
}
static void _str(ppp_cp_t *cp)
{
	int id = 666; /*TODO*/
	cp_pkt_t pkt;
	pkt->hdr->code = PPP_CP_TERM_REQUEST;
	pkt->hdr->id = id;
	pkt->hdr->length = 4;
	pkt->opts->num_opts = 0;
	send_cp(cp, pkt);
}
static void _sta(ppp_cp_t *cp)
{
	int id = 666; /*TODO*/
	cp_pkt_t pkt;
	pkt->hdr->code = PPP_CP_TERM_ACK;
	pkt->hdr->id = id;
	pkt->hdr->length = 4;
	pkt->opts->num_opts = 0;
	send_cp(cp, pkt);
}
static void _scj(ppp_cp_t *cp)
{
	send_cp(cp, PPP_CP_CODE_REJ);
}
static void _ser(ppp_cp_t *cp)
{
	send_cp(cp,PPP_CP_SER);
}

static int _handle_cp_term_req(ppp_cp_t *cp, cp_pkt_t *pkt)
{
	cp->tr_identifier = cp_pkt->hdr->id;
	cp->event = E_RTR;
}

static int _handle_cp_term_ack(ppp_cp_t *cp, cp_pkt_t *pkt)
{
	cp->event = E_RTA;
}

static int _handle_cp_code_rej(ppp_cp_t *cp, gnrc_pktsnip_t *pkt)
{
	cp->event = E_RXJm;
}

