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

#define E_UP (1<<0)
#define E_DOWN (1<<1)
#define E_OPEN (1<<2)
#define E_CLOSE (1<<3)
#define E_TOp (1<<4)
#define E_TOm (1<<5)
#define E_RCRp (1<<6)
#define E_RCRm (1<<7)
#define E_RCA (1<<8)
#define E_RCN (1<<9)
#define E_RTR (1<<10)
#define E_RTA (1<<11)
#define E_RUC (1<<12)
#define E_RXJp (1<<13)
#define E_RXJm (1<<14)
#define E_RX (1<<15)

#define PPP_STATE_INITIAL (0)
#define PPP_STATE_STARTING (1)
#define PPP_STATE_CLOSED (2)
#define PPP_STATE_STOPPED (3)
#define PPP_STATE_CLOSING (4)
#define PPP_STATE_STOPPING (5)
#define PPP_STATE_REQ_SENT (6)
#define PPP_STATE_ACK_RCVD (7)
#define PPP_STATE_ACK_SENT (8)
#define PPP_STATE_OPENED (9)

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
