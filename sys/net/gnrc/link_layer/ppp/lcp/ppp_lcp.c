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

#define ENABLE_DEBUG    (0)
#include "debug.h"

#if ENABLE_DEBUG
/* For PRIu16 etc. */
#include <inttypes.h>
#endif

/*Send Control Protocol. Assumes the opt payload is loaded in HDLC Control Protocol Buffer. */
static int send_cp(ppp_ctrl_prot_t  *cp, cp_pkt_t *pkt)
{
	ppp_dev_t *dev = cp->dev;

	/* Set code, identifier, length*/
	dev->_payload_buf[0] = code;
	dev->_payload_buf[1] = identifier;
	uint32_t length;
	/* if size is not zero, the hdlc cp buffer was preloaded */
	/*TODO: Change number to labels*/
	uint16_t cursor;
	/* Generate payload with corresponding options */
	cursor = 0;
	cp_opt_t *copt;
	for(int i=0;i<l_lcp->_num_opt;i++)
	{
		copt = &(opt_stack->_opt_buf[i]);
		*(dst+cursor) = copt->type;
		*(dst+cursor+1) = copt->p_size+2;
		for(int j=0;j<copt->opt_size;j++)
		{
			*(dst+cursor+2+i) = copt->payload[j];
		}
		cursor += copt->opt_size+2;
	}
	length =  opt_size+4;
	dev->_payload_buf[2] = length & 0xFF00;
	dev->_payload_buf[3] = length & 0x00FF;

	/* Create pkt snip */
	gnrc_pktsnip_t *pkt = gnrc_pktbuf_add(NULL, dev->_payload_buf, length, GNRC_NETTYPE_UNDEF);
	if (pkt == NULL){
		DEBUG("PPP: not enough space in pkt buffer");
		return 0; /*TODO Fix*/
	}
	
	/* Send pkt to ppp_send*/
	ppp_send(dev, pkt);
	return 0; /*TODO*/
}

static void _tlu(ppp_ctrl_prot_t *l_lcp)
{
	l_lcp->l_upper_msg |= PPP_MSG_UP;
}

static void _tld(ppp_ctrl_prot_t *l_lcp)
{
	l_lcp->l_upper_msg |= PPP_MSG_DOWN;
}

static void _tls(ppp_ctrl_prot_t *l_lcp)
{
	l_lcp->l_lower_msg |= PPP_MSG_UP;
}

static void _tlf(ppp_ctrl_prot_t *l_lcp)
{
	l_lcp->l_lower_msg |= PPP_MSG_DOWN;
}

static void _irc(ppp_ctrl_prot_t *l_lcp)
{
	/*Depending on the state, set the right value for restart counter*/
	/* TODO: Activate restart timer with corresponding time out */
	switch(l_lcp->timer_select)
	{
		case RC_SEL_CONF:
			l_lcp->counter_term = PPP_MAX_TERMINATE;
			break;
		case RC_SEL_TERM:
			l_lcp->counter_conf = PPP_MAX_CONFIG;
			break;
		default:
			/* Shouldn't be here */
			break;
	}
}
static void _zrc(ppp_ctrl_prot_t *l_lcp)
{
	l_lcp->restart_counter = 0;
	/* Set timer to appropiate value TODO*/
}

uint8_t *_get_cpopt_pointer(cp)
{
	return cp->dev->_hdlc_cp_buf+4; /* TODO: Fix offset*/
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
static void _sca(ppp_ctrl_prot_t *l_lcp, cp_pkt_t *pkt)
{
	pkt->hdr->code = PPP_CP_REQUEST_ACK;
	send_cp(l_lcp, pkt);
}
static void _remove_opts_by_status(uint8_t status, opt_stack_t opt_stack)
{
	cp_opt_t *copt;
	copt = opt_stack->opts;

	cp_opt_t *last_linked_opt=NULL;

	while(copt->next != NULL)
	{
		if (copt->status == status)
		{
			last_linked_opt->next = copt;
			last_linked_opt = copt;
		}
	}
}
static void _scn(ppp_ctrl_prot_t *l_lcp, cp_pkt_t *pkt)
{
	/* Check the content of received options */
	if(pkt->opts->content_flag & OPT_HAS_REJ)
	{
		_remove_opts_by_status(PPP_CP_REQUEST_REJ, pkt->opts);
		pkt->hdr->code = PPP_CP_REQUEST_REJ;
		send_cp(l_lcp, pkt);
	}
	else
	{
		_remove_opts_by_status(PPP_CP_REQUEST_NAK, pkt->opts);
		pkt->hdr->code = PPP_CP_REQUEST_NAK;
		send_cp(l_lcp, pkt);
	}
}
static void _str(ppp_ctrl_prot_t *l_lcp)
{
	int id = 666; /*TODO*/
	send_cp(l_lcp, PPP_CP_TERM_REQUEST, id);
}
static void _sta(ppp_ctrl_prot_t *l_lcp)
{
	send_cp(l_lcp, PPP_CP_TERM_ACK);
}
static void _scj(ppp_ctrl_prot_t *l_lcp)
{
	send_cp(l_lcp, PPP_CP_CODE_REJ);
}
static void _ser(ppp_ctrl_prot_t *l_lcp)
{
	send_cp(l_lcp,PPP_CP_SER);
}


/* Call functions depending on function flag*/
static void _event_action(ppp_ctrl_prot_t *l_lcp) 
{
	uint8_t flags;
	/* Reset link status */
	l_lcp->l_upper_msg = 0;
	l_lcp->l_lower_msg = 0;

	flags = actions[l_lcp->state][l_lcp->event];

	if(flags & F_TLU) _tlu(l_lcp);
	if(flags & F_TLD) _tld(l_lcp);
	if(flags & F_TLS) _tls(l_lcp);
	if(flags & F_TLF) _tlf(l_lcp);
	if(flags & F_IRC) _irc(l_lcp);
	if(flags & F_ZRC) _zrc(l_lcp);
	if(flags & F_SRC) _src(l_lcp);
	if(flags & F_SCA) _sca(l_lcp);
	if(flags & F_SCN) _scn(l_lcp);
	if(flags & F_STR) _str(l_lcp);
	if(flags & F_STA) _sta(l_lcp);
	if(flags & F_SCJ) _scj(l_lcp);
	if(flags & F_SER) _ser(l_lcp);
}

static int lcp_fsm(ppp_ctrl_prot_t *l_lcp)
{
	int8_t next_state;


	/* Call the appropiate functions for each state */
	_event_action(l_lcp);
	/* Set next state */
	next_state = state_trans[l_lcp->state][l_lcp->event];

	/* Keep in same state if there's something wrong (RFC 1661) */
	if(next_state != S_UNDEF){
		l_lcp->state = next_state;
	}
	return 0; /*TODO*/
}
