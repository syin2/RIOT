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
#endif
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
static int _parse_cp_options(opt_stack_t *o_stack, uint8_t *payload, size_t p_size)
{
	o_stack->num_opts = 0;

	/*Start iterating over options */
	uint16_t cursor = 0;
	
	uint8_t curr_type, curr_len;
	uint8_t curr_status;

	/* Current option status*/
	cp_opt_t curr_opt_status;

	o_stack->content_flag=0;

	/* TODO: Check default value (no opts sent)*/
	while(cursor < p_size) {
		/* Read current option type */
		curr_type = *(payload+cursor);
		curr_len = *(payload+cursor+1);
		
		/* TODO: If cursor + len > total_length, discard pkt*/

		_read_lcp_pkt(curr_type, payload+cursor+2, (size_t) curr_len, &curr_opt_status);
		curr_status =curr_opt_status.status;

		DEBUG("Current status: %i\n",curr_status);

		o_stack->content_flag |= 1<<curr_status;

		o_stack->_opt_buf[o_stack->num_opts] = curr_opt_status;
		o_stack->num_opts+=1;
		cursor = cursor + curr_len;
	}

	return 0; /*TODO: Check return*/
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
static int _handle_cp_rcr(ppp_cp_t *l_lcp, gnrc_pktsnip_t *pkt)
{
	/* Get payload length */
	uint8_t *data = (uint8_t*) pkt->data;
	//uint8_t identifier = *(data+1); /*TODO: Change to OFFSET*/

	/*Set the remote identifier*/
	/*TODO*/

	uint16_t length = (*(data+2) << 8) + *(data+3); /*TODO: Change to OFFSET*/
	
	if (length != pkt->size) {
		/* TODO: Error code*/
		return 0;
	}
	
	DEBUG("gnrc_ppp: CP: Length of whole packet is %i \n", (int) length);
	int status = _parse_cp_options(&(l_lcp->outgoing_opts), data+PPP_CP_HDR_BASE_SIZE,(size_t) (length-PPP_CP_HDR_BASE_SIZE));

	if(status == 1000)
	{
		return 100;/*TODO: Fix error code*/
	}


	/* At this point, we have the responses and type of responses. Process each one*/
	if (l_lcp->outgoing_opts.content_flag & (OPT_HAS_NAK | OPT_HAS_REJ))
	{
		l_lcp->event = E_RCRm;
	}
	else
	{
		l_lcp->event = E_RCRp;
	}

return 0; /*TODO: Fix output*/
}
#if 0
/*Add hdlc header to pkt, send*/
void ppp_send(ppp_dev_t *dev, gnrc_pktsnip_t *pkt)
{
	// Pkt could be network layer type (NETIF->IPv6) or PPP Control Protocol
	(void) dev;
	(void) pkt;
}

static void _handle_pkt_lcp(ppp_cp_t *l_lcp, gnrc_pktsnip_t *pkt)
{
	/*LCP type*/
	uint8_t *data = (uint8_t*) pkt->data;
	int type = (int) *(data);	
	
	switch(type){
		case PPP_CONF_REQ:
			_handle_cp_rcr(l_lcp, pkt);
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

static void _handle_pkt_ncp(ppp_cp_t *l_lcp, gnrc_pktsnip_t *pkt)
{
	(void) l_lcp;
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

	/* Route the packet according to protocol */
	switch(hdlc_hdr->protocol)
	{
		case PPPTYPE_IPV4:
			/* Check if NCP is up. Silently discard pkt if not */
			if (!dev->l_ncp->up)
			{
				break;
			}
			_pktsnd_upper_layer(dev, pkt);
			goto dont_discard;
			break;
		case PPPTYPE_LCP:
			_handle_pkt_lcp(dev->l_lcp, pkt);
			break;
		case PPPTYPE_NCP_IPV4:
			_handle_pkt_ncp(dev->l_ncp, pkt);
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
#endif
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
