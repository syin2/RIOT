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
#include "net/gnrc/netdev2.h"

#ifdef __cplusplus
extern "C" {
#endif

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
	E_RX,
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
} ppp_state_t;


/*Function flags*/
#define F_TLU (1<<0)
#define F_TLD (1<<1)
#define F_TLS (1<<2)
#define F_TLF (1<<3)
#define F_IRC (1<<4)
#define F_ZRC (1<<5)
#define F_SRC (1<<6)
#define F_SCA (1<<7)
#define F_SCN (1<<8)
#define F_STR (1<<9)
#define F_STA (1<<10)
#define F_SCJ (1<<11)
#define F_SER (1<<12)

#if 0
/* Functions flags for each state */
const uint16_t actions[PPP_NUM_EVENTS][PPP_NUM_STATES] = {
{0,F_IRC | F_SRC,0,0,0,0,0,0,0,0},
{0,0,0,F_TLS,0,0,0,0,0,F_TLD},
{F_TLS,0,F_IRC | F_SRC, 0,0,0,0,0,0,0},
{0,F_TLF,0,0,0,0,F_IRC | F_STR, F_IRC | F_STR, F_IRC | F_STR, F_TLD | F_IRC | F_STR},
{0,0,0,0,F_STR,F_STR,F_SRC,F_SRC,F_SRC,0},
{0,0,0,0,F_TLF,F_TLF,F_TLF,F_TLF,F_TLF,0},
{0,0,F_STA,F_IRC | F_SRC | F_SCA,0,0,F_SCA,F_SCA | F_TLU,F_SCA,F_TLD | F_SRC | F_SCA},
{0,0,F_STA,F_IRC | F_SRC | F_SCA,0,0,F_SCN,F_SCN,F_SCN,F_TLD | F_SRC | F_SCN},
{0,0,F_STA,F_STA,0,0,F_IRC | F_SRC,F_SRC,F_IRC | F_SRC, F_TLD | F_SRC},
{0,0,F_STA,F_STA,0,0,F_IRC | F_SRC,F_SRC, F_IRC | F_SRC,F_TLD | F_SRC},
{0,0,F_STA,F_STA,F_STA,F_STA,F_STA,F_STA,F_STA,F_TLD | F_ZRC | F_STA},
{0,0,0,0,F_TLF,F_TLF,0,0,0,F_TLD | F_SRC},
{0,0,F_SCJ,F_SCJ,F_SCJ,F_SCJ,F_SCJ,F_SCJ,F_SCJ,F_SCJ},
{0,0,0,0,0,0,0,0,0,0},
{0,0,F_TLF,F_TLF,F_TLF,F_TLF,F_TLF,F_TLF,F_TLF,F_TLD | F_IRC | F_STR},
{0,0,0,0,0,0,0,0,0,F_SER}
};


/* state transition for control layer FSM */
const int8_t state_trans[PPP_NUM_EVENTS][PPP_NUM_STATES] = {
{S_CLOSED,S_REQ_SENT,S_UNDEF,S_UNDEF,S_UNDEF,S_UNDEF,S_UNDEF,S_UNDEF,S_UNDEF,S_UNDEF},
{S_UNDEF,S_UNDEF,S_INITIAL,S_STARTING,S_INITIAL,S_STARTING,S_STARTING,S_STARTING,S_STARTING,S_STARTING},
{S_STARTING,S_STARTING,S_REQ_SENT,S_STOPPED,S_STOPPING,S_STOPPING,S_REQ_SENT,S_ACK_RCVD,S_ACK_SENT,S_OPENED},
{S_INITIAL,S_INITIAL,S_CLOSED,S_CLOSED,S_CLOSING,S_CLOSING,S_CLOSING,S_CLOSING,S_CLOSING,S_CLOSING},
{S_UNDEF,S_UNDEF,S_UNDEF,S_UNDEF,S_CLOSING,S_STOPPING,S_REQ_SENT,S_REQ_SENT,S_ACK_SENT,S_UNDEF},
{S_UNDEF,S_UNDEF,S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_STOPPED,S_STOPPED,S_STOPPED,S_UNDEF},
{S_UNDEF,S_UNDEF,S_CLOSED,S_ACK_SENT,S_CLOSING,S_STOPPING,S_ACK_SENT,S_OPENED,S_ACK_SENT,S_ACK_SENT},
{S_UNDEF,S_UNDEF,S_CLOSED,S_REQ_SENT,S_CLOSING,S_STOPPING,S_REQ_SENT,S_ACK_RCVD,S_REQ_SENT,S_REQ_SENT},
{S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_CLOSING,S_STOPPING,S_ACK_RCVD,S_REQ_SENT,S_OPENED,S_REQ_SENT},
{S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_CLOSING,S_STOPPING,S_REQ_SENT,S_REQ_SENT,S_ACK_SENT,S_REQ_SENT},
{S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_CLOSING,S_STOPPING,S_REQ_SENT,S_REQ_SENT,S_REQ_SENT,S_STOPPING},
{S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_CLOSED,S_STOPPED,S_REQ_SENT,S_REQ_SENT,S_ACK_SENT,S_REQ_SENT},
{S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_CLOSING,S_STOPPING,S_REQ_SENT,S_ACK_RCVD,S_ACK_SENT,S_OPENED},
{S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_CLOSING,S_STOPPING,S_REQ_SENT,S_REQ_SENT,S_ACK_SENT,S_OPENED},
{S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_CLOSED,S_STOPPED,S_STOPPED,S_STOPPED,S_STOPPED,S_STOPPING},
{S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_CLOSING,S_STOPPING,S_REQ_SENT,S_ACK_RCVD,S_ACK_SENT,S_OPENED}
};
#endif

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

