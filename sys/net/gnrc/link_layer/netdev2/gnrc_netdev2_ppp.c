/*
 * Copyright (C) 2015 José Ignacio Alamos <jialamos@uc.cl>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 * @ingroup     net
 * @file
 * @brief       gnrc netdev2 ppp glue code
 *
 * @author      José Ignacio Alamos <jialamos@uc.cl>
 * @}
 */

#include "net/gnrc.h"
#include "net/gnrc/netdev2.h"
#include "net/hdlc/hdr.h"
#include "net/hdlctype.h"

#ifdef MODULE_GNRC_IPV6
#include "net/ipv6/hdr.h"
#endif

#include "od.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

/* Receive PPP packet. Send back only payload */
static gnrc_pktsnip_t *_recv(gnrc_netdev2_t *gnrc_netdev2)
{
    netdev2_t *dev = gnrc_netdev2->dev;
	/* we need to know the number of bytes expected. */
	/* -> call driver->recv with second parameter NULL */
    int bytes_expected = dev->driver->recv(dev, NULL, 0, NULL);

	/* Packet to be received */
    gnrc_pktsnip_t *pkt = NULL;

	/* Create packet */
    if (bytes_expected) {
        pkt = gnrc_pktbuf_add(NULL, NULL,
                bytes_expected,
                GNRC_NETTYPE_UNDEF);

        if(!pkt) {
            DEBUG("_recv_ppp_packet: cannot allocate pktsnip.\n");
            goto out;
        }

		/* Get the actual number of bytes read */
        int nread = dev->driver->recv(dev, pkt->data, bytes_expected, NULL);
        if(nread <= 0) {
            DEBUG("_recv_ppp_packet: read error.\n");
            goto safe_out;
        }

        if (nread < bytes_expected) {
            /* we've got less then the expected packet size,
             * so free the unused space.*/

            DEBUG("_recv_ppp_packet: reallocating.\n");
            gnrc_pktbuf_realloc_data(pkt, nread);
        }

        /* mark hdlc header */
        gnrc_pktsnip_t *hdlc_hdr = gnrc_pktbuf_mark(pkt, sizeof(hdlc_hdr_t), GNRC_NETTYPE_UNDEF);
        if (!hdlc_hdr) {
            DEBUG("gnrc_netdev2_ppp: no space left in packet buffer\n");
            goto safe_out;
        }

		/* HDLC header (Address + Protocol) */
        hdlc_hdr_t *hdr = (hdlc_hdr_t *)hdlc_hdr->data;

		/* if HDLC address is not PPP...*/
		if (hdlc_hdr->address != HDLTYPE_PPP){
			DEBUG("gnrc_netdev2_ppp: HDLC frame is not PPP!\n");
			gnrc_pktbuf_release(hdlc_hdr);
			goto safe_out;
		}

        DEBUG("gnrc_netdev2_ppp: received PPP packet")

#if defined(MODULE_OD) && ENABLE_DEBUG
        od_hex_dump(hdr, nread, OD_WIDTH_DEFAULT);
#endif

		/* Don't remove hdlc_hdr. In case of Address and Control Supression, PPP module needs to handle that */
		goto out;
    }

out:
    return pkt;

safe_out:
    gnrc_pktbuf_release(pkt);
    return NULL;
}

static int _send(gnrc_netdev2_t *gnrc_netdev2, gnrc_pktsnip_t *pkt)
{
    hdlc_hdr_t hdr;
    netdev2_t *dev = gnrc_netdev2->dev;

    if (pkt == NULL) {
        DEBUG("gnrc_netdev2_ppp: pkt was NULL");
        return -EINVAL;
    }

	/* Check if pkt type is PPP*/
    if (pkt->type != GNRC_NETTYPE_PPP) {
        DEBUG("gnrc_netdev2_eth: First header was not PPP\n");
        return -EBADMSG;
    }

	/*At this point there's an HDLC frame without FCS and flag.*/
	/*|--Address--|--Control--|--Protocol--|--Information--|--Padding--| */
    size_t n;
    pkt = gnrc_pktbuf_get_iovec(pkt, &n);
    struct iovec *vector = (struct iovec *)pkt->data;

    dev->driver->send(dev, vector, n);
    DEBUG("gnrc_netdev2_ppp: Packet sent");

    gnrc_pktbuf_release(pkt);

    return 0;
}

int gnrc_netdev2_ppp_init(gnrc_netdev2_t *gnrc_netdev2, netdev2_t *dev)
{
    gnrc_netdev2->send = _send;
    gnrc_netdev2->recv = _recv;
    gnrc_netdev2->dev = dev;

    return 0;
}
