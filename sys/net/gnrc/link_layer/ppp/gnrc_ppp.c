/*
 * Copyright (C) 2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 * @ingroup     net_ppp
 * @file
 * @brief       Implementation of the PPP protocol
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
#include "net/ppp/hdr.h"
#include <errno.h>
#include <string.h>

#define ENABLE_DEBUG    (1)
#include "debug.h"

#if ENABLE_DEBUG
/* For PRIu16 etc. */
#include <inttypes.h>
#endif


static void print_state(int state)
{
	switch(state)
	{
		case S_UNDEF:
			DEBUG("UNDEF");
			break;
		case S_INITIAL:
			DEBUG("INITIAL");
			break;
		case S_STARTING:
			DEBUG("STARTING");
			break;
		case S_CLOSED:
			DEBUG("CLOSED");
			break;
		case S_STOPPED:
			DEBUG("STOPPED");
			break;
		case S_CLOSING:
			DEBUG("CLOSING");
			break;
		case S_STOPPING:
			DEBUG("STOPPING");
			break;
		case S_REQ_SENT:
			DEBUG("REQ_SENT");
			break;
		case S_ACK_RCVD:
			DEBUG("ACK_RECV");
			break;
		case S_ACK_SENT:
			DEBUG("ACK_SENT");
			break;
		case S_OPENED:
			DEBUG("OPENED");
			break;
	}
}
static void print_event(uint8_t event)
{
	switch(event)
	{
		case E_UP:
			DEBUG("UP");
			break;
		case E_DOWN:
			DEBUG("DOWN");
			break;
		case E_OPEN:
			DEBUG("OPEN");
			break;
		case E_CLOSE:
			DEBUG("CLOSE");
			break;
		case E_TOp:
			DEBUG("TO+");
			break;
		case E_TOm:
			DEBUG("TO-");
			break;
		case E_RCRp:
			DEBUG("RCR+");
			break;
		case E_RCRm:
			DEBUG("RCR-");
			break;
		case E_RCA:
			DEBUG("RCA");
			break;
		case E_RCN:
			DEBUG("RCN");
			break;
		case E_RTR:
			DEBUG("RTR");
			break;
		case E_RTA:
			DEBUG("RTA");
			break;
		case E_RUC:
			DEBUG("RUC");
			break;
		case E_RXJp:
			DEBUG("RXJ+");
			break;
		case E_RXJm:
			DEBUG("RXJ-");
			break;
		case E_RXR:
			DEBUG("RXR");
			break;
	}
}
static void print_transition(int state, uint8_t event, int next_state)
{
	DEBUG("From state ");
	print_state(state);
	DEBUG(" with event ");
	print_event(event);
	DEBUG(". Next state is ");
	print_state(next_state);
	DEBUG("\n");
}
/* Call functions depending on function flag*/
static void _event_action(ppp_cp_t *cp, uint8_t event, gnrc_pktsnip_t *pkt) 
{
	int flags;

	flags = actions[event][cp->state];

	if(flags & F_TLU) tlu(cp, NULL);
	if(flags & F_TLD) tld(cp, NULL);
	if(flags & F_TLS) tls(cp, NULL);
	if(flags & F_TLF) tlf(cp, NULL);
	if(flags & F_IRC) irc(cp, (void*) &flags);
	if(flags & F_ZRC) zrc(cp, NULL);
	if(flags & F_SCR) scr(cp, NULL);
	if(flags & F_SCA) sca(cp, (void*) pkt);
	if(flags & F_SCN) scn(cp, (void*) pkt);
	if(flags & F_STR) str(cp, NULL);
	if(flags & F_STA) sta(cp, (void*) pkt);
	if(flags & F_SCJ) scj(cp, (void*) pkt);
	if(flags & F_SER) ser(cp, (void*) pkt);
}

