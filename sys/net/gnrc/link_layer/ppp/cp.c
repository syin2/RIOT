
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

static int _parse_cp_options(opt_stack_t *o_stack, uint8_t *payload, size_t p_size)
{
	o_stack->num_opts = 0;

	/*Start iterating over options */
	uint16_t cursor = 0;
	
	uint8_t curr_type, curr_len;
	uint8_t curr_status;

	/* Current option status*/
	cp_opt_t curr_opt_status;

	cp_opt_t *last_opt= NULL;

	o_stack->content_flag=0;

	/* TODO: Check default value (no opts sent)*/
	while(cursor < p_size) {
		/* Read current option type */
		curr_type = *(payload+cursor);
		curr_len = *(payload+cursor+1);
		
		/* TODO: If cursor + len > total_length, discard pkt*/

		curr_opt_status.next = NULL;
		_read_lcp_pkt(curr_type, payload+cursor+2, (size_t) curr_len, &curr_opt_status);
		if(last_opt != NULL)
		{
			last_opt->next = &curr_opt_status;
		}
		curr_status =curr_opt_status.status;

		DEBUG("Current status: %i\n",curr_status);

		o_stack->content_flag |= 1<<curr_status;

		o_stack->_opt_buf[o_stack->num_opts] = curr_opt_status;
		o_stack->num_opts+=1;
		cursor = cursor + curr_len;
		last_opt = &curr_opt_status;
	}

	return 0; /*TODO: Check return*/
}

static int _handle_cp_rcr(ppp_cp_t *l_lcp, cp_pkt_t *pkt)
{
	/*Set the remote identifier*/
	cp->cr_recv_identifier = pkt->hdr->id;

	/* At this point, we have the responses and type of responses. Process each one*/
	if (pkt->opts->content_flag  & (OPT_HAS_NAK | OPT_HAS_REJ))
	{
		l_lcp->event = E_RCRm;
	}
	else
	{
		l_lcp->event = E_RCRp;
	}

return 0; /*TODO: Fix output*/
}
static int _opts_are_equal(cp_opt_t *o1, cp_opt_t *o2)
{
	if (o1->type != o2->type || o1->status != o2->status || o1->p_size != o2->p_size || memcmp(o1->payload,
	o2->payload,o1->p_size)){
		return false;
	}
	return true;
}
static int _opt_stacks_are_equal(opt_stack_t *o1, opt_stack_t *o2)
{
	uint8_t len_1 = o1->num_opts;
	uint8_t len_2= o2->num_opts;

	if (len_1 != len_2)
	{
		return false;
	}
	for(int i=0;i<len_1;i++)
	{
		if(!_opt_are_equal(o1->_opt_buf[i], o2->_opt_buf[i]))
		{
			return false;
		}
	}
	return true;
}
static int _handle_cp_rca(ppp_cp_t *cp, cp_pkt_t *pkt)
{
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
