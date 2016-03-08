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


static int lcp_recv(ppp_dev_t *dev, gnrc_pktsnip_t *pkt)
{
	/* Mark LCP header*/
	gnrc_pktsnip_t *lcp_hdr = gnrc_pktbuf_mark(pkt, sizeof(ppp_ctrl_hdr_t), GNRC_NETTYPE_UNDEF);
	gnrc_pktsnip_t *opt_hdr = NULL;;

	
	if (!lcp_hdr) {
		DEBUG("gnrc_ppp: no space left in packet buffer\n");
		return;	
	}

	/* Handle packet depeding on state */	
	switch(ppp_dev->state){
		case LCPSTATE_DEAD:
			/* Discards packets here */
			goto safe_out;
			break;
		case LCPSTATE_ESTABLISHED:
			/* Discard packet if it's not configure ACK or if it's not expected*/
			if(lcp_hdr->type != PPP_CONF_ACK || dev->lcp_opt.identifier != lcp_hdr->identifier){
				goto safe_out;
			}
			/* Handle configuration ack */
			_handle_conf_ack(dev, pkt);
			break;
		case LCPSTATE_AUTH:
			break;
		case LCPSTATE_NCP:
			break;
		case LCPSTATE_OPEN:
			break;
		case LCPSTATE_TERMINATION:
			break;
		default:
			/* Should not reach here! */
			//Assert?
			break;
safe_out:

}
static int _retr_peer_neg(gnrc_pktsnip_t *pkt)
{
	
}
static int _handle_conf_ack(ppt_dev_t *dev, gnrc_pktsnip_t *pkt)
{
	/* Retrieve peer negotiation */
	lcp_conf_t *opts = _retr_peer_neg(pkt);
	
	/* Shouldn't be necessary... but check if the response has the right negotiation params */
	if (opts->opts == dev->lcp_conf->opts)
	{
		/* Auth still not implemented. Go to NCP state...*/
		/* Change state to LCP's NCP */
		dev->state = LCPSTATE_NCP;

		/* Start negotiation of choosen NCP protocol*/
		ppp_ncp_start(dev)
	}
	else
	{
	}
	return 0;
}
