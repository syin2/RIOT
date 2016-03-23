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

#define ENABLE_DEBUG    (1)
#include "debug.h"

#if ENABLE_DEBUG
/* For PRIu16 etc. */
#include <inttypes.h>
#endif

/**
 * @brief   Function called by the device driver on device events
 *
 * @param[in] event         type of event
 * @param[in] data          optional parameter
 */
#if 0
static void _event_cb(gnrc_netdev_event_t event, void *data)
{
    DEBUG("ppp: event triggered -> %i\n", event);
    /* PPP only understands the RX_COMPLETE event... */
    if (event == NETDEV_EVENT_RX_COMPLETE) {
        gnrc_pktsnip_t *pkt;

        /* get pointer to the received packet */
        pkt = (gnrc_pktsnip_t *)data;
        /* send the packet to everyone interested in it's type */
        if (!gnrc_netapi_dispatch_receive(pkt->type, GNRC_NETREG_DEMUX_CTX_ALL, pkt)) {
            DEBUG("ppp: unable to forward packet of type %i\n", pkt->type);
            gnrc_pktbuf_release(pkt);
        }
    }
}
static int _read_lcp_pkt(uint8_t type, uint8_t *payload, size_t size, cp_opt_t *opt_buf)
{
	uint16_t u16;
	opt_buf->type = type;
	/* For the moment, only MRU option supported */
	DEBUG("lcp_opt_type: %i\n",type);
	DEBUG("lcp_opt_size: %i\n",(int) size);
	switch(type)
	{
		case LCP_OPT_MRU:
			if(size != 4) /* TODO: Replace with label*/
				return -1; /* TODO: Replace with label*/
			u16 = ((*payload)<<8) + *(payload+1);
			opt_buf->payload[0] = (*payload);
			opt_buf->payload[1] = *(payload+1);
			opt_buf->p_size = (size_t) sizeof(uint16_t);
			DEBUG("->%i\n",(int)u16);
			if(u16 > LCP_MAX_MRU){
				opt_buf->status = CP_CREQ_NAK;
				return 0;/*TODO: Right value*/
			}
			opt_buf->status = CP_CREQ_ACK;
			break;
		default:
			memcpy(opt_buf->payload, payload, size-2);
			opt_buf->p_size = size-2;
			opt_buf->status = CP_CREQ_REJ;
	}
	return 0;/*TODO: Fix right value */
}
#if 0
static void _lcp_negotiate_nak(opt_stack_t *opts)
{
	/* Iterate through every NAK'd option*/
	uint16_t u16;
	cp_opt_t *curr_opt;
	for(int i=0;i<opts->num_opts;i++)
	{
		curr_opt = (opts->opts+i);
		switch((*curr_opt).type)
		{
			case LCP_OPT_MRU:
				u16 = LCP_DEFAULT_MRU;
				(*curr_opt).payload[0] = (u16 & 0xFF00) >> 8;
				(*curr_opt).payload[1] = u16 & 0x00FF;
				break;
			default:
				/*Shouldn't reach here*/ /*TODO: Assert?*/
				break;
		}
	}
}
#endif



/*Add hdlc header to pkt, send*/
void ppp_send(ppp_dev_t *dev, gnrc_pktsnip_t *pkt)
{
	// Pkt could be network layer type (NETIF->IPv6) or PPP Control Protocol
	(void) dev;
	(void) pkt;
}


void _pktsnd_upper_layer(ppp_dev_t *dev, gnrc_pktsnip_t *pkt)
{
	(void) dev;
	(void) pkt;
}

/*Wake up events for packet reception goes here*/
static int _ppp_recv_pkt(ppp_dev_t *dev, gnrc_pktsnip_t *pkt)
{
	/* Mark hdlc header */
	gnrc_pktsnip_t *result = gnrc_pktbuf_mark(pkt, sizeof(hdlc_hdr_t), GNRC_NETTYPE_UNDEF);
	if (!result) {
		DEBUG("gnrc_ppp: no space left in packet buffer\n");
		return 0; /*TODO:Fix right value */	
	}
	
	hdlc_hdr_t *hdlc_hdr = (hdlc_hdr_t*) result->data;

	cp_pkt_t cp_pkt;
	/* Route the packet according to prot(ocol */
	switch(hdlc_hdr->protocol)
	{
		case PPPTYPE_IPV4:
			/* Check if NCP is up. Silently discard pkt if not */
			if (!dev->l_ncp->up)
			{
				break;
			}
			_pktsnd_upper_layer(dev, &cp_pkt);
			goto dont_discard;
			break;
		case PPPTYPE_LCP:
			/* Populate received pkt */
			_populate_recv_pkt(pkt, &cp_pkt);
			_handle_cp_pkt(dev->l_lcp, &cp_pkt);
			break;
		case PPPTYPE_NCP_IPV4:
			_populate_recv_pkt(pkt, &cp_pkt);
			_handle_cp_pkt(dev->l_ncp, &cp_pkt);
			break;
		default:
			break;
	}

		pkt = result;
		gnrc_pktbuf_release(pkt);
		return 0; /*TODO:Fix right value */

	dont_discard:
		return 0;/*TODO: Fix right value */
}
/* Used for unittest */
void test_handle_cp_rcr(ppp_cp_t *l_lcp, gnrc_pktsnip_t *pkt)
{
	DEBUG("Testing _handle_cp_rcr\n");
	_handle_cp_rcr(l_lcp, pkt);
}
#if 0
void test_ppp_recv_pkt(ppp_dev_t *dev, gnrc_pktsnip_t *pkt)
{
	_ppp_recv_pkt(dev, pkt);
}

