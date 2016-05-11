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
#include "net/gnrc/ppp/ipcp.h"
#include "net/gnrc/ppp/fsm.h"
#include "net/hdlc/hdr.h"
#include "net/ppp/hdr.h"
#include <errno.h>
#include <string.h>

#define ENABLE_DEBUG    (0)
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

	uint16_t protocol = hdlc_hdr_get_protocol(hdlc_hdr);
	switch(protocol)
	{
		case PPPTYPE_LCP:
			return ID_LCP;
			break;
		case PPPTYPE_NCP_IPV4:
			return ID_IPCP;
			break;
		case PPPTYPE_IPV4:
			return ID_IPV4;
		default:
			DEBUG("Unknown PPP protocol: %i\n", protocol);
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

#if ENABLE_DEBUG
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
#endif
void print_pkt(gnrc_pktsnip_t *hdlc_hdr, gnrc_pktsnip_t *ppp_hdr, gnrc_pktsnip_t *payload)
{
#if ENABLE_DEBUG
	hdlc_hdr_t *hdlc = (hdlc_hdr_t*) hdlc_hdr->data;
	DEBUG("[");
	print_protocol(hdlc_hdr_get_protocol(hdlc));
	DEBUG(": ");

	ppp_hdr_t *ppp = (ppp_hdr_t*) ppp_hdr->data;
	uint8_t code = ppp_hdr_get_code(ppp);
	print_ppp_code(code);
	DEBUG(",ID:%i,SIZE:%i,", ppp_hdr_get_id(ppp), ppp_hdr_get_length(ppp));
	if(code >= PPP_CONF_REQ && code <= PPP_CONF_REJ)
	{
		print_opts(payload);
	}
	else
	{
		DEBUG("DATA:<");
		if(payload)
		{
			for(int i=0;i<payload->size;i++)
			{
				DEBUG(" %02x ", (int)*(((uint8_t*)payload->data)+i));
			}
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
#endif
}

int gnrc_ppp_setup(gnrc_pppdev_t *dev, pppdev_t *netdev)
{
	dev->netdev = netdev;
	dev->state = PPP_LINK_DEAD;

	dcp_init(dev, (ppp_protocol_t*) &dev->l_dcp);
	lcp_init(dev, (ppp_fsm_t*) &dev->l_lcp);
	ipcp_init(dev, (ppp_fsm_t*) &dev->l_ipcp);
	ppp_ipv4_init(dev, (ppp_ipv4_t*) &dev->l_ipv4, (ipcp_t*) &dev->l_ipcp, dev);

	trigger_event((ppp_fsm_t*) &dev->l_lcp, E_OPEN, NULL);
	trigger_event((ppp_fsm_t*) &dev->l_ipcp, E_OPEN, NULL);
	return 0;
}


int gnrc_ppp_send(gnrc_pppdev_t *dev, gnrc_pktsnip_t *pkt)
{
	hdlc_hdr_t hdlc_hdr;

	hdlc_hdr_set_address(&hdlc_hdr, PPP_HDLC_ADDRESS);
	hdlc_hdr_set_control(&hdlc_hdr, PPP_HDLC_CONTROL);
	hdlc_hdr_set_protocol(&hdlc_hdr, gnrc_nettype_to_ppp_protnum(pkt->type));

	gnrc_pktsnip_t *hdr = gnrc_pktbuf_add(pkt, (void*) &hdlc_hdr, sizeof(hdlc_hdr_t), GNRC_NETTYPE_HDLC);
	DEBUG(">>>>>>>>>> SEND:");

	/*TODO: De-hardcode this*/
	if(hdlc_hdr_get_protocol(&hdlc_hdr) == PPPTYPE_IPV4)
	{
		DEBUG("HDLC's IPV4: <");
		for(gnrc_pktsnip_t *p=hdr;p!=NULL;p=p->next)
		{
			for(int i=0;i<p->size;i++)
			{
				DEBUG("%02x ", *(((uint8_t*) p->data)+i));
			}
		}
	}
	else
	{
		print_pkt(hdr, pkt, pkt->next);
	}
	
	DEBUG("Hdr: %p\n", dev);
	if(gnrc_pkt_len(hdr) > ((lcp_t*) &dev->l_lcp)->peer_mru)
	{
		DEBUG("Sending exceeds peer MRU. Dropping packet.\n");
		gnrc_pktbuf_release(hdr);
		return -EBADMSG;
	}
	/* Get iovec representation */
	size_t n;
	int res = -ENOBUFS;
	hdr = gnrc_pktbuf_get_iovec(hdr, &n);
	if (hdr != NULL)
	{
		struct iovec *vector = (struct iovec*) hdr->data;
		res = dev->netdev->driver->send(dev->netdev, vector, n);
	}
	gnrc_pktbuf_release(hdr);
	return res;
}

void send_protocol_reject(lcp_t *lcp, gnrc_pktsnip_t *ppp_pkt)
{
	/* Remove hdlc header */
	network_uint16_t protocol = byteorder_htons(hdlc_hdr_get_protocol(ppp_pkt->next->data));
	gnrc_pktbuf_remove_snip(ppp_pkt, ppp_pkt->next);
	gnrc_pktsnip_t *rp = gnrc_pktbuf_add(ppp_pkt, &protocol, 2, GNRC_NETTYPE_UNDEF);
	gnrc_pktsnip_t *send_pkt = pkt_build(GNRC_NETTYPE_LCP, PPP_PROT_REJ, lcp->pr_id++, rp);
	gnrc_ppp_send(((ppp_fsm_t*) lcp)->dev, send_pkt);
}

int gnrc_ppp_get_state(gnrc_pppdev_t *dev)
{
	ppp_fsm_t *lcp = (ppp_fsm_t*) &dev->l_lcp;
	switch(lcp->state)
	{
		case S_INITIAL:
		case S_STARTING:
			return PPP_LINK_DEAD;
			break;
		case S_CLOSED:
		case S_STOPPED:
		case S_CLOSING:
		case S_STOPPING:
			return PPP_TERMINATION;
		case S_REQ_SENT:
		case S_ACK_RCVD:
		case S_ACK_SENT:
			return PPP_LINK_ESTABLISHED;
		case S_OPENED:
			return PPP_NETWORK;
			break;
		default:
			break;
	}
	DEBUG("gnrc_ppp_get_state: Shouldn't be here!\n");
	return 0;
}

uint8_t _pkt_allowed(uint8_t state, uint8_t target)
{
	switch(target)
	{
		case ID_LCP:
			return state == PPP_LINK_ESTABLISHED || state == PPP_AUTHENTICATION || state == PPP_NETWORK || state == PPP_TERMINATION;
		case ID_IPCP:
		case ID_IPV4:
			return state == PPP_NETWORK;
		default:
			DEBUG("Pkt not allowed in this PPP state. Discard\n"); 
			break;
	}
	return 0;
}

int dispatch_ppp_msg(gnrc_pppdev_t *dev, int ppp_msg)
{
	uint8_t target = (ppp_msg & 0xFF00)>>8;
	uint8_t event = ppp_msg & 0xFF;
	DEBUG("Receiving a PPP_NETTYPE msg with target %i and event %i\n", target, event);
	int ppp_state;
	gnrc_pktsnip_t *pkt = NULL;

	if(event == PPP_RECV)
	{
		pkt = retrieve_pkt(dev->netdev);
		target = mark_ppp_pkt(pkt);
		if(!target)
		{
			send_protocol_reject((lcp_t*) &dev->l_lcp, pkt);
			return -EBADMSG;
		}
		ppp_state = gnrc_ppp_get_state(dev);
		DEBUG("PPP STATE IS: %i\n", ppp_state);
		if(!_pkt_allowed(ppp_state, target))
		{
			gnrc_pktbuf_release(pkt);
			return -EBADMSG;
		}
	}

	/*Drop packet if exceeds MRU*/
	if(gnrc_pkt_len(pkt) > ((lcp_t*) &dev->l_lcp)->mru)
	{
		DEBUG("gnrc_ppp: Exceeded MRU of device. Dropping packet.\n");
		gnrc_pktbuf_release(pkt);
		return -EBADMSG;
	}

	ppp_protocol_t *target_prot;
	switch(target)
	{
		case ID_LCP:
			target_prot = (ppp_protocol_t*) &dev->l_lcp;
			break;
		case ID_IPCP:
		case 0xFE:
			target_prot = (ppp_protocol_t*) &dev->l_ipcp;
			break;
		case ID_IPV4:
			target_prot = (ppp_protocol_t*) &dev->l_ipv4;
			break;
		case ID_PPPDEV:
		case 0xFF:
			target_prot = (ppp_protocol_t*) &dev->l_dcp;
			break;
		default:
			DEBUG("Unrecognized target\n");
			return -1;
			break;
	}
	target_prot->handler(target_prot, event, pkt);
	return 0;
}


int gnrc_ppp_set_opt(gnrc_pppdev_t *dev, netopt_t opt, void *value, size_t value_len)
{
	switch(opt)
	{
		case NETOPT_APN_NAME:
			DEBUG("Setting APN!\n");
			dev->netdev->driver->set(dev->netdev, PPPOPT_APN_NAME, value, value_len);
			break;
		default:
			DEBUG("No options from gnrc_ppp yet!\n");
			break;
	}
	return 0;
}


void *_gnrc_ppp_thread(void *args)
{
	gnrc_pppdev_t *pppdev = (gnrc_pppdev_t*) args;
	gnrc_netif_add(thread_getpid());
	pppdev_t *d = pppdev->netdev;
    d->driver->init(d);

	msg_t msg_queue[GNRC_PPP_MSG_QUEUE];;
	msg_init_queue(msg_queue, GNRC_PPP_MSG_QUEUE);
	msg_t msg, reply;
	int event, res;
	gnrc_netapi_opt_t *opt;
    while(1)
    {
    	msg_receive(&msg);
		event = msg.content.value;	
		switch(msg.type){
			case GNRC_PPPDEV_MSG_TYPE_EVENT:
				dispatch_ppp_msg(pppdev, event);
				break;
			case PPPDEV_MSG_TYPE_EVENT:
				d->driver->driver_ev((pppdev_t*) d, event);
				break;
			case GNRC_NETAPI_MSG_TYPE_SET:
				DEBUG("\n\n\n :) \n\n\n");
				opt = (gnrc_netapi_opt_t*) msg.content.ptr;
				res = gnrc_ppp_set_opt(pppdev, opt->opt, opt->data, opt->data_len);
				reply.type = GNRC_NETAPI_MSG_TYPE_ACK;
				reply.content.value = (uint32_t) res;
				msg_reply(&msg, &reply);
				break;
            case GNRC_NETAPI_MSG_TYPE_SND:
                DEBUG("gnrc_pppdev: GNRC_NETAPI_MSG_TYPE_SND received\n");
                gnrc_pktsnip_t *pkt = (gnrc_pktsnip_t *)msg.content.ptr;
                ppp_ipv4_send(pppdev, pkt);
                break;
			default:
				DEBUG("Received an unknown thread msg\n");
    	}
    }
}

void gnrc_ppp_trigger_event(msg_t *msg, kernel_pid_t pid, uint8_t target, uint8_t event)
{
	msg->type = GNRC_PPPDEV_MSG_TYPE_EVENT;
	msg->content.value = (target<<8)|(event & 0xffff);
	msg_send(msg, pid);
}

void gnrc_ppp_link_up(msg_t *msg, kernel_pid_t pid)
{
	gnrc_ppp_trigger_event(msg, pid, 0xFF, PPP_LINKUP);
}

void gnrc_ppp_link_down(msg_t *msg, kernel_pid_t pid)
{
	gnrc_ppp_trigger_event(msg, pid, 0xFF, PPP_LINKDOWN);
}

void gnrc_ppp_dispatch_pkt(msg_t *msg, kernel_pid_t pid)
{
	gnrc_ppp_trigger_event(msg, pid, 0xFF, PPP_RECV);
}

void gnrc_ppp_dial_up(msg_t *msg, kernel_pid_t pid)
{
	gnrc_ppp_trigger_event(msg, pid, ID_PPPDEV, PPP_DIALUP);
}

kernel_pid_t gnrc_pppdev_init(char *stack, int stacksize, char priority,
                        const char *name, gnrc_pppdev_t *gnrc_pppdev)
{
    kernel_pid_t res;

    /* check if given netdev device is defined and the driver is set */
    if (gnrc_pppdev == NULL || gnrc_pppdev->netdev == NULL) {
        return -ENODEV;
    }

    /* create new gnrc_pppdev thread */
    res = thread_create(stack, stacksize, priority, THREAD_CREATE_STACKTEST,
                         _gnrc_ppp_thread, (void *)gnrc_pppdev, name);
    if (res <= 0) {
        return -EINVAL;
    }

    return res;
}