#define PPP_MSG_UP (1)
#define PPP_MSG_DOWN (2)

#define PPP_MAX_TERMINATE (3)
#define PPP_MAX_CONFIG (3)

#define PPP_MSG_TIMEOUT (1)

#define PPP_CP_HDR_BASE_SIZE (4)

#define PPP_PAYLOAD_BUF_SIZE (256)

#define GNRC_PPP_MSG_QUEUE_SIZE (20)

#define CP_CREQ_ACK (0)
#define CP_CREQ_NAK (1)
#define CP_CREQ_REJ (2)


#define MAX_CP_OPTIONS (20)
#define OPT_PAYLOAD_SIZE (20)

#define PPP_CP_REQUEST_CONFIGURE (1)
#define PPP_CP_REQUEST_ACK (2)
#define PPP_CP_REQUEST_NAK (3)
#define PPP_CP_REQUEST_REJ (4)
#define PPP_CP_TERM_REQUEST (5)
#define PPP_CP_TERM_ACK (6)
#define PPP_CP_SER (7)

#define OPT_HAS_ACK (1)
#define OPT_HAS_NAK (2)
#define OPT_HAS_REJ (4)

#define RC_SEL_CONF (0)
#define RC_SEL_TERM (1)

struct ppp_dev_t;

/*Control Protocol configure option*/
typedef struct cp_opt_t{
	uint8_t type;
	uint8_t status;
	uint8_t payload[OPT_PAYLOAD_SIZE];
	size_t p_size;
} cp_opt_t;

/* Status of Control Protocol options response */
typedef struct opt_stack_t
{
	uint8_t type; /* Status of the set of CP opt response (ACK, NAK, REJ)*/
	uint8_t num_opts; /* Number of options in response */

	/* CP options to be sent are stored here, before copying to payload buffer*/
	cp_opt_t _opt_buf[MAX_CP_OPTIONS];
	uint8_t content_flag;
}opt_stack_t;

/* Control Protocol struct*/
typedef struct ppp_cp_t{
	uint8_t event;
	uint8_t l_upper_msg;
	uint8_t l_lower_msg;
	uint8_t up;
	uint8_t state;

	/* Select Configure or Terminate timer */
	uint8_t timer_select;

	uint32_t restart_time;
	uint32_t restart_counter;
	uint8_t counter_term;
	uint8_t counter_config;
	uint8_t counter_failure;

	struct ppp_dev_t *dev;

	/* Outgoing options stack*/
	opt_stack_t outgoing_opts;

	/* For Configure Request */
	uint8_t cr_identifier;
	uint8_t *cr_send_opts;

	/* For terminate request */
	uint8_t tr_identifier;

} ppp_cp_t;


/* PPP device */
typedef struct ppp_dev_t{
	struct ppp_cp_t *l_lcp;
	struct ppp_cp_t *l_ncp;
	gnrc_netdev2_t *dev;

	uint8_t _payload_buf[PPP_PAYLOAD_BUF_SIZE];
	uint8_t _payload_size;
} ppp_dev_t;


void test_handle_cp_rcr(ppp_cp_t *l_lcp, gnrc_pktsnip_t *pkt);
void test_ppp_recv_pkt(ppp_dev_t *dev, gnrc_pktsnip_t *pkt);



void ppp_send(ppp_dev_t *dev, gnrc_pktsnip_t *pkt);

#ifdef __cplusplus
}
#endif

#endif /* GNRC_PPP_H_ */
/** @} */
