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
#include "net/gnrc/ppp/opt.h"
#include "net/gnrc/pkt.h"
#include "net/ppp/hdr.h"
#include "net/gnrc/pktbuf.h"
#include "net/gnrc/nettype.h"
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

static int _pkt_get_ppp_header(gnrc_pktsnip_t *pkt, ppp_hdr_t **ppp_hdr)
{
	if(pkt->type == GNRC_NETTYPE_LCP)
	{
		*ppp_hdr = (ppp_hdr_t*) pkt->data;
		return false;
	}
	else
	{
		*ppp_hdr = (ppp_hdr_t*) pkt->next->data;
		return true;
	}
}
static int _lcp_get_opt_status(ppp_option_t *opt)
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

static int _lcp_handle_rcr(gnrc_pktsnip_t *pkt)
{
	
	ppp_hdr_t *ppp_hdr;
	int has_options = _pkt_get_ppp_header(pkt, &ppp_hdr);

	if(!has_options)
	{
		/* If the packet doesn't have options, it's considered as valid. */
		return E_RCRp;
	}

	uint8_t pkt_length = ppp_hdr_get_length(ppp_hdr);
	uint8_t opt_reminder = pkt_length-sizeof(ppp_hdr_t);

	/* Check if options are valid */

	if (!ppp_conf_opts_valid(pkt, opt_reminder))
	{
		return -EBADMSG;
	}


	ppp_option_t *curr_opt = (ppp_option_t*) pkt->data;
	uint8_t opt_length;


	while(opt_reminder)
	{
		opt_length = ppp_opt_get_length(curr_opt);

		if(_lcp_get_opt_status(curr_opt )!= CP_CREQ_ACK){
			return E_RCRm;
		}
		
		curr_opt = (void*)((uint8_t*)curr_opt+opt_length);
		opt_reminder-=opt_length;
	}

	return E_RCRp;
}

static int _lcp_handle_rca(ppp_cp_t *lcp, gnrc_pktsnip_t *pkt)
{
	
	ppp_hdr_t *ppp_hdr;
	_pkt_get_ppp_header(pkt, &ppp_hdr);

	uint8_t pkt_id = ppp_hdr_get_id(ppp_hdr);
	uint8_t pkt_length = ppp_hdr_get_length(ppp_hdr);

	if (pkt_id != lcp->cr_sent_identifier || memcmp(lcp->cr_sent_opts,pkt->data,pkt_length-sizeof(ppp_hdr_t)))
	{
		return -EBADMSG;
	}
	return E_RCA;
}

static int _lcp_handle_rcn_nak(ppp_cp_t *lcp, gnrc_pktsnip_t *pkt)
{

	ppp_hdr_t *ppp_hdr;
	_pkt_get_ppp_header(pkt, &ppp_hdr);

	if (ppp_hdr_get_id(ppp_hdr) != lcp->cr_sent_identifier)
		return -EBADMSG;
	return E_RCN;
}

static int _lcp_handle_rcn_rej(ppp_cp_t *lcp, gnrc_pktsnip_t *pkt)
{
	ppp_hdr_t *ppp_hdr;
	int has_options = _pkt_get_ppp_header(pkt, &ppp_hdr);

	if(!has_options)
		return -EBADMSG;

	if (ppp_hdr_get_id(ppp_hdr) != lcp->cr_sent_identifier)
		return -EBADMSG;

	/* Check if opts are subset of sent options */
	ppp_option_t *curr_opt = pkt->data;

	uint16_t size = lcp->cr_sent_size;
	if(ppp_hdr_get_length(ppp_hdr)-sizeof(ppp_hdr_t) > lcp->cr_sent_size)
		return -EBADMSG;

	uint8_t cursor=0;
	while(cursor<size)
	{
		if(!ppp_opt_is_subset(curr_opt, lcp->cr_sent_opts, size))
			return -EBADMSG;
		cursor+=ppp_opt_get_length(curr_opt);
		curr_opt = (void*) ((uint8_t*) curr_opt + (int) ppp_opt_get_length(curr_opt));
	}
	return E_RCN;
}

