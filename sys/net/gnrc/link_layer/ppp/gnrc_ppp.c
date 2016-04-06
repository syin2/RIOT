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
#endif

/*Wake up events for packet reception goes here*/
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
			lcp_handle_pkt(&dev->l_lcp, pkt);
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
gnrc_pktsnip_t * _pkt_build(gnrc_nettype_t pkt_type, uint8_t code, uint8_t id, gnrc_pktsnip_t *payload)
{
	ppp_hdr_t ppp_hdr;
	ppp_hdr_set_code(&ppp_hdr, code);
	ppp_hdr_set_id(&ppp_hdr, id);
	int payload_length = gnrc_pkt_len(payload);
	ppp_hdr_set_length(&ppp_hdr, payload_length + sizeof(ppp_hdr_t));

	gnrc_pktsnip_t *ppp_pkt = gnrc_pktbuf_add(payload, (void*) &ppp_hdr, sizeof(ppp_hdr_t), pkt_type);
	return ppp_pkt;
}

gnrc_pktsnip_t *lcp_pkt_build(uint8_t type, uint8_t id, gnrc_pktsnip_t *payload)
{
	return _pkt_build(GNRC_NETTYPE_LCP, type, id, payload);
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
	int nbytes;
	switch (ppp_event)
	{
		case PPP_RECV:
			nbytes = dev->netdev->driver->recv(dev->netdev, NULL, 0, NULL);
			gnrc_pktsnip_t *pkt = gnrc_pktbuf_add(NULL, NULL, nbytes, GNRC_NETTYPE_UNDEF);
			dev->netdev->driver->recv(dev->netdev, pkt->data, nbytes, NULL);
			gnrc_ppp_recv(dev, pkt);
			break;
		case PPP_LINKUP:
			DEBUG("!!!!");
			trigger_lcp_event(&dev->l_lcp, E_UP, NULL);
			trigger_lcp_event(&dev->l_lcp, E_OPEN, NULL);
			break;
	}
	return 0;
}
