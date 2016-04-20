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
#include "net/gnrc/ppp/lcp.h"
#include "net/gnrc/ppp/ipcp.h"
#include "sys/uio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GNRC_PPP_MSG_QUEUE 64

#define PPP_HDLC_ADDRESS (0xFF)
#define PPP_HDLC_CONTROL (0x03)

#define PPP_LINKUP (0)
#define PPP_RECV (1)
#define PPP_TIMEOUT (2)
#define PPP_LINKDOWN (4)

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

/*Function flags*/
#define F_TLU (1U<<0)
#define F_TLD (1U<<1)
#define F_TLS (1U<<2)
#define F_TLF (1U<<3)
#define F_IRC (1U<<4)
#define F_ZRC (1U<<5)
#define F_SCR (1U<<6)
#define F_SCA (1U<<7)
#define F_SCN (1U<<8)
#define F_STR (1U<<9)
#define F_STA (1U<<10)
#define F_SCJ (1U<<11)
#define F_SER (1U<<12)

#define PPP_MAX_TERMINATE (3)
#define PPP_MAX_CONFIG (3)

#define GNRC_PPP_MSG_QUEUE_SIZE (20)

#define OPT_PAYLOAD_BUF_SIZE (100)

#define OPT_ENABLED (1)
#define OPT_REQUIRED (2)

/*TODO: Of course, change!*/
#define PPPDEV_MSG_TYPE_EVENT (100)

typedef enum{
	E_UP,
	E_DOWN,
	E_OPEN,
	E_CLOSE,
	E_TOp,
	E_TOm,
	E_RCRp,
	E_RCRm,
	E_RCA,
	E_RCN,
	E_RTR,
	E_RTA,
	E_RUC,
	E_RXJp,
	E_RXJm,
	E_RXR,
	PPP_NUM_EVENTS
} ppp_event_t;


typedef enum{
	S_UNDEF=-1,
	S_INITIAL=0,
	S_STARTING,
	S_CLOSED,
	S_STOPPED,
	S_CLOSING,
	S_STOPPING,
	S_REQ_SENT,
	S_ACK_RCVD,
	S_ACK_SENT,
	S_OPENED,
	PPP_NUM_STATES
} ppp_cp_state_t;



/* Functions flags for each state */
static const uint16_t actions[PPP_NUM_EVENTS][PPP_NUM_STATES] = {
{0,F_IRC | F_SCR,0,0,0,0,0,0,0,0},
{0,0,0,F_TLS,0,0,0,0,0,F_TLD},
{F_TLS,0,F_IRC | F_SCR,0,0,0,0,0,0,0},
{0,F_TLF,0,0,0,0,F_IRC | F_STR,F_IRC | F_STR,F_IRC | F_STR,F_TLD | F_IRC | F_STR},
{0,0,0,0,F_STR,F_STR,F_SCR,F_SCR,F_SCR,0},
{0,0,0,0,F_TLF,F_TLF,F_TLF,F_TLF,F_TLF,0},
{0,0,F_STA,F_IRC | F_SCR | F_SCA,0,0,F_SCA,F_SCA | F_TLU,F_SCA,F_TLD | F_SCR | F_SCA},
{0,0,F_STA,F_IRC | F_SCR | F_SCN,0,0,F_SCN,F_SCN,F_SCN,F_TLD | F_SCR | F_SCN},
{0,0,F_STA,F_STA,0,0,F_IRC,F_SCR,F_IRC | F_TLU,F_TLD | F_SCR},
{0,0,F_STA,F_STA,0,0,F_IRC | F_SCR,F_SCR,F_IRC | F_SCR,F_TLD | F_SCR},
{0,0,F_STA,F_STA,F_STA,F_STA,F_STA,F_STA,F_STA,F_TLD | F_ZRC | F_STA},
{0,0,0,0,F_TLF,F_TLF,0,0,0,F_TLD | F_SCR},
{0,0,F_SCJ,F_SCJ,F_SCJ,F_SCJ,F_SCJ,F_SCJ,F_SCJ,F_SCJ},
{0,0,0,0,0,0,0,0,0,0},
{0,0,F_TLF,F_TLF,F_TLF,F_TLF,F_TLF,F_TLF,F_TLF,F_TLD | F_IRC | F_STR},
{0,0,0,0,0,0,0,0,0,F_SER}};

