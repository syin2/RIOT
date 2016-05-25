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
#include "net/ipv6/addr.h"
#include "net/gnrc/ipv6/netif.h"
#include "net/gnrc/ppp/pap.h"
#include "net/hdlc/hdr.h"
#include "net/ppp/hdr.h"
#include "net/eui64.h"
#include <errno.h>
#include <string.h>

#define ENABLE_DEBUG    (1)
#include "debug.h"

#if ENABLE_DEBUG
/* For PRIu16 etc. */
#include <inttypes.h>
#endif

#define dump_hex 1
#define DUMMY_ADDR_LEN (6)


void send_ppp_event(msg_t *msg, ppp_msg_t ppp_msg)
{
	msg->type = GNRC_PPPDEV_MSG_TYPE_EVENT;
	msg->content.value = ppp_msg;
	msg_send(msg, thread_getpid());
}

void send_ppp_event_xtimer(msg_t *msg, xtimer_t *xtimer, ppp_msg_t ppp_msg, int timeout)
{
	msg->type = GNRC_PPPDEV_MSG_TYPE_EVENT;
	msg->content.value = ppp_msg;
	xtimer_remove(xtimer);
	xtimer_set_msg(xtimer, timeout, msg, thread_getpid());
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

ppp_target_t _get_target_from_protocol(uint16_t protocol)
{
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
		case PPPTYPE_PAP:
			return ID_PAP;
		default:
			DEBUG("Unknown PPP protocol: %i\n", protocol);
	}
	return ID_UNDEF;
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
			ppp_opt_get_payload(curr_opt, (void**) &p);
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
	if(code >= PPP_CONF_REQ && code <= PPP_CONF_REJ && (ppp_hdr->type == GNRC_NETTYPE_LCP || ppp_hdr->type == GNRC_NETTYPE_IPCP))
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
	pap_init(dev, (pap_t*) &dev->l_pap);

	trigger_fsm_event((ppp_fsm_t*) &dev->l_lcp, E_OPEN, NULL);
	trigger_fsm_event((ppp_fsm_t*) &dev->l_ipcp, E_OPEN, NULL);
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

int dispatch_ppp_msg(gnrc_pppdev_t *dev, ppp_msg_t ppp_msg)
{
	ppp_target_t target = ppp_msg_get_target(ppp_msg);
	ppp_event_t event = ppp_msg_get_event(ppp_msg);
	gnrc_pktsnip_t *pkt = NULL;

	if(event == PPP_RECV)
	{
		pkt = retrieve_pkt(dev->netdev);
		gnrc_pktsnip_t *result = gnrc_pktbuf_mark(pkt, sizeof(hdlc_hdr_t), GNRC_NETTYPE_HDLC);
		if (!result) {
			DEBUG("gnrc_ppp: no space left in packet buffer\n");
			return -ENOBUFS;
		}
	
		hdlc_hdr_t *hdlc_hdr = (hdlc_hdr_t*) result->data;

		target = _get_target_from_protocol(hdlc_hdr_get_protocol(hdlc_hdr));
		if(!target)
		{
			/* Remove hdlc header */
			network_uint16_t protocol = byteorder_htons(hdlc_hdr_get_protocol(pkt->next->data));
			gnrc_pktbuf_remove_snip(pkt, pkt->next);
			gnrc_pktsnip_t *rp = gnrc_pktbuf_add(pkt, &protocol, 2, GNRC_NETTYPE_UNDEF);
			send_protocol_reject(dev, dev->l_lcp.pr_id++, rp);
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
		case ID_PAP:
			target_prot = (ppp_protocol_t*) &dev->l_pap;
			break;
		default:
			DEBUG("Unrecognized target\n");
			return -1;
			break;
	}
	target_prot->handler(target_prot, event, pkt);
	return 0;
}


static int _get_iid(pppdev_t *pppdev, eui64_t *value, size_t max_len)
{
    if (max_len < sizeof(eui64_t)) {
        return -EOVERFLOW;
    }

    uint8_t addr[DUMMY_ADDR_LEN];
    pppdev->driver->get(pppdev, NETOPT_ADDRESS, addr, DUMMY_ADDR_LEN);
    value->uint8[0] = addr[0] ^ 0x02;
    value->uint8[1] = addr[1];
    value->uint8[2] = addr[2];
    value->uint8[3] = 0xff;
    value->uint8[4] = 0xfe;
    value->uint8[5] = addr[3];
    value->uint8[6] = addr[4];
    value->uint8[7] = addr[5];

    return sizeof(eui64_t);
}
int gnrc_ppp_get_opt(gnrc_pppdev_t *dev, netopt_t opt, void *value, size_t value_len)
{
	int res;
	/*Fake values*/
	uint8_t mac[] = {38, 53, 143, 233, 6, 93};
	switch(opt)
	{
		case NETOPT_ADDRESS:
			memcpy(value, mac, 6);
			return 6;
		case NETOPT_IPV6_IID:
			return _get_iid(dev->netdev, value, DUMMY_ADDR_LEN);
			return 8;
		default:
			res = -ENOTSUP;
			break;
	}
	return res;
}

int gnrc_ppp_set_opt(gnrc_pppdev_t *dev, netopt_t opt, void *value, size_t value_len)
{
	int res;
	switch(opt)
	{
		case NETOPT_APN_NAME:
			res = dev->netdev->driver->set(dev->netdev, PPPOPT_APN_NAME, value, value_len);
			break;
		case NETOPT_TUNNEL_IPV4_ADDRESS:
			dev->l_ipv4.tunnel_addr = *((ipv4_addr_t*) value);
			res = 0;
			break;
		case NETOPT_TUNNEL_UDP_PORT:
			dev->l_ipv4.tunnel_port = *((uint16_t*) value);
			res = 0;
			break;
		case NETOPT_APN_USER:
			memcpy(dev->l_pap.username, value, value_len);
			dev->l_pap.user_size = value_len;
			res = 0;
			break;
		case NETOPT_APN_PASS:
			memcpy(dev->l_pap.password, value, value_len);
			dev->l_pap.pass_size = value_len;
			res = 0;
			break;
		default:
			DEBUG("Unknown option\n");
			res = -ENOTSUP;
			break;
	}
	return res;
}


void *_gnrc_ppp_thread(void *args)
{
	DEBUG("gnrc_ppp_trhead started\n");
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
		DEBUG("gnrc_ppp: waiting for msg\n");
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
				opt = (gnrc_netapi_opt_t*) msg.content.ptr;
				res = gnrc_ppp_set_opt(pppdev, opt->opt, opt->data, opt->data_len);
				reply.type = GNRC_NETAPI_MSG_TYPE_ACK;
				reply.content.value = (uint32_t) res;
				msg_reply(&msg, &reply);
				break;
			case GNRC_NETAPI_MSG_TYPE_GET:
				opt = (gnrc_netapi_opt_t*) msg.content.ptr;
				res = gnrc_ppp_get_opt(pppdev, opt->opt, opt->data, opt->data_len);
				reply.type = GNRC_NETAPI_MSG_TYPE_ACK;
				reply.content.value = (uint32_t) res;
				msg_reply(&msg, &reply);
				break;
            case GNRC_NETAPI_MSG_TYPE_SND:
                ppp_ipv4_send(pppdev, (gnrc_pktsnip_t*) msg.content.ptr);
                break;
			default:
				DEBUG("Received an unknown thread msg: %i\n", msg.type);
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
void ppp_protocol_init(ppp_protocol_t *protocol, struct gnrc_pppdev_t *pppdev, int (*handler)(struct ppp_protocol_t*, uint8_t, void*), uint8_t id)
{
	protocol->handler = handler;
	protocol->id = id;
	protocol->pppdev = pppdev;
	protocol->state = PROTOCOL_DOWN;
}
