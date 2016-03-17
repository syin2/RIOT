
/*
 * Copyright (C) 2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 * @ingroup     net_ppp_cp
 * @file
 * @brief       Implementation of the of Generic Control Protocol for PPP
 *
 * @author      José Ignacio Alamos <jialamos@uc.cl>
 * @}
 */

#include <errno.h>
#include <string.h>

#include "msg.h"
#include "thread.h"
#include "net/gnrc.h"
#include "net/ppptype.h"
#include "net/gnrc/ppp/ppp.h"
#include "net/gnrc/ppp/lcp.h"
#include "net/hdlc/hdr.h"

#define ENABLE_DEBUG    (1)
#include "debug.h"

#if ENABLE_DEBUG
/* For PRIu16 etc. */
#include <inttypes.h>
#endif



static int _handle_cp_rcr(ppp_cp_t *l_lcp, cp_pkt_t *pkt)
{
	if (cp->metadata->opts_status_content  & (OPT_HAS_NAK | OPT_HAS_REJ))
	{
		l_lcp->event = E_RCRm;
	}
	else
	{
		l_lcp->event = E_RCRp;
	}

return 0; /*TODO: Fix output*/
}
#if 0
static int _handle_cp_rca(ppp_cp_t *cp, cp_pkt_t *pkt)
{
	populate_opt_metadata(cp);

	/* Identifier should match */
	if (pkt->id != cp->cr_sent_identifier)
	{
		return -1; /* TODO: Fix error code*/
	}

	/* Sent options and ACK options should match. */
	opt_stack_t opt_stack;
	cp->optpkt_to_optstack(cp->cp_options, &opt_stack);

	if (!_opt_stacks_are_equal(opt_stack, pkt->opts))
	{
		return -1; /* TODO: Error code*/
	}

	cp->event = E_RCA;
}

/* Fix params for request */
static int _handle_cp_nak(ppp_cp_t *cp, cp_pkt *pkt)
{
	cp->negotiate_nak(cp->cp_options, pkt->opts);
	l_lcp->event = E_RCN;
	return 0; /*TODO: Fix output*/
}

static int _handle_cp_rej(ppp_cp_t *cp, cp_pkt_t *pkt)
{
	l_lcp->event = E_RCJ;
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
#endif
void handle_cp_pkt(ppp_cp_t *cp, cp_pkt_t *pkt)
{
	cp->metadata.pkt = pkt;
	ppp_pkt_gen_metadata(cp->metadata, pkt, cp->get_opt_status);

	int type = ppp_pkt_get_code(pkt);
	
	switch(type){
		case PPP_CONF_REQ:
			_handle_cp_rcr(cp, pkt);
			break;
		case PPP_CONF_ACK:
			break;
		case PPP_CONF_NAK:
			break;
		case PPP_CONF_REJ:
			break;
		case PPP_TERM_REQ:
			break;
		case PPP_TERM_ACK:
			break;
		case PPP_CODE_REJ:
			break;
		case PPP_PROT_REJ:
			break;
		case PPP_ECHO_REQ:
			break;
		case PPP_ECHO_REP:
			break;
		case PPP_DISC_REQ:
			break;
		case PPP_IDENT:
			break;
		case PPP_TIME_REM:
			break;
	}
}