typedef enum
{
	PPP_LINK_DEAD,
	PPP_LINK_ESTABLISHED,
	PPP_AUTHENTICATION,
	PPP_NETWORK,
	PPP_OPEN,
	PPP_TERMINATION
} ppp_state_t;


/* state transition for control layer FSM */
static const int8_t state_trans[PPP_NUM_EVENTS][PPP_NUM_STATES] = {
{S_CLOSED,S_REQ_SENT,S_UNDEF,S_UNDEF,S_UNDEF,S_UNDEF,S_UNDEF,S_UNDEF,S_UNDEF,S_UNDEF},
{S_UNDEF,S_UNDEF,S_INITIAL,S_STARTING,S_INITIAL,S_STARTING,S_STARTING,S_STARTING,S_STARTING,S_STARTING},
{S_STARTING,S_STARTING,S_REQ_SENT,S_STOPPED,S_STOPPING,S_STOPPING,S_REQ_SENT,S_ACK_RCVD,S_ACK_SENT,S_OPENED},
{S_INITIAL,S_INITIAL,S_CLOSED,S_CLOSED,S_CLOSING,S_CLOSING,S_CLOSING,S_CLOSING,S_CLOSING,S_CLOSING},
{S_UNDEF,S_UNDEF,S_UNDEF,S_UNDEF,S_CLOSING,S_STOPPING,S_REQ_SENT,S_REQ_SENT,S_ACK_SENT,S_UNDEF},
{S_UNDEF,S_UNDEF,S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_STOPPED,S_STOPPED,S_STOPPED,S_UNDEF},
{S_UNDEF,S_UNDEF,S_CLOSED,S_ACK_SENT,S_CLOSING,S_STOPPING,S_ACK_SENT,S_OPENED,S_ACK_SENT,S_ACK_SENT},
{S_UNDEF,S_UNDEF,S_CLOSED,S_REQ_SENT,S_CLOSING,S_STOPPING,S_REQ_SENT,S_ACK_RCVD,S_REQ_SENT,S_REQ_SENT},
{S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_CLOSING,S_STOPPING,S_ACK_RCVD,S_UNDEF,S_OPENED,S_UNDEF},
{S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_CLOSING,S_STOPPING,S_REQ_SENT,S_UNDEF,S_ACK_SENT,S_UNDEF},
{S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_CLOSING,S_STOPPING,S_REQ_SENT,S_REQ_SENT,S_REQ_SENT,S_STOPPING},
{S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_CLOSED,S_STOPPED,S_REQ_SENT,S_REQ_SENT,S_ACK_SENT,S_REQ_SENT},
{S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_CLOSING,S_STOPPING,S_REQ_SENT,S_ACK_RCVD,S_ACK_SENT,S_OPENED},
{S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_CLOSING,S_STOPPING,S_REQ_SENT,S_REQ_SENT,S_ACK_SENT,S_OPENED},
{S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_CLOSED,S_STOPPED,S_STOPPED,S_STOPPED,S_STOPPED,S_STOPPING},
{S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_CLOSING,S_STOPPING,S_REQ_SENT,S_ACK_RCVD,S_ACK_SENT,S_OPENED}};



typedef struct ppp_cp_t ppp_cp_t;
typedef struct cp_conf_t cp_conf_t;

/* Control Protocol struct*/
typedef struct ppp_cp_t{
	uint8_t id;
	gnrc_nettype_t prottype;
	uint16_t supported_codes;
	uint8_t state;

	uint8_t restart_counter;
	uint8_t counter_failure;
	uint32_t restart_timer;

	struct gnrc_pppdev_t *dev;
	xtimer_t xtimer;

	/* For Configure Request */
	uint8_t cr_sent_identifier;

	uint8_t cr_sent_opts[OPT_PAYLOAD_BUF_SIZE];
	uint16_t cr_sent_size;

	/* For terminate request */
	uint8_t tr_sent_identifier;

	msg_t msg;

	int (*handle_pkt)(ppp_cp_t *cp, ppp_hdr_t *hdr, gnrc_pktsnip_t *pkt);
	cp_conf_t* (*get_conf_by_code)(ppp_cp_t *cp, uint8_t code);

	cp_conf_t *conf;
} ppp_cp_t;


