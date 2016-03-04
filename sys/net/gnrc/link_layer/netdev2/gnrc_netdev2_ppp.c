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

        /* set payload type for HDLC frame (from now, PPP frame) -> LDC, NCP, etc */
        pkt->type = gnrc_nettype_from_pppprotocol(byteorder_ntohs(hdr->protocol));

        DEBUG("gnrc_netdev2_ppp: received PPP packet")

#if defined(MODULE_OD) && ENABLE_DEBUG
        od_hex_dump(hdr, nread, OD_WIDTH_DEFAULT);
#endif

		/* Remove hdlc header from packet */
        gnrc_pktbuf_remove_snip(pkt, hdlc_hdr);
		goto out;
    }

out:
    return pkt;

safe_out:
    gnrc_pktbuf_release(pkt);
    return NULL;
}

static inline void _addr_set_broadcast(uint8_t *dst)
{
    memset(dst, 0xff, ETHERNET_ADDR_LEN);
}

static inline void _addr_set_multicast(uint8_t *dst, gnrc_pktsnip_t *payload)
{
    switch (payload->type) {
#ifdef MODULE_GNRC_IPV6
        case GNRC_NETTYPE_IPV6:
            /* https://tools.ietf.org/html/rfc2464#section-7 */
            dst[0] = 0x33;
            dst[1] = 0x33;
            ipv6_hdr_t *ipv6 = payload->data;
            memcpy(dst + 2, ipv6->dst.u8 + 12, 4);
            break;
#endif
        default:
            _addr_set_broadcast(dst);
            break;
    }
}

static int _send(gnrc_netdev2_t *gnrc_netdev2, gnrc_pktsnip_t *pkt)
{
    hdlc_hdr_t hdr;
    gnrc_pktsnip_t *payload;

    netdev2_t *dev = gnrc_netdev2->dev;

    if (pkt == NULL) {
        DEBUG("gnrc_netdev2_ppp: pkt was NULL");
        return -EINVAL;
    }

    payload = pkt->next;

    if (gnrc_nettype_is_ppp(pkt->type)) {
        DEBUG("gnrc_netdev2_eth: First header was not PPP\n");
        return -EBADMSG;
    }

    if (payload) {
        hdr.type = byteorder_htons(gnrc_nettype_to_ethertype(payload->type));
    }
    else {
        hdr.type = byteorder_htons(ETHERTYPE_UNKNOWN);
    }

    netif_hdr = pkt->data;

    /* set ppp header */
    if (netif_hdr->src_l2addr_len == ETHERNET_ADDR_LEN) {
        memcpy(hdr.dst, gnrc_netif_hdr_get_src_addr(netif_hdr),
               netif_hdr->src_l2addr_len);
    }
    else {
        dev->driver->get(dev, NETOPT_ADDRESS, hdr.src, ETHERNET_ADDR_LEN);
    }

    if (netif_hdr->flags & GNRC_NETIF_HDR_FLAGS_BROADCAST) {
        _addr_set_broadcast(hdr.dst);
    }
    else if (netif_hdr->flags & GNRC_NETIF_HDR_FLAGS_MULTICAST) {
        if (payload == NULL) {
            DEBUG("gnrc_netdev2_eth: empty multicast packets over Ethernet "\
                  "are not yet supported\n");
            return -ENOTSUP;
        }
        _addr_set_multicast(hdr.dst, payload);
    }
    else if (netif_hdr->dst_l2addr_len == ETHERNET_ADDR_LEN) {
        memcpy(hdr.dst, gnrc_netif_hdr_get_dst_addr(netif_hdr),
               ETHERNET_ADDR_LEN);
    }
    else {
        DEBUG("gnrc_netdev2_eth: destination address had unexpected format\n");
        return -EBADMSG;
    }

    DEBUG("gnrc_netdev2_eth: send to %02x:%02x:%02x:%02x:%02x:%02x\n",
          hdr.dst[0], hdr.dst[1], hdr.dst[2],
          hdr.dst[3], hdr.dst[4], hdr.dst[5]);

    size_t n;
    pkt = gnrc_pktbuf_get_iovec(pkt, &n);
    struct iovec *vector = (struct iovec *)pkt->data;
    vector[0].iov_base = (char*)&hdr;
    vector[0].iov_len = sizeof(ppp_hdr_t);
    dev->driver->send(dev, vector, n);

    gnrc_pktbuf_release(pkt);

    return 0;
}

int gnrc_netdev2_eth_init(gnrc_netdev2_t *gnrc_netdev2, netdev2_t *dev)
{
    gnrc_netdev2->send = _send;
    gnrc_netdev2->recv = _recv;
    gnrc_netdev2->dev = dev;

    return 0;
}
