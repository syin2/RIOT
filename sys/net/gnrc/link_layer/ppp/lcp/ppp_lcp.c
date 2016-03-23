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
#include "net/ppp/pkt.h"
#include <errno.h>

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

static int _lcp_handle_rcr(cp_pkt_t *pkt)
{
	void *curr_opt;
	uint16_t curr_status;
	opt_list_t opts;
	ppp_opts_init(&opts, pkt);
	curr_opt = ppp_opts_get_head(&opts);
	for(int i=0; i<ppp_opts_get_num(&opts); i++)
	{
		curr_status = _lcp_get_opt_status(curr_opt);
		if(curr_status != CP_CREQ_ACK){
			return E_RCRm;
		}
		curr_opt = ppp_opts_next(&opts);
	}
	return E_RCRp;
}

static int _lcp_handle_rca(cp_pkt_t *pkt, ppp_cp_t *lcp)
{
	uint8_t pkt_id = ppp_pkt_get_id(pkt);
	uint8_t pkt_length = ppp_pkt_get_length(pkt);

	if (pkt_id != lcp->cr_sent_identifier)
	{
		return -EBADMSG; /* TODO: Fix error code*/
	}

	if (lcp->cr_sent_size != pkt_length || memcmp(lcp->cr_sent_opts,pkt->payload,pkt_length-sizeof(cp_hdr_t)))
	{
		return -EBADMSG; /* TODO: Error code*/
	}

	return E_RCA;
}


void lcp_tlu(ppp_cp_t *lcp)
{
	lcp->l_upper_msg |= PPP_MSG_UP;
}

void lcp_tld(ppp_cp_t *lcp)
{
	lcp->l_upper_msg |= PPP_MSG_DOWN;
}

void lcp_tls(ppp_cp_t *lcp)
{
	lcp->l_lower_msg |= PPP_MSG_UP;
}

void lcp_tlf(ppp_cp_t *lcp)
{
	lcp->l_lower_msg |= PPP_MSG_DOWN;
}

void lcp_irc(ppp_cp_t *lcp)
{
	/*Depending on the state, set the right value for restart counter*/
	/* TODO: Activate restart timer with corresponding time out */
	switch(lcp->timer_select)
	{
		case RC_SEL_CONF:
			lcp->counter_term = PPP_MAX_TERMINATE;
			break;
		case RC_SEL_TERM:
			lcp->counter_config = PPP_MAX_CONFIG;
			break;
		default:
			/* Shouldn't be here */
			break;
	}
}
void lcp_zrc(ppp_cp_t *lcp)
{
	lcp->restart_counter = 0;
	/* Set timer to appropiate value TODO*/
}

void lcp_src(ppp_cp_t *lcp)
{
	/* Decrement configure counter */
	lcp->counter_config -= 1;

	int id=666; /* TODO */
	cp_pkt_t pkt;
	ppp_pkt_set_code(&pkt, PPP_CONF_REQ);
	ppp_pkt_set_id(&pkt, id);
	
	//send_cp(cp, &pkt);
	/* TODO: Set timeout for SRC */
}
void lcp_sca(ppp_cp_t *lcp, cp_pkt_t *pkt)
{
	ppp_pkt_set_code(pkt, PPP_CONF_ACK);
	(void) lcp;
	//send_cp(cp, pkt);
}
void lcp_scn(ppp_cp_t *lcp, cp_pkt_t *pkt)
{
	(void) lcp;
	(void) pkt;
#if 0
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
#endif
}
void lcp_str(ppp_cp_t *lcp)
{
	(void) lcp;
#if 0
	int id = 666; /*TODO*/
	cp_pkt_t pkt;
	pkt->hdr->code = PPP_CP_TERM_REQUEST;
	pkt->hdr->id = id;
	pkt->hdr->length = 4;
	pkt->opts->num_opts = 0;
	send_cp(cp, pkt);
#endif
}
void lcp_sta(ppp_cp_t *lcp, cp_pkt_t *pkt)
{
	(void) lcp;
	(void) pkt;
#if 0
	int id = 666; /*TODO*/
	cp_pkt_t pkt;
	pkt->hdr->code = PPP_CP_TERM_ACK;
	pkt->hdr->id = id;
	pkt->hdr->length = 4;
	pkt->opts->num_opts = 0;
	send_cp(cp, pkt);
#endif
}
void lcp_scj(ppp_cp_t *lcp, cp_pkt_t *pkt)
{
	(void) lcp;
	(void) pkt;
	//send_cp(lcp, PPP_CP_CODE_REJ);
}
void lcp_ser(ppp_cp_t *lcp, cp_pkt_t *pkt)
{
	(void) lcp;
	(void) pkt;
	//send_cp(lcp,PPP_CP_SER);
}

int lcp_handle_conf(ppp_cp_t *lcp, cp_pkt_t *pkt)
{
	int result;
	switch(ppp_pkt_get_code(pkt))
	{
		case PPP_CONF_REQ:
			result = _lcp_handle_rcr(pkt);
			break;
		case PPP_CONF_ACK:
			result = _lcp_handle_rca(pkt, lcp);
			break;
		case PPP_CONF_NAK:
		case PPP_CONF_REJ:
			result = E_RCN;
			break;
	}
	return result;
}

int lcp_handle_code(cp_pkt_t *pkt)
{
	/* Generate ppp packet from payload */
	cp_pkt_t rej_pkt;
	ppp_pkt_init(ppp_pkt_get_payload(pkt), ppp_pkt_get_length(pkt), &rej_pkt);
	uint8_t code = ppp_pkt_get_code(&rej_pkt);
	if (code >= PPP_CONF_REQ && code <= PPP_TERM_ACK)
	{
		return E_RXJp;
	}
	else
	{
		return E_RXJm;
	}
}

int lcp_handle_echo(ppp_cp_t *lcp, cp_pkt_t *pkt)
{
	(void) lcp;
	(void) pkt;
	return 0;
}

int lcp_handle_unknown_code(ppp_cp_t *lcp, cp_pkt_t *pkt)
{
	(void) lcp;
	(void) pkt;
	return 0;
}
int lcp_handle_term(ppp_cp_t *lcp, cp_pkt_t *pkt)
{
	(void) lcp;
	(void) pkt;
	uint8_t code = ppp_pkt_get_code(pkt);
	switch(code)
	{
		case PPP_TERM_REQ:
			return E_RTR;
			break;
		case PPP_TERM_ACK:
			/*TODO: Compare incoming ID */
			return E_RTA;
	}

	return -EBADMSG;
}

int lcp_handle_pkt(ppp_cp_t *lcp, cp_pkt_t *pkt)
{
	/* Check options recv are subset of opts sent */
	/* Check pkt sanity */

	int type = ppp_pkt_get_code(pkt);
	int event;
	
	switch(type){
		case PPP_CONF_REQ:
		case PPP_CONF_ACK:
		case PPP_CONF_NAK:
		case PPP_CONF_REJ:
			event = lcp_handle_conf(lcp, pkt);
			break;
		case PPP_TERM_REQ:
		case PPP_TERM_ACK:
			event = lcp_handle_term(lcp, pkt);
			break;
		case PPP_CODE_REJ:
			event = lcp_handle_code(pkt);
			break;
		case PPP_ECHO_REQ:
		case PPP_ECHO_REP:
			event = lcp_handle_echo(lcp, pkt);
			break;
		default:
			event = lcp_handle_unknown_code(lcp, pkt);
			break;
	}

	return event;
}

