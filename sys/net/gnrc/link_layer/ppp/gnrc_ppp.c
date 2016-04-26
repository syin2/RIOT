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

#include "msg.h"
#include "thread.h"
#include "net/gnrc.h"
#include "net/gnrc/netdev2.h"
#include "net/ppptype.h"
#include "net/gnrc/ppp/ppp.h"
#include "net/gnrc/ppp/lcp.h"
#include "net/gnrc/ppp/fsm.h"
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

#define dump_hex 1


/* Generate PPP pkt */
gnrc_pktsnip_t * pkt_build(gnrc_nettype_t pkt_type, uint8_t code, uint8_t id, gnrc_pktsnip_t *payload)
{
	ppp_hdr_t ppp_hdr;
	ppp_hdr_set_code(&ppp_hdr, code);
	ppp_hdr_set_id(&ppp_hdr, id);

	int payload_length = payload ? payload->size : 0;
	ppp_hdr_set_length(&ppp_hdr, payload_length + sizeof(ppp_hdr_t));

	gnrc_pktsnip_t *ppp_pkt = gnrc_pktbuf_add(payload, (void*) &ppp_hdr, sizeof(ppp_hdr_t), pkt_type);
	return ppp_pkt;
}

uint8_t mark_ppp_pkt(gnrc_pktsnip_t *pkt)
{
	gnrc_pktsnip_t *result = gnrc_pktbuf_mark(pkt, sizeof(hdlc_hdr_t), GNRC_NETTYPE_HDLC);
	if (!result) {
		DEBUG("gnrc_ppp: no space left in packet buffer\n");
		return 0; /*TODO:Fix right value */	
	}
	
	hdlc_hdr_t *hdlc_hdr = (hdlc_hdr_t*) result->data;

	switch(hdlc_hdr_get_protocol(hdlc_hdr))
	{
		case PPPTYPE_LCP:
			return ID_LCP;
			break;
		case PPPTYPE_NCP_IPV4:
			return ID_IPCP;
			break;
		default:
			DEBUG("Unknown PPP protocol");
	}
	return 0;
}
gnrc_pktsnip_t *retrieve_pkt(pppdev_t *dev)
{
		int nbytes;
		nbytes = dev->driver->recv(dev, NULL, 0, NULL);
		gnrc_pktsnip_t *pkt = gnrc_pktbuf_add(NULL, NULL, nbytes, GNRC_NETTYPE_UNDEF);
		dev->driver->recv(dev, pkt->data, nbytes, NULL);
		return pkt;
}

void print_protocol(uint16_t protocol)
{
	switch(protocol)
	{
		case PPPTYPE_LCP:
			DEBUG("LCP");
			break;
		case PPPTYPE_NCP_IPV4:
			DEBUG("IPCP");
			break;
		default:
			DEBUG("UNKNOWN_PROTOCOL");
			break;
	}
}
void print_ppp_code(uint8_t code)
{
	switch(code)
	{
		case PPP_CONF_REQ:
			DEBUG("Configure Request");
			break;
		case PPP_CONF_ACK:
			DEBUG("Configure Ack");
			break;
		case PPP_CONF_NAK:
			DEBUG("Configure Nak");
			break;
		case PPP_CONF_REJ:
			DEBUG("Configure Rej");
			break;
		case PPP_TERM_REQ:
			DEBUG("Terminate Request");
			break;
		case PPP_TERM_ACK:
			DEBUG("Terminate Ack");
			break;
		case PPP_CODE_REJ:
			DEBUG("Code Reject");
			break;
		case PPP_PROT_REJ:
			DEBUG("Protocol Reject");
			break;
		case PPP_ECHO_REQ:
			DEBUG("Echo Request");
			break;
		case PPP_ECHO_REP:
			DEBUG("Echo Reply");
			break;
		case PPP_DISC_REQ:
			DEBUG("Discard Request");
			break;
		case PPP_IDENT:
			DEBUG("Identification");
			break;
		case PPP_TIME_REM:
			DEBUG("Time Remaining");
			break;
		case PPP_UNKNOWN_CODE:
			DEBUG("Unknown");
			break;
	}
}
void print_opts(gnrc_pktsnip_t *payload)
{
	DEBUG("OPTS:<");
	ppp_option_t *head = payload->data;
	ppp_option_t *curr_opt = head;
	uint8_t type, length;
	uint8_t *p;
	if(payload)
	{
		while(curr_opt)
		{
			type = ppp_opt_get_type(curr_opt);
			length = ppp_opt_get_length(curr_opt);
			DEBUG("\n\t[TYPE:%i, LENGTH:%i, VALUE:<",type, length);
			p = (uint8_t*) ppp_opt_get_payload(curr_opt);
			for(int i=0;i<length-2; i++)
			{
				DEBUG(" %02x ", *(p+i));
			}
			DEBUG(">]");
			curr_opt = ppp_opt_get_next(curr_opt, head, payload->size);
			if(curr_opt)
				DEBUG(",");
		}
	}
	else
	{
		DEBUG("None");
	}
	DEBUG(">");
}
void print_pkt(gnrc_pktsnip_t *hdlc_hdr, gnrc_pktsnip_t *ppp_hdr, gnrc_pktsnip_t *payload)
{
	hdlc_hdr_t *hdlc = (hdlc_hdr_t*) hdlc_hdr->data;
	DEBUG("[");
	print_protocol(hdlc_hdr_get_protocol(hdlc));
	DEBUG(": ");

	ppp_hdr_t *ppp = (ppp_hdr_t*) ppp_hdr->data;
	uint8_t code = ppp_hdr_get_code(ppp);
	print_ppp_code(code);
	DEBUG(",ID:%i,SIZE:%i,", ppp_hdr_get_id(ppp), ppp_hdr_get_length(ppp));
	if(code >= PPP_CONF_REQ && code <= PPP_CODE_REJ)
	{
		print_opts(payload);
	}
	else
	{
		DEBUG("DATA:<");
		for(int i=0;i<payload->size;i++)
		{
			DEBUG(" %02x ", (int)*(((uint8_t*)payload->data)+i));
		}
	}
	DEBUG("] ");

	int i;
	DEBUG("HEX: <");
	for(i=0;i<4;i++)
	{
		DEBUG("%02x ", *(((uint8_t*)hdlc_hdr->data)+i));
	}
	for(i=0;i<4;i++)
	{
		DEBUG("%02x ", *(((uint8_t*)ppp_hdr->data)+i));
	}
	int payload_len;
	if(payload)
	{
		payload_len = payload->size;
		for(i=0;i<payload_len;i++)
		{
			DEBUG("%02x ", *(((uint8_t*)payload->data)+i));
		}
	}
	DEBUG(">\n");
}