#if 0
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

	int id=lcp->cr_sent_identifier++;

	ppp_pkt_set_code(&pkt, PPP_CONF_REQ);
	ppp_pkt_set_id(&pkt, id);

	opt_list_t opts;
	ppp_opts_init();
	
	//send_cp(cp, &pkt);
	/* TODO: Set timeout for SRC */
}

void lcp_sca(ppp_cp_t *lcp, gnrc_pktsnip_t *pkt)
{
	//ppp_hdr_t *ppp_hdr = (ppp_hdr_t*) pkt->data;
	//ppp_pkt_get_code(ppp_hdr, PPP_CONF_ACK);
	(void) lcp;
	(void) pkt;
	//send_cp(cp, pkt);
}
void lcp_scn(ppp_cp_t *lcp, gnrc_pktsnip_t *pkt)
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
	gnrc_pktsnip_t pkt;
	pkt->hdr->code = PPP_CP_TERM_REQUEST;
	pkt->hdr->id = id;
	pkt->hdr->length = 4;
	pkt->opts->num_opts = 0;
	send_cp(cp, pkt);
#endif
}

void lcp_sta(ppp_cp_t *lcp, gnrc_pktsnip_t *pkt)
{
	(void) lcp;
	(void) pkt;
#if 0
	int id = 666; /*TODO*/
	gnrc_pktsnip_t pkt;
	pkt->hdr->code = PPP_CP_TERM_ACK;
	pkt->hdr->id = id;
	pkt->hdr->length = 4;
	pkt->opts->num_opts = 0;
	send_cp(cp, pkt);
#endif
}
void lcp_scj(ppp_cp_t *lcp, gnrc_pktsnip_t *pkt)
{
	(void) lcp;
	(void) pkt;
	//send_cp(lcp, PPP_CP_CODE_REJ);
}
void lcp_ser(ppp_cp_t *lcp, gnrc_pktsnip_t *pkt)
{
	(void) lcp;
	(void) pkt;
	//send_cp(lcp,PPP_CP_SER);
}
#endif

int _lcp_handle_coderej(gnrc_pktsnip_t *pkt)
{
	/* Generate ppp packet from payload */
	/* Mark ppp headr */

	gnrc_pktbuf_mark(pkt, sizeof(ppp_hdr_t), GNRC_NETTYPE_UNDEF);
	ppp_hdr_t *rej_hdr = (ppp_hdr_t*) pkt->data;

	uint8_t code = ppp_hdr_get_code(rej_hdr);
	if (code >= PPP_CONF_REQ && code <= PPP_TERM_ACK)
	{
		return E_RXJm;
	}
	else
	{
		return E_RXJp;
	}
}

int _lcp_handle_term_ack(ppp_cp_t *lcp, gnrc_pktsnip_t *pkt)
{
	ppp_hdr_t *ppp_hdr;
	_pkt_get_ppp_header(pkt, &ppp_hdr);
	
	int id = ppp_hdr_get_id(ppp_hdr);
	if(id == lcp->tr_sent_identifier)
		return E_RTA;
	return -EBADMSG;
}

int lcp_handle_pkt(ppp_cp_t *lcp, gnrc_pktsnip_t *pkt)
{
	gnrc_pktsnip_t *hdr = gnrc_pktbuf_mark(pkt, sizeof(ppp_hdr_t), GNRC_NETTYPE_LCP);
	ppp_hdr_t *ppp_hdr = (ppp_hdr_t*) hdr->data;


	int type = ppp_hdr_get_code(ppp_hdr);
	int event;
	
	switch(type){
		case PPP_CONF_REQ:
			event = _lcp_handle_rcr(pkt);
			break;
		case PPP_CONF_ACK:
			event = _lcp_handle_rca(lcp, pkt);
			break;
		case PPP_CONF_NAK:
			event = _lcp_handle_rcn_nak(lcp, pkt);
			break;
		case PPP_CONF_REJ:
			event = _lcp_handle_rcn_rej(lcp, pkt);
			break;
		case PPP_TERM_REQ:
			event = E_RTR;
			break;
		case PPP_TERM_ACK:
			event = _lcp_handle_term_ack(lcp, pkt);
			break;
		case PPP_CODE_REJ:
			event = _lcp_handle_coderej(pkt);
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

	return event;
}

