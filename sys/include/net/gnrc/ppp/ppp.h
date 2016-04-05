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
#include "net/gnrc/ppp/cp.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PPP_BUF_SIZE (200)
#define PPP_HDLC_ADDRESS (0xFF)
#define PPP_HDLC_CONTROL (0x03)

struct ppp_dev_t;

/* PPP device */
typedef struct ppp_dev_t{
	struct ppp_cp_t *l_lcp;
	struct ppp_cp_t *l_ncp;
	gnrc_netdev2_t *dev;
} ppp_dev_t;



gnrc_ptksnip_t *lcp_pkt_build(uint8_t type, uint8_t id, gnrc_pktsnip_t *payload);
int gnrc_ppp_send(netdev2_t *dev, gnrc_pktsnip_t *pkt);

#ifdef __cplusplus
}
#endif

#endif /* GNRC_PPP_H_ */
/** @} */