int trigger_event(ppp_cp_t *cp, uint8_t event, gnrc_pktsnip_t *pkt)
{
	if (event < 0)
	{
		return -EBADMSG;
	}
	int8_t next_state;
	next_state = state_trans[event][cp->state];
	print_transition(cp->state, event, next_state);
	_event_action(cp, event, pkt);
	/* Keep in same state if there's something wrong (RFC 1661) */
	if(next_state != S_UNDEF){
		cp->state = next_state;
	}
	/*Check if next state doesn't have a running timer*/
	if (cp->state < S_CLOSING || cp->state == S_OPENED)
		xtimer_remove(&cp->xtimer);
	return 0;
}
static int _pkt_get_ppp_header(gnrc_pktsnip_t *pkt, ppp_hdr_t **ppp_hdr)
{
	if(pkt->type == GNRC_NETTYPE_UNDEF)
	{
		*ppp_hdr = (ppp_hdr_t*) pkt->next->data;
		return true;
	}
	else
	{
		*ppp_hdr = (ppp_hdr_t*) pkt->data;
		return false;
	}
}


void tlu(ppp_cp_t *cp, void *args)
{
	DEBUG("%i", cp->prot);
	DEBUG("> This layer up (a.k.a Successfully negotiated Link)\n");
	(void) cp;
	//cp->l_upper_msg |= PPP_MSG_UP;
}

void tld(ppp_cp_t *cp, void *args)
{
	DEBUG("%i", cp->prot);
	DEBUG("> This layer down\n");
	(void) cp;
	//cp->l_upper_msg |= PPP_MSG_DOWN;
}

void tls(ppp_cp_t *cp, void *args)
{
	DEBUG("%i", cp->prot);
	DEBUG(">  This layer started\n");
	(void) cp;
	//cp->l_lower_msg |= PPP_MSG_UP;
}

void tlf(ppp_cp_t *cp, void *args)
{
	DEBUG("%i", cp->prot);
	DEBUG(">  This layer finished\n");
	(void) cp;
	//cp->l_lower_msg |= PPP_MSG_DOWN;
}

void irc(ppp_cp_t *cp, void *args)
{
	DEBUG("%i", cp->prot);
	DEBUG(">  Init Restart Counter\n");
	uint8_t cr = *((int*) args) & F_SCR; 

	if(cr)
	{
		cp->restart_counter = PPP_MAX_CONFIG;
	}
	else
	{
		cp->restart_counter = PPP_MAX_TERMINATE;
	}
}
void zrc(ppp_cp_t *cp, void *args)
{
	DEBUG("%i", cp->prot);
	DEBUG(">  Zero restart counter\n ");
	(void) cp;
	//cp->restart_counter = 0;
	/* Set timer to appropiate value TODO*/
}

gnrc_pktsnip_t *build_options(ppp_cp_t *cp)
{
	size_t size=0;
	for(int i=0;i<cp->num_opts;i++)
	{
		if(cp->conf[i].flags & OPT_ENABLED)
		{
			size += conf[i].size;
		}
	}

	gnrc_pktsnip_t *opts = gnrc_pktbuf_add(NULL, NULL, size, GNRC_NETTYPE_UNDEF);

	int cursor=0;
	for(int i=0;i<cp->num_opts;i++)
	{
		if(cp->conf[i].flags & OPT_ENABLED)
		{
			memcpy(opts->data+cursor, conf[i].value, conf[i].size);	
			cursor+=conf[i].size;
		}
	}
	return opts;
}


void scr(ppp_cp_t *cp, void *args)
{
	DEBUG("%i", cp->prot);
	DEBUG(">  Sending Configure Request\n");
	/* Decrement configure counter */
	cp->restart_counter -= 1;

	/*TODO: Add options*/

	gnrc_pktsnip_t *opts = build_options(cp);
	gnrc_pktsnip_t *pkt = pkt_build(cp->prot, PPP_CONF_REQ, cp->cr_sent_identifier,opts);
	
	/*Send packet*/
	gnrc_ppp_send(cp->dev->netdev, pkt);

	cp->msg.type = NETDEV2_MSG_TYPE_EVENT;
	cp->msg.content.value = 0x0100 +PPP_TIMEOUT;
	xtimer_set_msg(&cp->xtimer, cp->restart_timer, &cp->msg, thread_getpid());
}

