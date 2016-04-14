/*
 * Copyright (C) 2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 * @ingroup     ppp_ipcp
 * @file
 * @brief       Implementation of PPP's IPCP protocol
 *
 * @author      José Ignacio Alamos <jialamos@uc.cl>
 * @}
 */

#include "net/gnrc/ppp/ipcp.h"
#include "net/gnrc/ppp/ppp.h"

#include "net/ppp/hdr.h"

#include "net/gnrc/ppp/opt.h"
#include "net/gnrc/pkt.h"
#include "net/gnrc/pktbuf.h"
#include "net/gnrc/nettype.h"
#include <errno.h>

#define ENABLE_DEBUG    (1)
#include "debug.h"

#if ENABLE_DEBUG
/* For PRIu16 etc. */
#include <inttypes.h>
#endif

#if 0
static int ipcp_get_opt_status(ppp_option_t *opt, uint8_t suggested)
{
	(void) suggested;
	uint8_t opt_type = ppp_opt_get_type(opt);

	/* For the moment, only MRU option supported */
	switch(opt_type)
	{
		default:
			return CP_CREQ_ACK;
	}
	return -EBADMSG; /* Never reaches here. Something went wrong if that's the case */
}
#endif

static int ipcp_handle_pkt(ppp_cp_t *ipcp, gnrc_pktsnip_t *pkt)
{
	gnrc_pktsnip_t *hdr = gnrc_pktbuf_mark(pkt, sizeof(ppp_hdr_t), GNRC_NETTYPE_IPCP);
	ppp_hdr_t *ppp_hdr = (ppp_hdr_t*) hdr->data;


	int type = ppp_hdr_get_code(ppp_hdr);
	int event;
	
	switch(type){
		case PPP_CONF_REQ:
			event = handle_rcr(ipcp, pkt);
			break;
		case PPP_CONF_ACK:
			event = handle_rca(ipcp, pkt);
			break;
		case PPP_CONF_NAK:
			event = handle_rcn_nak(ipcp, pkt);
			break;
		case PPP_CONF_REJ:
			event = handle_rcn_rej(ipcp, pkt);
			break;
		case PPP_TERM_REQ:
			event = E_RTR;
			break;
		case PPP_TERM_ACK:
			event = handle_term_ack(ipcp, pkt);
			break;
		case PPP_CODE_REJ:
			event = handle_coderej(pkt);
			break;
		default:
			event = E_RUC;
			break;
	}

	trigger_event(ipcp, event, pkt);
	return event;
}

int ipcp_init(ppp_dev_t *ppp_dev, ppp_cp_t *ipcp)
{
	cp_init(ppp_dev, ipcp);

	//ipcp->num_opts = IPCP_NUMOPTS;
	//ipcp->conf = &ppp_dev->l_ipcp;

	ipcp->id = ID_IPCP;
	ipcp->prottype = GNRC_NETTYPE_IPCP;
	ipcp->restart_timer = IPCP_RESTART_TIMER;
	//ipcp->get_opt_status = &ipcp_get_opt_status;
	ipcp->handle_pkt = &ipcp_handle_pkt;
	return 0;
}
