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


void print_pkt(gnrc_pktsnip_t *pkt)
{
	for(int i=0;i<pkt->size;i++)
	{
		DEBUG("%i\n", (int)*(((uint8_t*)pkt->data)+i));
	}
}

int gnrc_ppp_init(gnrc_pppdev_t *dev, pppdev_t *netdev)
{
	dev->netdev = netdev;
	dev->state = PPP_LINK_DEAD;

	lcp_init(dev, (ppp_cp_t*) dev->l_lcp);
	ipcp_init(dev, (ppp_cp_t*) dev->l_ipcp);
	return 0;
}

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

int gnrc_ppp_send(pppdev_t *dev, gnrc_pktsnip_t *pkt)
{
	hdlc_hdr_t hdlc_hdr;

	hdlc_hdr_set_address(&hdlc_hdr, PPP_HDLC_ADDRESS);
	hdlc_hdr_set_control(&hdlc_hdr, PPP_HDLC_CONTROL);
	hdlc_hdr_set_protocol(&hdlc_hdr, gnrc_nettype_to_ppp_protnum(pkt->type));

	DEBUG("Sending with protocol: %i\n", gnrc_nettype_to_ppp_protnum(pkt->type));
	gnrc_pktsnip_t *hdr = gnrc_pktbuf_add(pkt, (void*) &hdlc_hdr, sizeof(hdlc_hdr_t), GNRC_NETTYPE_HDLC);
	DEBUG("Sending:\n");
	print_pkt(hdr);
	print_pkt(pkt);
	if(pkt->next)
		print_pkt(pkt->next);
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

int fsm_handle_ppp_msg(struct ppp_protocol_t *protocol, uint8_t ppp_event, void *args)
{
	ppp_cp_t *target = (ppp_cp_t*) protocol;
	uint8_t event;
	gnrc_pktsnip_t *pkt = (gnrc_pktsnip_t*) args;
	switch(ppp_event)
	{
		case PPP_RECV:
			event = fsm_event_from_pkt(target, pkt);
			trigger_event(target, event, pkt);
			/*TODO: Fix this*/
			gnrc_pktbuf_release(pkt);
			break;
		case PPP_LINKUP:
			DEBUG("Event: PPP_LINKUP\n");
			/*Set here PPP states...*/
			trigger_event(target, E_OPEN, NULL);
			trigger_event(target, E_UP, NULL);
			break;
		case PPP_LINKDOWN:
			/* Just to test, print message when this happens */
			DEBUG("Some layer finished\n");
			break;
		case PPP_TIMEOUT:
			if(target->restart_counter)
			{
				DEBUG("Event: TO+\n");
				trigger_event(target, E_TOp, NULL);
			}
			else
			{
				DEBUG("Event: TO-\n");
				trigger_event(target, E_TOm, NULL);
			}
			break;
	}
	return 0;
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
	ppp_cp_t lcp;
	ppp_cp_t ipcp;

	pppdev.l_lcp = (ppp_protocol_t*) &lcp;
	pppdev.l_ipcp = (ppp_protocol_t*) &ipcp;

	gnrc_ppp_init(&pppdev, (pppdev_t*) args);



	pppdev_t *d = pppdev.netdev;
    d->driver->init(d);

	msg_t msg_queue[GNRC_PPP_MSG_QUEUE];;
	msg_init_queue(msg_queue, GNRC_PPP_MSG_QUEUE);
	msg_t msg;
#if 0
	sim900_t *dev = (sim900_t*) d;
#if TEST_PPP
	dev->state = AT_STATE_RX;
	dev->ppp_rx_state = PPP_RX_IDLE;
	dev->msg.type = NETDEV2_MSG_TYPE_EVENT;
	dev->msg.content.value = PDP_UP;
	msg_send(&dev->msg, dev->mac_pid);
#else
#endif
#if TEST_WRITE
	test_sending(dev);
#endif
#endif

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

void broadcast_upper_layer(msg_t *msg, uint8_t id, uint8_t event)
{
	DEBUG("Sending msg to upper layer...\n");
	msg->type = PPPDEV_MSG_TYPE_EVENT;
	uint8_t target;
	switch(id)
	{
		case ID_LCP:
			target = 0xFE;
			break;
		default:
			DEBUG("Unrecognized lower layer!\n");
			return;
	}
	msg->content.value = (target<<8) + event;
	msg_send(msg, thread_getpid());
}

