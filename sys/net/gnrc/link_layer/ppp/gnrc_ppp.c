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

#define ENABLE_DEBUG    (0)
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

static int _rx_lcp_conf_req(ppp_ctrl_prot_t l_lcp, gnrc_pktsnip_t *pkt)
{
	/* Get payload length */
	uint8_t identifier = pkt->data[1];

	/*Set the remote identifier*/
	/*TODO*/

	uint16_t length = (pkt->data[2] << 8) + pkt->data[3];
	
	if (length != pkt->size)
	{
		return;
	}
	
	uint8_t *opts = NULL;
	/*If there are options, default values are used... implementation must support default values*/
	if (length == PPP_CP_HDR_BASE_SIZE)
	{
		/* Good request config received*/
		l_lcp->event = E_RCRp;
		return;	
	}

	_opts = (uint8_t*) (pkt->data+PPP_CP_HDR_BASE_SIZE);
	/*Start iterating over options */
	uint16_t cursor = 0;
	
	/* For now, only MRU is implemented */
	uint8_t curr_type, curr_len;
	while(cursor < length-PPP_CP_HDR_BASE_SIZE)
	{
		/* Read current option type */
		curr_type = *(_opts+cursor);
		curr_len = *(_opts+cursor+1);

	}
}

static void ppp_send(ppp_dev *dev, gnrc_pktsnip_t *pkt)
{
	/* Pkt could be network layer type (NETIF->IPv6) or PPP Control Protocol*/
}

static void _handle_lcp_pkt(ppp_ctrl_prot_t *l_lcp, gnrc_pktsnip_t *pkt)
{
	/*LCP type*/
	int type = (int) pkt->data[0];	
	
	switch(type){
		case PPP_CONF_REQ:
			_rx_lcp_conf_req(l_lcp, pkt);
			break;
		case PPP_CONF_ACK:
			break;
		case PPP_CONF_NAK:
			break;
		case PPP_CONF_REJ:
			break;
		case PPP_TERM_REQ:
			break;
		case PPP_TERM_ACK:
			break;
		case PPP_CODE_REJ:
			break;
		case PPP_PROT_REJ:
			break;
		case PPP_ECHO_REQ:
			break;
		case PPP_ECHO_REP:
			break;
		case PPP_DISC_REQ:
			break;
		case PPP_IDENT:
			break;
		case PPP_TIME_REM:
			break;
	}
}
/*Wake up events for packet reception goes here*/
static int _ppp_recv_pkt(ppp_dev *dev, gnrc_pktsnip_t *pkt)
{
	/* Mark hdlc header */
	gnrc_pktsnip_t *hdlc_hdr = gnrc_pktbuf_mark(pkt, sizeof(hdlc_hdr_t), GNRC_NETTYPE_UNDEF);
	if (!hdlc_hdr) {
		DEBUG("gnrc_ppp: no space left in packet buffer\n");
		return;	
	}
	
	/* Route the packet according to protocol */
	switch(hdlc_hdr->protocol)
	{
		case PPPTYPE_IPV4:
			/* Check if NCP is up. Silently discard pkt if not */
			if (!dev->l_ncp->up)
			{
				pkt = hdlc_hdr;
				goto safe_out
			}
			_pktsnd_upper_layer(dev, pkt);
			break;
		case PPPTYPE_LCP:
			_handle_lcp_pkt(dev->l_lcp, pkt);
			break;
		case PPPTYPE_NCP:
			_handle_ncp_pkt(dev->l_ncp, pkt);
			break;
		default:
			/* Silently discard */
			break;
	}

	out:
		return;
	safe_out:
		gnrc_pktbuf_release(pkt);
		return;
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
                /* TODO: filter out MAC layer options -> for now forward
                         everything to the device driver */
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
                /* TODO: filter out MAC layer options -> for now forward
                         everything to the device driver */
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
