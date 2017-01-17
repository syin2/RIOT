/*
 * Copyright (C) 2016 José Ignacio Alamos <jialamos@uc.cl>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 * @ingroup     net_gnrc_ppp
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
#include <errno.h>
#include <string.h>
#include "byteorder.h"

#define ENABLE_DEBUG    (0)
#include "debug.h"

#if ENABLE_DEBUG
/* For PRIu16 etc. */
#include <inttypes.h>
#endif


void send_ppp_event(msg_t *msg, ppp_msg_t ppp_msg)
{
    msg->type = GNRC_PPP_MSG_TYPE_EVENT;
    msg->content.value = ppp_msg;
    msg_send(msg, thread_getpid());
}

void send_ppp_event_xtimer(msg_t *msg, xtimer_t *xtimer, ppp_msg_t ppp_msg, int timeout)
{
    msg->type = GNRC_PPP_MSG_TYPE_EVENT;
    msg->content.value = ppp_msg;
    xtimer_remove(xtimer);
    xtimer_set_msg(xtimer, timeout, msg, thread_getpid());
}

/* Generate PPP pkt */
gnrc_pktsnip_t *pkt_build(gnrc_nettype_t pkt_type, uint8_t code, uint8_t id, gnrc_pktsnip_t *payload)
{
    ppp_hdr_t ppp_hdr;

    ppp_hdr.code = code;
    ppp_hdr.id = id;

    int payload_length = payload ? payload->size : 0;
    ppp_hdr.length = byteorder_htons(payload_length + sizeof(ppp_hdr_t));

    gnrc_pktsnip_t *ppp_pkt = gnrc_pktbuf_add(payload, (void *) &ppp_hdr, sizeof(ppp_hdr_t), pkt_type);
    return ppp_pkt;
}

ppp_target_t _get_target_from_protocol(uint16_t protocol)
{
    switch (protocol) {
        case PPPTYPE_LCP:
            return PROT_LCP;
            break;
        case PPPTYPE_NCP_IPV4:
            return PROT_IPCP;
            break;
        case PPPTYPE_IPV4:
            return PROT_IPV4;
        case PPPTYPE_PAP:
            return PROT_AUTH;
        default:
            DEBUG("gnrc_ppp: Received unknown PPP protocol. Discard.\n");
    }
    return PROT_UNDEF;
}
gnrc_pktsnip_t *retrieve_pkt(netdev2_t *dev)
{
    int nbytes;

    nbytes = dev->driver->recv(dev, NULL, 0, NULL);
    gnrc_pktsnip_t *pkt = gnrc_pktbuf_add(NULL, NULL, nbytes, GNRC_NETTYPE_UNDEF);
    dev->driver->recv(dev, pkt->data, nbytes, NULL);
    return pkt;
}

int gnrc_ppp_setup(gnrc_pppdev_t *dev, netdev2_t *netdev)
{
    dev->netdev = (netdev2_ppp_t*) netdev;

	netdev2_ppp_t *pppdev = (netdev2_ppp_t*) netdev;

    dcp_init(dev);
    lcp_init(dev);
    ipcp_init(dev);
    ppp_ipv4_init(dev);
    pap_init(dev);

    trigger_fsm_event((ppp_fsm_t *) &pppdev->lcp, E_OPEN, NULL);
    trigger_fsm_event((ppp_fsm_t *) &pppdev->ipcp, E_OPEN, NULL);
    return 0;
}


int gnrc_ppp_send(gnrc_pppdev_t *dev, gnrc_pktsnip_t *pkt)
{
    hdlc_hdr_t hdlc_hdr;
    netdev2_t *netdev = (netdev2_t*) dev->netdev;
	netdev2_ppp_t *pppdev = (netdev2_ppp_t*) netdev;

    hdlc_hdr_set_address(&hdlc_hdr, PPP_HDLC_ADDRESS);
    hdlc_hdr_set_control(&hdlc_hdr, PPP_HDLC_CONTROL);
    hdlc_hdr_set_protocol(&hdlc_hdr, gnrc_nettype_to_ppp_protnum(pkt->type));

    gnrc_pktsnip_t *hdr = gnrc_pktbuf_add(pkt, (void *) &hdlc_hdr, sizeof(hdlc_hdr_t), GNRC_NETTYPE_HDLC);

    if (gnrc_pkt_len(hdr) > pppdev->lcp.peer_mru) {
        DEBUG("gnrc_ppp: Sending exceeds peer MRU. Dropping packet.\n");
        gnrc_pktbuf_release(hdr);
        return -EBADMSG;
    }
    /* Get iovec representation */
    size_t n;
    int res = -ENOBUFS;
    hdr = gnrc_pktbuf_get_iovec(hdr, &n);
    if (hdr != NULL) {
        struct iovec *vector = (struct iovec *) hdr->data;
        res = netdev->driver->send(netdev, vector, n);
    }
    gnrc_pktbuf_release(hdr);
    return res;
}

