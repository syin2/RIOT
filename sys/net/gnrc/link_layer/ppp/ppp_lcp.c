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

#include "net/ppp/lcp.h"


static int send_cp(ppp_dev_t *dev, uint8_t code, uint8_t *payload, size_t p_size)
{
	/* Get real size of payload */
	uint16_t payload_size = PPP_CP_HDR_BASE_SIZE+p_size;

	/* Set code, identifier, length*/
	dev->payload_buf[0] = code;
	/*Identifier. For now, a random number*/
	int id = 666;
	dev->payload_buf[1] = id;
	dev->paylload_buf[2] = 0;
	dev->payload_buf[3] = p_size;

	/*Copy payload to payload_buf. DON'T FORGET TO CHECK SIZES!!*/
	memcpy(&dev->payload_buf[PPP_CP_HDR_BASE_SIZE], payload, p_size);

	/* Create pkt snip */
	gnrc_pktsnip_t *pkt = gnrc_pktbuf_add(NULL, &dev->payload_buf[0], p_size, GNRC_NETTYPE_UNDEF);,
	if (pkt == NULL){
		DEBUG("PPP: not enough space in pkt buffer");
		return;
	}
	
	/* Send pkt to ppp_send*/
	ppp_send(dev, pkt);

}

static int _gen_lcp_ctrl_payload(ppp_ctrl_prot_t *l_lcp, uint8_t *payload)
{
	/* Get LCP options struct */
	lcp_opt_t lcp_opts = (lcp_opt_t*) l_lcp->opts;

	/* Get the options available to send  */
	int opt_flags = lcp_opts->flags;

	int size = 0;

	/* options still not implemented */
	/* Don't send options*/
	payload = NULL;
	return size;
}

static void _tlu(ppp_dev_t *dev)
{
	dev->l_upper_msg |= PPP_MSG_UP;
}

static void _tld(ppp_dev_t *dev)
{
	dev->l_upper_msg |= PPP_MSG_DOWN;
}

static void _tls(ppp_dev_t *dev)
{
	dev->l_lower_msg |= PPP_MSG_UP;
}

static void _tlf(ppp_dev_t *dev)
{
	dev->l_lower_msg |= PPP_MSG_DOWN;
}

static void _irc(ppp_dev_t *dev)
{
	/*Depending on the state, set the right value for restart counter*/
}
static void _zrc(ppp_ctrl_prot_t *l_lcp)
{
	l_lcp->restart_counter = 0;
	/* Set timer to appropiate value */
}


static void _src(ppp_dev_t *dev)
{
	uint8_t *opts;
	/* Size of payload (options) */
	int size;
	size = _gen_ctrl_payload(dev, opts);

	/* Send control protocol*/
	pkt = send_cp(PPP_CP_REQUEST_CONFIGURE, opts, size)
}

static void _sca(ppp_dev_t *dev)
{
	
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

static int lcp_fsm(ppp_ctrl_prot *l_lcp)
{
	uint8_t next_state;


	/* Call the appropiate functions for each state */
	_event_action(l_lcp);
	/* Set next state */
	next_state = state_trans[l_lcp->state][l_lcp->event]

	/* Keep in same state if there's something wrong (RFC 1661) */
	if(next_state != S_UNDEF){
		l_lcp->state = next_state;
	}
}
