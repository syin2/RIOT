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
#include "net/ppp/hdr.h"
#include "net/gnrc/ppp/cp.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PPP_BUF_SIZE (200)
#define PPP_HDLC_ADDRESS (0xFF)
#define PPP_HDLC_CONTROL (0x03)

#define PPP_LINKUP (0)
#define PPP_RECV (1)
#define PPP_TIMEOUT (2)

/* PPP device */
typedef struct ppp_dev_t{
	ppp_cp_t l_lcp;
	ppp_cp_t l_ncp;
	netdev2_t *netdev;
} ppp_dev_t;



int gnrc_ppp_init(ppp_dev_t *dev, netdev2_t *netdev);
gnrc_pktsnip_t *pkt_build(gnrc_nettype_t pkt_type, uint8_t code, uint8_t id, gnrc_pktsnip_t *payload);
int gnrc_ppp_send(netdev2_t *dev, gnrc_pktsnip_t *pkt);
int gnrc_ppp_recv(ppp_dev_t *dev, gnrc_pktsnip_t *pkt);
int gnrc_ppp_event_callback(ppp_dev_t *dev, int ppp_event);

#ifdef __cplusplus
}
#endif

#endif /* GNRC_PPP_H_ */
/** @} */