void sca(ppp_cp_t *cp, void *args)
{
	gnrc_pktsnip_t *pkt = (gnrc_pktsnip_t*) args;
	DEBUG("%i", cp->prot);
	DEBUG(">  Sending Configure Ack\n");
	ppp_hdr_t *recv_ppp_hdr;

	gnrc_pktsnip_t *opts = NULL;
	int has_options = _pkt_get_ppp_header(pkt, &recv_ppp_hdr);

	if(has_options)
	{
		DEBUG(">> Received pkt asked for options. Send them back, with ACK pkt\n");
		opts = pkt->data;
	}
	else
	{
		DEBUG(">> Received pkt didn't ask for options -> So just ACK\n");
	}

	gnrc_pktsnip_t *send_pkt = pkt_build(cp->prot, PPP_CONF_ACK, ppp_hdr_get_id(recv_ppp_hdr),opts);
	
	/*Send packet*/
	gnrc_ppp_send(cp->dev->netdev, send_pkt);
}

void scn(ppp_cp_t *cp, void *args)
{
	gnrc_pktsnip_t *pkt = (gnrc_pktsnip_t*) args;
	DEBUG("%i", cp->prot);
	DEBUG(">  Sending Configure Nak/Rej\n");

	/* Get the size of the pkt */
	uint8_t rej=0;

}

void str(ppp_cp_t *cp, void *args)
{
	DEBUG("%i", cp->prot);
	DEBUG(">  Sending Terminate Request\n");
	(void) cp;
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

void sta(ppp_cp_t *cp, void *args)
{ 
	gnrc_pktsnip_t *pkt = (gnrc_pktsnip_t*) args;
	DEBUG("%i", cp->prot);
	DEBUG(">  Sending Terminate Ack\n");
	(void) cp;
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
void scj(ppp_cp_t *cp, void *args)
{
	gnrc_pktsnip_t *pkt = (gnrc_pktsnip_t*) args;
	DEBUG("%i", cp->prot);
	DEBUG(">  Sending Code Rej\n");
	(void) cp;
	(void) pkt;
	//send_cp(cp, PPP_CP_CODE_REJ);
}
void ser(ppp_cp_t *cp, void *args)
{
	gnrc_pktsnip_t *pkt = (gnrc_pktsnip_t*) args;
	DEBUG("%i", cp->prot);
	DEBUG(">  Sending Echo/Discard/Replay\n");
	(void) cp;
	(void) pkt;
	//send_cp(cp,PPP_CP_SER);
}

int handle_rcr(ppp_cp_t *cp, gnrc_pktsnip_t *pkt)
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

		if(cp->get_opt_status(curr_opt )!= CP_CREQ_ACK){
			return E_RCRm;
		}
		
		curr_opt = (void*)((uint8_t*)curr_opt+opt_length);
		opt_reminder-=opt_length;
	}

	return E_RCRp;
}

int handle_rca(ppp_cp_t *cp, gnrc_pktsnip_t *pkt)
{
	
	ppp_hdr_t *ppp_hdr;
	_pkt_get_ppp_header(pkt, &ppp_hdr);

	uint8_t pkt_id = ppp_hdr_get_id(ppp_hdr);
	uint8_t pkt_length = ppp_hdr_get_length(ppp_hdr);

	if (pkt_id != cp->cr_sent_identifier || memcmp(cp->cr_sent_opts,pkt->data,pkt_length-sizeof(ppp_hdr_t)))
	{
		return -EBADMSG;
	}
	return E_RCA;
}

