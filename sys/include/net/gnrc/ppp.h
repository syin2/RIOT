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
typedef enum {
	LCPSTATE_DEAD,
	LCPSTATE_ESTABLISHED,
	LCPSTATE_AUTH,
	LCP_NCP,
	LCP_OPEN,
	LCP_TERMINATION
} lcp_state_t; 

typedef struct {
	int state;
} ppp_dev_t;


#ifdef __cplusplus
}
#endif

#endif /* GNRC_PPP_H_ */
/** @} */
