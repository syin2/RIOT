/*
 * Copyright (C) 2015 José Ignacio Alamos <jialamos@uc.cl>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    net_gnrc_ppp gnrc ppp definitions
 * @ingroup     net_gnrc_ppp
 * @{
 *
 * @file
 * @brief  Definitions and interface of gnrc ppp 
 *
 * @author  José Ignacio Alamos <jialamos@uc.cl>
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

#define PPP_HDLC_ADDRESS (0xFF) /**< HDLC address field for PPP */
#define PPP_HDLC_CONTROL (0x03) /**< HDLC control field for PPP */


#define AUTH_PAP (1) /**< Label of PAP authentication */

#define PPP_CONF_REQ (1) /**< Code of Configure Request packet */
#define PPP_CONF_ACK (2) /**< Code of Configure Ack packet */
#define PPP_CONF_NAK (3) /**< Code of Configure NAK packet */
#define PPP_CONF_REJ (4) /**< Code of Configure Reject packet */
#define PPP_TERM_REQ (5) /**< Code of Temrminate Request packet */
#define PPP_TERM_ACK (6) /**< Code of Terminate ACK packet */
#define PPP_CODE_REJ (7) /**< Code of Code Reject packet */
#define PPP_PROT_REJ (8) /**< Code of Protocol Reject packet */
#define PPP_ECHO_REQ (9) /**< Code of Echo Request packet */
#define PPP_ECHO_REP (10) /**< Code of Echo Reply packet */
#define PPP_DISC_REQ (11) /**< Code of Discard Request packet */
#define PPP_IDENT (12) /**< Code of Identification (not used yet) */
#define PPP_TIME_REM (13) /**< Code of Time Remaining /not used yet) */
#define PPP_UNKNOWN_CODE (0) /**< Code for Unknown Code packet (internal use)*/

#define BROADCAST_LCP (0xff) /**< Shortcut to LCP message */
#define BROADCAST_NCP (0xfe) /**< Broadcast message to al NCP available */


#define GNRC_PPP_MSG_QUEUE_SIZE (20)


#define PPPDEV_MSG_TYPE_EVENT (100)
#define GNRC_PPPDEV_MSG_TYPE_EVENT (101)

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
	ppp_protocol_t *protocol[NUM_OF_PROTS];
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
ppp_protocol_t *dcp_get_static_pointer(void);

void gnrc_ppp_link_up(msg_t *msg, kernel_pid_t pid);
void gnrc_ppp_link_down(msg_t *msg, kernel_pid_t pid);
void gnrc_ppp_dispatch_pkt(msg_t *msg, kernel_pid_t pid);
void gnrc_ppp_dial_up(msg_t *msg, kernel_pid_t pid);
void gnrc_ppp_disconnect(msg_t *msg, kernel_pid_t pid);
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