int handle_rcn_nak(ppp_cp_t *cp, gnrc_pktsnip_t *pkt)
{

	ppp_hdr_t *ppp_hdr;
	_pkt_get_ppp_header(pkt, &ppp_hdr);

	int has_options = _pkt_get_ppp_header(pkt, &ppp_hdr);

	if(!has_options)
	{
		/* If the packet doesn't have options, it's considered as invalid. */
		return -EBADMSG;
	}

	uint8_t pkt_length = ppp_hdr_get_length(ppp_hdr);
	uint8_t opt_reminder = pkt_length-sizeof(ppp_hdr_t);

	/* Check if options are valid */
	if (!ppp_conf_opts_valid(pkt, opt_reminder))
	{
		return -EBADMSG;
	}

	if (ppp_hdr_get_id(ppp_hdr) != cp->cr_sent_identifier)
		return -EBADMSG;
	return E_RCN;
}

int handle_rcn_rej(ppp_cp_t *cp, gnrc_pktsnip_t *pkt)
{
	ppp_hdr_t *ppp_hdr;
	int has_options = _pkt_get_ppp_header(pkt, &ppp_hdr);

	if(!has_options)
		return -EBADMSG;

	if (ppp_hdr_get_id(ppp_hdr) != cp->cr_sent_identifier)
		return -EBADMSG;

	/* Check if opts are subset of sent options */
	ppp_option_t *curr_opt = pkt->data;

	uint16_t size = cp->cr_sent_size;
	if(ppp_hdr_get_length(ppp_hdr)-sizeof(ppp_hdr_t) > cp->cr_sent_size)
		return -EBADMSG;

	uint8_t cursor=0;
	while(cursor<size)
	{
		if(!ppp_opt_is_subset(curr_opt, cp->cr_sent_opts, size))
			return -EBADMSG;
		cursor+=ppp_opt_get_length(curr_opt);
		curr_opt = (void*) ((uint8_t*) curr_opt + (int) ppp_opt_get_length(curr_opt));
	}
	return E_RCN;
}
int handle_coderej(gnrc_pktsnip_t *pkt)
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
int cp_init(ppp_dev_t *ppp_dev, ppp_cp_t *cp)
{
	cp->state = S_INITIAL;
	cp->cr_sent_identifier = 1;
	cp->dev = ppp_dev;
	return 0;
}

int handle_term_ack(ppp_cp_t *cp, gnrc_pktsnip_t *pkt)
{
	ppp_hdr_t *ppp_hdr;
	_pkt_get_ppp_header(pkt, &ppp_hdr);
	
	int id = ppp_hdr_get_id(ppp_hdr);
	if(id == cp->tr_sent_identifier)
	{
		return E_RTA;
	}
	return -EBADMSG;
}


int gnrc_ppp_init(ppp_dev_t *dev, netdev2_t *netdev)
{
	dev->netdev = netdev;
	lcp_init(dev, &dev->l_lcp);
	return 0;
}

int gnrc_ppp_recv(ppp_dev_t *dev, gnrc_pktsnip_t *pkt)
{
	/* Mark hdlc header */
	gnrc_pktsnip_t *result = gnrc_pktbuf_mark(pkt, sizeof(hdlc_hdr_t), GNRC_NETTYPE_HDLC);
	if (!result) {
		DEBUG("gnrc_ppp: no space left in packet buffer\n");
		return 0; /*TODO:Fix right value */	
	}
	
	hdlc_hdr_t *hdlc_hdr = (hdlc_hdr_t*) result->data;

	/* Route the packet according to prot(ocol */
	switch(hdlc_hdr_get_protocol(hdlc_hdr))
	{
		case PPPTYPE_IPV4:
			/* Check if NCP is up. Silently discard pkt if not */
			/*if (!dev->l_ncp->up)
			{
				break;
			}*/
			//_pktsnd_upper_layer(dev, &cp_pkt);
			//goto dont_discard;
			break;
		case PPPTYPE_LCP:
			/* Populate received pkt */
			dev->l_lcp.handle_pkt(&dev->l_lcp, pkt);
			break;
		case PPPTYPE_NCP_IPV4:
		//	_handle_cp_pkt(dev->l_ncp, &cp_pkt);
			break;
		default:
			break;
	}

		gnrc_pktbuf_release(pkt);
		return 0; /*TODO:Fix right value */

	//dont_discard:
	//	return 0;/*TODO: Fix right value */
}

