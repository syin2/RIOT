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

static int _lcp_get_opt_status(void *opt)
{
	uint8_t opt_type = ppp_opt_get_type(opt);
	int8_t payload = ppp_opt_get_payload(opt);
	int8_t length = ppp_opt_get_length(opt);

	uint16_t u16;
	/* For the moment, only MRU option supported */
	switch(opt_type)
	{
		case LCP_OPT_MRU:
			if(size != 4) /* TODO: Replace with label*/
				return -1; /* TODO: Replace with label*/
			u16 = ((*payload)<<8) + *(payload+1);
			if(u16 > LCP_MAX_MRU){
				return CP_CREQ_NAK;
			}
			return CP_CREQ_ACK;
			break;
		default:
			return CP_CREQ_REJ;
	}
	return 0;/*TODO: Fix right value */
}

static int _lcp_handle_rcr(ppp_cp_t *lcp, cp_pkt_t *pkt)
{
	uint8_t code = ppp_pkt_get_code(pkt);
	void *curr_opt;
	uint16_t curr_status;
	opt_list_t opts;
	ppp_opts_init(&opts, pkt);
	curr_opt = ppp_opts_get_head(&opts);
	for(int i=0; i<ppp_opts_get_num(&opts); i++)
	{
		curr_status = _lcp_get_opt_status(curr_opt);
		if(curr_status != CP_CREQ_ACK){
			cp->event = E_RCRm;
			return 0;
		}
		curr_opt = ppp_opts_next(&metadata->opts);
	}
	cp->event = E_RCRm;

	return 0; /*TODO: Fix output*/
}

static int _lcp_handle_rca(ppp_cp_t *lcp, cp_pkt_t *pkt)
{
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
static int _lcp_handle_nak(ppp_cp_t *lcp)
{
	lcp->event = E_RCN;
	return 0; /*TODO: Fix output*/
}

static int _lcp_handle_rej(ppp_cp_t *lcp)
{
	l_lcp->event = E_RCJ;
	return 0; /*TODO: Fix output*/
}


void lcp_handle_conf(cp_ppp_t *lcp, cp_pkt_t *pkt)
{
	switch(ppp_pkt_get_type(pkt))
	{
		case PPP_CONF_REQ:
			_lcp_handle_rcr(lcp, pkt);
			break;
		case PPP_CONF_ACK:
			_lcp_handle_rca(lcp, pkt);
			break;
		case PPP_CONF_NAK:
			_lcp_handle_rcn(lcp, pkt);
			break;
		case PPP_CONF_REJ:
			_lcp_handle_rej(lcp, pkt);
			break;
	}
}

void lcp_handle_code(cp_ppp_t *lcp, uint8_t code)
{
}

static void lcp_tlu(ppp_cp_t *lcp)
{
	lcp->l_upper_msg |= PPP_MSG_UP;
}

static void lcp_tld(ppp_cp_t *lcp)
{
	lcp->l_upper_msg |= PPP_MSG_DOWN;
}

static void lcp_tls(ppp_cp_t *lcp)
{
	lcp->l_lower_msg |= PPP_MSG_UP;
}

static void lcp_tlf(ppp_cp_t *lcp)
{
	lcp->l_lower_msg |= PPP_MSG_DOWN;
}

static void lcp_irc(ppp_cp_t *lcp)
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
static void lcp_zrc(ppp_cp_t *lcp)
{
	cp->restart_counter = 0;
	/* Set timer to appropiate value TODO*/
}

static void lcp_src(ppp_cp_t *lcp)
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
static void lcp_sca(ppp_cp_t *lcp, cp_pkt_t *pkt)
{
	pkt->hdr->code = PPP_CP_REQUEST_ACK;
	send_cp(cp, pkt);
}
static void lcp_scn(ppp_cp_t *lcp, cp_pkt_t *pkt)
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
static void lcp_str(ppp_cp_t *lcp)
{
	int id = 666; /*TODO*/
	cp_pkt_t pkt;
	pkt->hdr->code = PPP_CP_TERM_REQUEST;
	pkt->hdr->id = id;
	pkt->hdr->length = 4;
	pkt->opts->num_opts = 0;
	send_cp(cp, pkt);
}
static void lcp_sta(ppp_cp_t *lcp)
{
	int id = 666; /*TODO*/
	cp_pkt_t pkt;
	pkt->hdr->code = PPP_CP_TERM_ACK;
	pkt->hdr->id = id;
	pkt->hdr->length = 4;
	pkt->opts->num_opts = 0;
	send_cp(cp, pkt);
}
static void lcp_scj(ppp_cp_t *lcp)
{
	send_cp(cp, PPP_CP_CODE_REJ);
}
static void lcp_ser(ppp_cp_t *lcp)
{
	send_cp(cp,PPP_CP_SER);
}

static int _handle_term_req(ppp_cp_t *lcp, cp_pkt_t *pkt)
{
	cp->tr_identifier = cp_pkt->hdr->id;
	cp->event = E_RTR;
}

static int _handle_term_ack(ppp_cp_t *lcp, cp_pkt_t *pkt)
{
	cp->event = E_RTA;
}

static int _handle_code_rej(ppp_cp_t *lcp, gnrc_pktsnip_t *pkt)
{
	cp->event = E_RXJm;
}