int gnrc_ppp_init(gnrc_pppdev_t *dev, pppdev_t *netdev)
{
	dev->netdev = netdev;
	dev->state = PPP_LINK_DEAD;

	lcp_init(dev, (ppp_fsm_t*) dev->l_lcp);
	ipcp_init(dev, (ppp_fsm_t*) dev->l_ipcp);
	return 0;
}


int gnrc_ppp_send(pppdev_t *dev, gnrc_pktsnip_t *pkt)
{
	hdlc_hdr_t hdlc_hdr;

	hdlc_hdr_set_address(&hdlc_hdr, PPP_HDLC_ADDRESS);
	hdlc_hdr_set_control(&hdlc_hdr, PPP_HDLC_CONTROL);
	hdlc_hdr_set_protocol(&hdlc_hdr, gnrc_nettype_to_ppp_protnum(pkt->type));

	gnrc_pktsnip_t *hdr = gnrc_pktbuf_add(pkt, (void*) &hdlc_hdr, sizeof(hdlc_hdr_t), GNRC_NETTYPE_HDLC);
	DEBUG(">>>>>>>>>> SEND:");
	print_pkt(hdr, pkt, pkt->next);
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


int dispatch_ppp_msg(gnrc_pppdev_t *dev, int ppp_msg)
{
	DEBUG("Receiving a PPP_NETTYPE msg\n");
	uint8_t target = (ppp_msg & 0xFF00)>>8;
	uint8_t event = ppp_msg & 0xFF;
	gnrc_pktsnip_t *pkt = NULL;
	if(event == PPP_RECV)
	{
		pkt = retrieve_pkt(dev->netdev);
		target = mark_ppp_pkt(pkt);
		if(!target)
		{
			DEBUG("Please implement protocol reject!");
			return -EBADMSG;
		}
	}
	/*Here we have the target*/
	switch(target)
	{
		case ID_LCP:
		case 0xFF:
			dev->l_lcp->handler(dev->l_lcp, event, pkt);
			break;
		case ID_IPCP:
		case 0xFE:
			dev->l_ipcp->handler(dev->l_ipcp, event, pkt);
			break;
		default:
			break;
	}
	return 0;
}

void *gnrc_ppp_thread(void *args)
{
    //Setup a new sim900 devide
	gnrc_pppdev_t pppdev;
	ppp_fsm_t lcp;
	ppp_fsm_t ipcp;

	pppdev.l_lcp = (ppp_protocol_t*) &lcp;
	pppdev.l_ipcp = (ppp_protocol_t*) &ipcp;

	gnrc_ppp_init(&pppdev, (pppdev_t*) args);

	pppdev_t *d = pppdev.netdev;
    d->driver->init(d);

	msg_t msg_queue[GNRC_PPP_MSG_QUEUE];;
	msg_init_queue(msg_queue, GNRC_PPP_MSG_QUEUE);
	msg_t msg;
	int event;
    while(1)
    {
    	msg_receive(&msg);
		event = msg.content.value;	
		switch(msg.type){
			case PPPDEV_MSG_TYPE_EVENT:
				dispatch_ppp_msg(&pppdev, event);
				break;
			case NETDEV2_MSG_TYPE_EVENT:
				d->driver->driver_ev((pppdev_t*) d, event);
				break;
    	}
    }
}