typedef struct pppdev_t pppdev_t;

typedef struct pppdev_driver_t
{
	int (*send)(pppdev_t *dev, const struct iovec *vector, int count);
	int (*recv)(pppdev_t *dev, char *buf, int len, void *info);
	void (*driver_ev)(pppdev_t *dev, uint8_t event);
	int (*init)(pppdev_t *dev);

} pppdev_driver_t;

typedef struct pppdev_t
{
	pppdev_driver_t *driver;
} pppdev_t;

typedef struct cp_conf_t
{
	uint8_t type;
	network_uint32_t value;
	size_t size;
	uint8_t flags;

	uint8_t (*is_valid)(ppp_option_t *opt);
	void (*handle_nak)(struct cp_conf_t *conf, ppp_option_t *opt);
	uint8_t (*build_nak_opts)(ppp_option_t *opt);
	struct cp_conf_t *next;
} cp_conf_t;

/* PPP device */
typedef struct gnrc_pppdev_t{
	ppp_cp_t l_lcp;
	ppp_cp_t l_ipcp;
	pppdev_t *netdev;

	cp_conf_t lcp_opts[LCP_NUMOPTS];
	cp_conf_t ipcp_opts[IPCP_NUMOPTS];

	uint8_t state;
} gnrc_pppdev_t;


int gnrc_ppp_init(gnrc_pppdev_t *dev, pppdev_t *netdev);
gnrc_pktsnip_t *pkt_build(gnrc_nettype_t pkt_type, uint8_t code, uint8_t id, gnrc_pktsnip_t *payload);
int gnrc_ppp_send(pppdev_t *dev, gnrc_pktsnip_t *pkt);
int gnrc_ppp_recv(gnrc_pppdev_t *dev, gnrc_pktsnip_t *pkt);
int gnrc_ppp_event_callback(gnrc_pppdev_t *dev, int ppp_event);

/* Implementation of LCP fsm actions */
void tlu(ppp_cp_t *lcp, void *args);
void tld(ppp_cp_t *lcp, void *args);
void tls(ppp_cp_t *lcp, void *args);
void tlf(ppp_cp_t *lcp, void *args);
void irc(ppp_cp_t *lcp, void *args);
void zrc(ppp_cp_t *lcp, void *args);
void scr(ppp_cp_t *lcp, void *args);
void sca(ppp_cp_t *lcp, void *args);
void scn(ppp_cp_t *lcp, void *args);
void str(ppp_cp_t *lcp, void *args);
void sta(ppp_cp_t *lcp, void *args);
void scj(ppp_cp_t *lcp, void *args);
void ser(ppp_cp_t *lcp, void *args);

int cp_init(struct gnrc_pppdev_t *ppp_dev, ppp_cp_t *cp);
int trigger_event(ppp_cp_t *cp, int event, gnrc_pktsnip_t *pkt);
int handle_rcr(ppp_cp_t *cp, ppp_hdr_t *hdr, gnrc_pktsnip_t *pkt);
int handle_rca(ppp_cp_t *cp, ppp_hdr_t *hdr, gnrc_pktsnip_t *pkt);
int handle_rcn_nak(ppp_cp_t *cp, ppp_hdr_t *hdr, gnrc_pktsnip_t *pkt);
int handle_rcn_rej(ppp_cp_t *cp, ppp_hdr_t *hdr, gnrc_pktsnip_t *pkt);
int handle_coderej(ppp_hdr_t *hdr, gnrc_pktsnip_t *pkt);
int handle_term_ack(ppp_cp_t *cp, ppp_hdr_t *hdr, gnrc_pktsnip_t *pkt);
int fsm_event_from_pkt(ppp_cp_t *cp, ppp_hdr_t *hdr, gnrc_pktsnip_t *pkt);

void print_pkt(gnrc_pktsnip_t *pkt);
int _pkt_get_ppp_header(gnrc_pktsnip_t *pkt, ppp_hdr_t **ppp_hdr);

void broadcast_upper_layer(msg_t *msg, uint8_t id, uint8_t event);
void *gnrc_ppp_thread(void *args);

#ifdef __cplusplus
}
#endif

#endif /* GNRC_PPP_H_ */
/** @} */