static int _pkt_is_ppp(gnrc_pktsnip_t *pkt)
{
    return (pkt->type == PPPTYPE_NCP_IPV4 || pkt->type == PPPTYPE_PAP || pkt->type == PPPTYPE_LCP);
}

static int _ppp_pkt_is_valid(gnrc_pktsnip_t *pkt)
{
    ppp_hdr_t *hdr = pkt->data;

    return byteorder_ntohs(hdr->length) < pkt->size;
}

int _prot_is_allowed(gnrc_pppdev_t *dev, uint16_t protocol)
{
	netdev2_ppp_t *pppdev = (netdev2_ppp_t*) dev->netdev;
    switch (protocol) {
        case PPPTYPE_LCP:
            return ((ppp_protocol_t*) &pppdev->lcp)->state == PROTOCOL_STARTING || ((ppp_protocol_t*) &pppdev->lcp)->state == PROTOCOL_UP;
        case PPPTYPE_NCP_IPV4:
            return ((ppp_protocol_t*) &pppdev->ipcp)->state == PROTOCOL_STARTING || ((ppp_protocol_t*) &pppdev->ipcp)->state == PROTOCOL_UP;
        case PPPTYPE_PAP:
            return ((ppp_protocol_t*) &pppdev->pap)->state == PROTOCOL_STARTING;
        case PPPTYPE_IPV4:
            return ((ppp_protocol_t*) &pppdev->ipv4)->state == PROTOCOL_UP;
    }
    return 0;
}
int dispatch_ppp_msg(gnrc_pppdev_t *dev, ppp_msg_t ppp_msg)
{
    ppp_target_t target = ppp_msg_get_target(ppp_msg);
    ppp_event_t event = ppp_msg_get_event(ppp_msg);
    gnrc_pktsnip_t *pkt = NULL;
    netdev2_t *netdev = (netdev2_t*) dev->netdev;
	netdev2_ppp_t *pppdev = (netdev2_ppp_t*) dev->netdev;

    if (event == PPP_RECV) {
        pkt = retrieve_pkt(netdev);
        gnrc_pktsnip_t *result = gnrc_pktbuf_mark(pkt, sizeof(hdlc_hdr_t), GNRC_NETTYPE_HDLC);
        if (!result) {
            DEBUG("gnrc_ppp: no space left in packet buffer\n");
            return -ENOBUFS;
        }

        /*Drop packet if exceeds MRU*/
        if (gnrc_pkt_len(pkt) > pppdev->lcp.mru) {
            DEBUG("gnrc_ppp: Exceeded MRU of device. Dropping packet.\n");
            gnrc_pktbuf_release(pkt);
            return -EBADMSG;
        }

        hdlc_hdr_t *hdlc_hdr = (hdlc_hdr_t *) result->data;
        target = _get_target_from_protocol(hdlc_hdr_get_protocol(hdlc_hdr));

        if (!target) {
            /* Remove hdlc header */
            network_uint16_t protocol = byteorder_htons(hdlc_hdr_get_protocol(pkt->next->data));
            gnrc_pktbuf_remove_snip(pkt, pkt->next);
            gnrc_pktsnip_t *rp = gnrc_pktbuf_add(pkt, &protocol, 2, GNRC_NETTYPE_UNDEF);
            send_protocol_reject(dev, pppdev->lcp.pr_id++, rp);
            return -EBADMSG;
        }

        if (!_prot_is_allowed(dev, hdlc_hdr_get_protocol(hdlc_hdr))) {
            DEBUG("gnrc_ppp: Received a ppp packet that's not allowed in current ppp state. Discard packet\n");
            gnrc_pktbuf_release(pkt);
            return -1;
        }

        if (_pkt_is_ppp(pkt) && !_ppp_pkt_is_valid(pkt)) {
            gnrc_pktbuf_release(pkt);
            DEBUG("gnrc_ppp: Invalid ppp packet. Discard.\n");
        }

    }


    ppp_protocol_t *target_prot;
    switch (target) {
        case PROT_LCP:
            target_prot = (ppp_protocol_t *) &pppdev->lcp;
            break;
        case PROT_IPCP:
        case BROADCAST_NCP:
            target_prot = (ppp_protocol_t *) &pppdev->ipcp;
            break;
        case PROT_IPV4:
            target_prot = (ppp_protocol_t *) &pppdev->ipv4;
            break;
        case PROT_DCP:
        case BROADCAST_LCP:
            target_prot = (ppp_protocol_t *) &pppdev->dcp;
            break;
        case PROT_AUTH:
            target_prot = (ppp_protocol_t *) &pppdev->pap;
            break;
        default:
            DEBUG("Unrecognized target\n");
            return -1;
            break;
    }

    return target_prot->handler(target_prot, event, pkt);
}

