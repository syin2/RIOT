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
typedef enum{
	F_TLU=1<<0,
	F_TLD=1<<1,
	F_TLS=1<<2,
	F_TLF=1<<3,
	F_IRC=1<<4,
	F_ZRC=1<<5,
	F_SRC=1<<6,
	F_SCA=1<<7,
	F_SCN=1<<8,
	F_STR=1<<9,
	F_STA=1<<10,
	F_SCJ=1<<11,
	F_SER=1<<12
} ppp_function_flag;

/* Functions flags for each state */
const uint8_t actions[PPP_NUM_EVENTS][PPP_NUM_STATES] = {
{0,F_IRC | F_SCR,0,0,0,0,0,0,0,0}
{0,0,0,F_TLS,0,0,0,0,0,F_TLD}
{F_TLS,0,F_IRC | F_SCR, 0,0,0,0,0,0,0}
{0,F_TLF,0,0,0,0,F_IRC | F_STR, F_IRC | F_STR, F_IRC | F_STR, F_TLD | F_IRC, F_STR}
{0,0,0,0,F_STR,F_STR,F_SCR,F_SCR,F_SCR,0}
{0,0,0,0,F_TLF,F_TLF,F_TLF,F_TLF,F_TLF,0}
{0,0,F_STA,F_IRC | F_SCR | F_SCA,0,0,F_SCA,F_SCA | F_TLU,F_SCA,F_TLD | F_SCR | F_SCA}
{0,0,F_STA,F_IRC | F_SCR | F_SCA,0,0,F_SCN,F_SCN,F_SCN,F_TLD | F_SCR | F_SCN}
{0,0,F_STA,F_STA,0,0,F_IRC | F_SCR,F_SCR,F_IRC | F_SCR, F_TLD | F_SCR}
{0,0,F_STA,F_STA,0,0,F_IRC | F_SCR,F_SCR, F_IRC | F_SCR,F_TLD | F_SCR}
{0,0,F_STA,F_STA,F_STA,F_STA,F_STA,F_STA,F_STA,F_TLD | F_ZRC | F_STA}
{0,0,0,0,F_TLF,F_TLF,0,0,0,F_TLD F_SCR}
{0,0,F_SCJ,F_SCJ,F_SCJ,F_SCJ,F_SCJ,F_SCJ,F_SJC,F_SJC}
{0,0,0,0,0,0,0,0,0,0}
{0,0,F_TLF,F_TLF,F_TLF,F_TLF,F_TLF,F_TLF,F_TLF,F_TLD | F_IRC | F_STR}
{0,0,0,0,0,0,0,0,0,F_SER}
}



/* state transition for control layer FSM */
const uint8_t state_trans[PPP_NUM_EVENTS][PPP_NUM_STATES] = {
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
}

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


typedef struct {
	int state;
	gnrc_netdev2_t *dev;
} ppp_dev_t;

typedef struct __attribute__((packed)) {
	uint8_t type;
	uint8_t identifier;
	uint16_t length;
} ppp_ctrl_hdr_t;

typedef struct __attribute__((packed)) {
	uint8_t type;
	uint16_t length;
} ppp_opt_hdr_t;


#ifdef __cplusplus
}
#endif

#endif /* GNRC_PPP_H_ */
/** @} */