/**
 * @brief   Startup code and event loop of the PPP layer
 *
 * @param[in] args          expects a pointer to the underlying netdev device
 *
 * @return                  never returns
 */

/* Generate PPP pkt */
gnrc_pktsnip_t * pkt_build(gnrc_nettype_t pkt_type, uint8_t code, uint8_t id, gnrc_pktsnip_t *payload)
{
	ppp_hdr_t ppp_hdr;
	ppp_hdr_set_code(&ppp_hdr, code);
	ppp_hdr_set_id(&ppp_hdr, id);
	int payload_length = gnrc_pkt_len(payload);
	ppp_hdr_set_length(&ppp_hdr, payload_length + sizeof(ppp_hdr_t));

	gnrc_pktsnip_t *ppp_pkt = gnrc_pktbuf_add(payload, (void*) &ppp_hdr, sizeof(ppp_hdr_t), pkt_type);
	return ppp_pkt;
}

int gnrc_ppp_send(netdev2_t *dev, gnrc_pktsnip_t *pkt)
{
	hdlc_hdr_t hdlc_hdr;

	hdlc_hdr_set_address(&hdlc_hdr, PPP_HDLC_ADDRESS);
	hdlc_hdr_set_control(&hdlc_hdr, PPP_HDLC_CONTROL);
	hdlc_hdr_set_protocol(&hdlc_hdr, gnrc_nettype_to_ppp_protnum(pkt->type));

	gnrc_pktsnip_t *hdr = gnrc_pktbuf_add(pkt, (void*) &hdlc_hdr, sizeof(hdlc_hdr_t), GNRC_NETTYPE_HDLC);
	/* Get iovec representation */
	size_t n;
	int res = -ENOBUFS;
	hdr = gnrc_pktbuf_get_iovec(hdr, &n);
	if (hdr != NULL)
	{
		struct iovec *vector = (struct iovec*) hdr->data;
		res = dev->driver->send(dev, vector, n);
	}
	return res;
}
int gnrc_ppp_event_callback(ppp_dev_t *dev, int ppp_event)
{
	DEBUG("Hi!");
	int nbytes;
	ppp_cp_t *target_protocol;
	uint8_t event = ppp_event & 0xFF;
	switch((ppp_event & 0xFF00)>>8)
	{
		case 1:
			target_protocol = &dev->l_lcp;
			break;
		case 0xFF:
			target_protocol = &dev->l_lcp;
			break;
		default:
			DEBUG("Unrecognized PPP protocol event!\n");
			return -EBADMSG;
			break;
	}


	switch (event)
	{
		case PPP_RECV:
			nbytes = dev->netdev->driver->recv(dev->netdev, NULL, 0, NULL);
			gnrc_pktsnip_t *pkt = gnrc_pktbuf_add(NULL, NULL, nbytes, GNRC_NETTYPE_UNDEF);
			dev->netdev->driver->recv(dev->netdev, pkt->data, nbytes, NULL);
			gnrc_ppp_recv(dev, pkt);
			break;
		case PPP_LINKUP:
			DEBUG("Event: PPP_LINKUP\n");
			trigger_event(target_protocol, E_UP, NULL);
			trigger_event(target_protocol, E_OPEN, NULL);
			break;
		case PPP_TIMEOUT:
			if(target_protocol->restart_counter)
			{
				DEBUG("Event: TO+\n");
				trigger_event(target_protocol, E_TOp, NULL);
			}
			else
			{
				DEBUG("Event: TO-\n");
				trigger_event(target_protocol, E_TOm, NULL);
			}
	}
	return 0;
}