/**
 * @brief   Startup code and event loop of the PPP layer
 *
 * @param[in] args          expects a pointer to the underlying netdev device
 *
 * @return                  never returns
 */
static void *_ppp_thread(void *args)
{
    gnrc_netdev2_t *dev = (gnrc_netdev2_t *) args;
    gnrc_netapi_opt_t *opt;

    int res;
    msg_t msg, reply, msg_queue[GNRC_PPP_MSG_QUEUE_SIZE];

    /* setup the MAC layers message queue */
    msg_init_queue(msg_queue, GNRC_PPP_MSG_QUEUE_SIZE);
    /* save the PID to the device descriptor and register the device */
    dev->pid = thread_getpid();

    gnrc_netif_add(dev->mac_pid);
    /* register the event callback with the device driver */
	//Check this...
    dev->driver->add_event_callback(dev, _event_cb);

    /* start the event loop */
    while (1) {
        DEBUG("ppp: waiting for incoming messages\n");
        msg_receive(&msg);
        /* dispatch NETDEV and NETAPI messages */
		/* Used for getting events to NCP layers*/
        switch (msg.type) {
            case GNRC_NETDEV_MSG_TYPE_EVENT:
                DEBUG("ppp: GNRC_NETDEV_MSG_TYPE_EVENT received\n");
				_ppp_recv_pkt(dev, (gnrc_pktsnit_t *)msg.content.ptr);
                break;
            case GNRC_NETAPI_MSG_TYPE_SND:
                DEBUG("ppp: GNRC_NETAPI_MSG_TYPE_SND received\n");
				_ppp_snd_pkt(dev, (gnrc_pktsnit_t *)msg.content.ptr);
                break;
            case GNRC_NETAPI_MSG_TYPE_SET:
                DEBUG("ppp: GNRC_NETAPI_MSG_TYPE_SET received\n");
                /* read incoming options */
                opt = (gnrc_netapi_opt_t *)msg.content.ptr;
                /* set option for device driver */
                res = dev->driver->set(dev, opt->opt, opt->data, opt->data_len);
                DEBUG("ppp: response of netdev->set: %i\n", res);
                /* send reply to calling thread */
                reply.type = GNRC_NETAPI_MSG_TYPE_ACK;
                reply.content.value = (uint32_t)res;
                msg_reply(&msg, &reply);
                break;
            case GNRC_NETAPI_MSG_TYPE_GET:
                DEBUG("ppp: GNRC_NETAPI_MSG_TYPE_GET received\n");
                /* read incoming options */
                opt = (gnrc_netapi_opt_t *)msg.content.ptr;
                /* get option from device driver */
                res = dev->driver->get(dev, opt->opt, opt->data, opt->data_len);
                DEBUG("ppp: response of netdev->get: %i\n", res);
                /* send reply to calling thread */
                reply.type = GNRC_NETAPI_MSG_TYPE_ACK;
                reply.content.value = (uint32_t)res;
                msg_reply(&msg, &reply);
                break;
			case PPP_MSG_TIMEOUT:
				break;
            default:
                DEBUG("ppp: Unknown command %" PRIu16 "\n", msg.type);
                break;
        }
    }
    /* never reached */
    return NULL;
}

kernel_pid_t gnrc_ppp_init(char *stack, int stacksize, char priority,
                             const char *name, gnrc_netdev_t *dev)
{
    kernel_pid_t res;

    /* check if given netdev device is defined and the driver is set */
    if (dev == NULL || dev->driver == NULL) {
        return -ENODEV;
    }
    /* create new PPP thread */
    res = thread_create(stack, stacksize, priority, THREAD_CREATE_STACKTEST,
                        _ppp_thread, (void *)dev, name);
    if (res <= 0) {
        return -EINVAL;
    }
    return res;
}
#endif
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
