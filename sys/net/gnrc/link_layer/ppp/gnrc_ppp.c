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
                break;
			case GNRC_PPP_
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

/* Generate PPP pkt */
static gnrc_pktsnip_t * ppp_pkt_build(uint8_t type, uint8_t id, uint8_t *data, size_t size))
{
	ppp_hdr_t ppp_hdr;
	ppp_hdr_set_type(&ppp_hdr, type);
	int pkt_length = size+sizeof(ppp_hdr_t);
	ppp_hdr_set_length(&ppp_hdr, pkt_length);

	gnrc_pktsnip_t *pkt = gnrc_pktbuf_add(NULL, NULL, pkt_length, GNRC_NETTYPE_PPP);
	memcpy(pkt->data, (uint8_t*) &ppp_hdr, sizeof(ppp_hdr_t));
	memcpy(pkt->data+sizeof(ppp_hdr_t), data, size);

	return pkt;
}

static int gnrc_ppp_send(gnrc_pktsnip_t *pkt, uint8_t protocol)
{
	gnrc_pktsnip_t *hdr = gnrc_pktbuf_add(pkt, NULL, sizeof(hdlc_hdr_t), GNRC_NETTYPE_HDLC);
	hdlc_hdr_t *hdlc_hdr;

	hdlc_set_address(hdlc_hdr, PPP_HDLC_ADDRESS);
	hdlc_set_control(hdlc_hdr, PPP_HDLC_CONTROL);
	hdlc_set_protocol(hdlc_hdr, protocol);

	/* Send to driver...*/

}
