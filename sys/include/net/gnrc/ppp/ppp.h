/*
 * Copyright (C) 2015 Martine Lenders <mlenders@inf.fu-berlin.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    net_gnrc_slip SLIP
 * @ingroup     net_gnrc
 * @brief       Provides a SLIP interface over UART utilizing
 *              @ref drivers_periph_uart.
 * @{
 *
 * @file
 * @brief       SLIP interface defintion
 *
 * @author      Martine Lenders <mlenders@inf.fu-berlin.de>
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 */

#ifndef GNRC_PPP_H_
#define GNRC_PPP_H_

#include <inttypes.h>

#include "net/gnrc.h"
#include "net/ppp/hdr.h"
#include "net/gnrc/pkt.h"
#include "net/gnrc/pktbuf.h"
#include "xtimer.h"
#include "thread.h"
#include "net/gnrc/ppp/opt.h"
#include "sys/uio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PPPOPT_ACCM_RX (1)
#define PPPOPT_ACCM_TX (2)

#define GNRC_PPP_MSG_QUEUE 64

#define PPP_HDLC_ADDRESS (0xFF)
#define PPP_HDLC_CONTROL (0x03)


#define PPP_CONF_REQ (1)
#define PPP_CONF_ACK (2)
#define PPP_CONF_NAK (3)
#define PPP_CONF_REJ (4)
#define PPP_TERM_REQ (5)
#define PPP_TERM_ACK (6)
#define PPP_CODE_REJ (7)
#define PPP_PROT_REJ (8)
#define PPP_ECHO_REQ (9)
#define PPP_ECHO_REP (10)
#define PPP_DISC_REQ (11)
#define PPP_IDENT (12)
#define PPP_TIME_REM (13)
#define PPP_UNKNOWN_CODE (0)


#define FLAG_CONF_REQ (1<<0)
#define FLAG_CONF_ACK (1<<1)
#define FLAG_CONF_NAK (1<<2)
#define FLAG_CONF_REJ (1<<3)
#define FLAG_TERM_REQ (1<<4)
#define FLAG_TERM_ACK (1<<5)
#define FLAG_CODE_REJ (1<<6)
#define FLAG_PROT_REJ (1<<7)
#define FLAG_ECHO_REQ (1<<8)
#define FLAG_ECHO_REP (1<<9)
#define FLAG_DISC_REQ (1<<10)
#define FLAG_IDENT (1<<11)
#define FLAG_TIME_REM (1<<12)


#define ID_LCP (1)
#define ID_IPCP (2)
#define ID_IPV4 (3)
#define ID_PPPDEV (0xee)

#define BROADCAST_LCP (0xff)
#define BROADCAST_NCP (0xfe)


#define GNRC_PPP_MSG_QUEUE_SIZE (20)

#define OPT_PAYLOAD_BUF_SIZE (100)

#define OPT_ENABLED (1)
#define OPT_REQUIRED (2)

/*TODO: Of course, change!*/
#define PPPDEV_MSG_TYPE_EVENT (100)

typedef enum
{
	PPP_LINKUP,
	PPP_RECV,
	PPP_TIMEOUT,
	PPP_LINKDOWN,
	PPP_UL_STARTED,
	PPP_UL_FINISHED
} ppp_dev_event_t;

typedef enum
{
	PPP_LINK_DEAD,
	PPP_LINK_ESTABLISHED,
	PPP_AUTHENTICATION,
	PPP_NETWORK,
	PPP_TERMINATION
} ppp_state_t;

typedef struct ppp_protocol_t
{
	int (*handler)(struct ppp_protocol_t *protocol, uint8_t ppp_event, void *args);
	uint8_t id;
} ppp_protocol_t;


typedef struct pppdev_t pppdev_t;
typedef struct pppdev_driver_t
{
	int (*send)(pppdev_t *dev, const struct iovec *vector, int count);
	int (*recv)(pppdev_t *dev, char *buf, int len, void *info);
	void (*driver_ev)(pppdev_t *dev, uint8_t event);
	int (*init)(pppdev_t *dev);
	int (*set)(pppdev_t *dev, uint8_t opt, void *value, size_t value_len);
} pppdev_driver_t;

typedef struct pppdev_t
{
	pppdev_driver_t *driver;
} pppdev_t;



/* PPP device */
typedef struct gnrc_pppdev_t{
	ppp_protocol_t *l_lcp;
	ppp_protocol_t *l_ipcp;
	ppp_protocol_t *l_ipv4;
	pppdev_t *netdev;

	uint8_t state;
} gnrc_pppdev_t;


int gnrc_ppp_init(gnrc_pppdev_t *dev, pppdev_t *netdev);
gnrc_pktsnip_t *pkt_build(gnrc_nettype_t pkt_type, uint8_t code, uint8_t id, gnrc_pktsnip_t *payload);
int gnrc_ppp_send(gnrc_pppdev_t *dev, gnrc_pktsnip_t *pkt);
int gnrc_ppp_recv(gnrc_pppdev_t *dev, gnrc_pktsnip_t *pkt);
int gnrc_ppp_event_callback(gnrc_pppdev_t *dev, int ppp_event);


void print_pkt(gnrc_pktsnip_t *hdlc, gnrc_pktsnip_t *ppp, gnrc_pktsnip_t *pkt);
int _pkt_get_ppp_header(gnrc_pktsnip_t *pkt, ppp_hdr_t **ppp_hdr);

void *gnrc_ppp_thread(void *args);
int fsm_handle_ppp_msg(struct ppp_protocol_t *protocol, uint8_t ppp_event, void *args); 
#ifdef __cplusplus
}
#endif

#endif /* GNRC_PPP_H_ */
/** @} */
