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
#include "net/gnrc/ppp/prot.h"
#include "net/gnrc/ppp/lcp.h"
#include "net/gnrc/ppp/pap.h"
#include "net/gnrc/ppp/ipcp.h"
#include "sys/uio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GNRC_PPP_MSG_QUEUE 64

#define PPP_HDLC_ADDRESS (0xFF)
#define PPP_HDLC_CONTROL (0x03)


#define AUTH_PAP (1)

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


#define ID_UNDEF (0)
#define ID_LCP (1)
#define ID_IPCP (2)
#define ID_IPV4 (3)
#define ID_PAP (4)
#define ID_PPPDEV (0xee)

#define BROADCAST_LCP (0xff)
#define BROADCAST_NCP (0xfe)


#define GNRC_PPP_MSG_QUEUE_SIZE (20)


#define OPT_ENABLED (1)
#define OPT_REQUIRED (2)

/*TODO: Of course, change!*/
#define PPPDEV_MSG_TYPE_EVENT (100)
#define GNRC_PPPDEV_MSG_TYPE_EVENT (101)

#define PPPDEV_LINK_DOWN_EVENT (10)

#define DCP_MONITOR_INIT_DELAY (15000000)
#define DCP_MONITOR_TIMEOUT (10000000)
#define DCP_DEAD_COUNTER (5)

typedef enum
{
	PPP_LINKUP,
	PPP_RECV,
	PPP_TIMEOUT,
	PPP_LINKDOWN,
	PPP_UL_STARTED,
	PPP_UL_FINISHED,
	PPP_MONITOR,
	PPP_LINK_ALIVE,
	PPP_DIALUP
} ppp_dev_event_t;

typedef enum
{
	PPP_LINK_DEAD,
	PPP_LINK_ESTABLISHED,
	PPP_AUTHENTICATION,
	PPP_NETWORK,
	PPP_TERMINATION
} ppp_state_t;

typedef struct pppdev_t pppdev_t;
typedef struct pppdev_driver_t
{
	int (*send)(pppdev_t *dev, const struct iovec *vector, int count);
	int (*recv)(pppdev_t *dev, char *buf, int len, void *info);
	void (*driver_ev)(pppdev_t *dev, uint8_t event);
	int (*init)(pppdev_t *dev);
	int (*set)(pppdev_t *dev, netopt_t opt, void *value, size_t value_len);
	int (*get)(pppdev_t *dev, netopt_t opt, void *value, size_t max_len);
	int (*dial_up)(pppdev_t *dev);
	int (*link_down)(pppdev_t *dev);
} pppdev_driver_t;

typedef struct pppdev_t
{
	const pppdev_driver_t *driver;
} pppdev_t;


typedef struct dcp_t
{
	ppp_protocol_t prot;
	msg_t timer_msg;
	xtimer_t xtimer;
	uint8_t dead_counter;
} dcp_t;


/* PPP device */
typedef struct gnrc_pppdev_t{
	dcp_t l_dcp;
	lcp_t l_lcp;
	ipcp_t l_ipcp;
	ppp_ipv4_t l_ipv4;
	pap_t l_pap;
	pppdev_t *netdev;

	uint8_t state;
} gnrc_pppdev_t;



int gnrc_ppp_setup(gnrc_pppdev_t *dev, pppdev_t *netdev);
gnrc_pktsnip_t *pkt_build(gnrc_nettype_t pkt_type, uint8_t code, uint8_t id, gnrc_pktsnip_t *payload);
int gnrc_ppp_send(gnrc_pppdev_t *dev, gnrc_pktsnip_t *pkt);
int gnrc_ppp_recv(gnrc_pppdev_t *dev, gnrc_pktsnip_t *pkt);
int gnrc_ppp_event_callback(gnrc_pppdev_t *dev, int ppp_event);


void print_pkt(gnrc_pktsnip_t *hdlc, gnrc_pktsnip_t *ppp, gnrc_pktsnip_t *pkt);
int _pkt_get_ppp_header(gnrc_pktsnip_t *pkt, ppp_hdr_t **ppp_hdr);

void *_gnrc_ppp_thread(void *args);
int fsm_handle_ppp_msg(struct ppp_protocol_t *protocol, uint8_t ppp_event, void *args); 
int dcp_init(gnrc_pppdev_t *ppp_dev, ppp_protocol_t *dcp);

void gnrc_ppp_link_up(msg_t *msg, kernel_pid_t pid);
void gnrc_ppp_link_down(msg_t *msg, kernel_pid_t pid);
void gnrc_ppp_dispatch_pkt(msg_t *msg, kernel_pid_t pid);
void gnrc_ppp_dial_up(msg_t *msg, kernel_pid_t pid);
int gnrc_ppp_set_opt(gnrc_pppdev_t *dev, netopt_t opt, void *value, size_t value_len);

kernel_pid_t gnrc_pppdev_init(char *stack, int stacksize, char priority,
                        const char *name, gnrc_pppdev_t *gnrc_pppdev);

void send_configure_request(gnrc_pppdev_t *dev, gnrc_nettype_t protocol, uint8_t id, gnrc_pktsnip_t *payload);
void send_configure_ack(gnrc_pppdev_t *dev, gnrc_nettype_t protocol, uint8_t id, gnrc_pktsnip_t *opts);
void send_configure_nak(gnrc_pppdev_t *dev, gnrc_nettype_t protocol, uint8_t id, gnrc_pktsnip_t *opts);
void send_configure_rej(gnrc_pppdev_t *dev, gnrc_nettype_t protocol, uint8_t id, gnrc_pktsnip_t *opts);
void send_terminate_req(gnrc_pppdev_t *dev, gnrc_nettype_t protocol, uint8_t id);
void send_terminate_ack(gnrc_pppdev_t *dev, gnrc_nettype_t protocol, uint8_t id, gnrc_pktsnip_t *response);
void send_code_rej(gnrc_pppdev_t *dev, gnrc_nettype_t protocol, uint8_t id, gnrc_pktsnip_t *rejected);
void send_echo_reply(gnrc_pppdev_t *dev, gnrc_nettype_t protocol, uint8_t id, gnrc_pktsnip_t *data);
void send_protocol_reject(gnrc_pppdev_t *dev, uint8_t id, gnrc_pktsnip_t *pkt);
void send_pap_request(gnrc_pppdev_t *dev, uint8_t id, gnrc_pktsnip_t *credentials);
#ifdef __cplusplus
}
#endif

#endif /* GNRC_PPP_H_ */
/** @} */