static void _event_cb(netdev2_t *dev, netdev2_event_t event)
{
    gnrc_pppdev_t *gnrc_pppdev = (gnrc_pppdev_t*) dev->context;
    if (event == NETDEV2_EVENT_ISR) {
        msg_t msg;

        msg.type = NETDEV2_MSG_TYPE_EVENT;
        msg.content.ptr = gnrc_pppdev;

        if (msg_send(&msg, gnrc_pppdev->pid) <= 0) {
            puts("gnrc_netdev2: possibly lost interrupt.");
        }
    }
}

void *_gnrc_ppp_thread(void *args)
{
    DEBUG("gnrc_ppp_thread started\n");
    gnrc_pppdev_t *pppdev = (gnrc_pppdev_t *) args;
    pppdev->pid = thread_getpid();
    gnrc_netif_add(thread_getpid());
    netdev2_t *d = (netdev2_t*) pppdev->netdev;
    d->event_callback = _event_cb;
    d->context = pppdev;
    d->driver->init(d);

    msg_t msg_queue[GNRC_PPP_MSG_QUEUE];;
    msg_init_queue(msg_queue, GNRC_PPP_MSG_QUEUE);
    msg_t msg, reply;
    int event, res;
    gnrc_netapi_opt_t *opt;
    while (1) {
        DEBUG("gnrc_ppp: waiting for msg\n");
        msg_receive(&msg);
        event = msg.content.value;
        switch (msg.type) {
            case NETDEV2_MSG_TYPE_EVENT:
                d->driver->isr((netdev2_t *) d);
                break;
            case GNRC_NETAPI_MSG_TYPE_SET:
                opt = (gnrc_netapi_opt_t *) msg.content.ptr;
                res = d->driver->set(d, opt->opt, opt->data, opt->data_len);
                reply.type = GNRC_NETAPI_MSG_TYPE_ACK;
                reply.content.value = (uint32_t) res;
                msg_reply(&msg, &reply);
                break;
            case GNRC_NETAPI_MSG_TYPE_GET:
                opt = (gnrc_netapi_opt_t *) msg.content.ptr;
                res = d->driver->get(d, opt->opt, opt->data, opt->data_len);
                reply.type = GNRC_NETAPI_MSG_TYPE_ACK;
                reply.content.value = (uint32_t) res;
                msg_reply(&msg, &reply);
                break;
            case GNRC_NETAPI_MSG_TYPE_SND:
                ppp_ipv4_send(pppdev, (gnrc_pktsnip_t *) msg.content.ptr);
                break;
            case GNRC_PPP_MSG_TYPE_EVENT:
                dispatch_ppp_msg(pppdev, event);
                break;
            default:
                DEBUG("Received an unknown thread msg: %i\n", msg.type);
        }
    }
}

void gnrc_ppp_trigger_event(msg_t *msg, kernel_pid_t pid, uint8_t target, uint8_t event)
{
    msg->type = GNRC_PPP_MSG_TYPE_EVENT;
    msg->content.value = (target << 8) | (event & 0xffff);
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
    gnrc_ppp_trigger_event(msg, pid, PROT_DCP, PPP_DIALUP);
}

void gnrc_ppp_disconnect(msg_t *msg, kernel_pid_t pid)
{
    gnrc_ppp_trigger_event(msg, pid, PROT_DCP, PPP_LINKDOWN);
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
void ppp_protocol_init(ppp_protocol_t *protocol, struct gnrc_pppdev_t *pppdev, int (*handler)(struct ppp_protocol_t *, uint8_t, void *), uint8_t id)
{
    protocol->handler = handler;
    protocol->id = id;
    protocol->pppdev = pppdev;
    protocol->state = PROTOCOL_DOWN;
}
