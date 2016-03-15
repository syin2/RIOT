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


struct ppp_dev_t;

/* PPP device */
typedef struct ppp_dev_t{
	struct ppp_cp_t *l_lcp;
	struct ppp_cp_t *l_ncp;
	gnrc_netdev2_t *dev;

	uint8_t _hdlc_cp_buf[PPP_PAYLOAD_BUF_SIZE];
	uint32_t _hdlc_cp_size;
} ppp_dev_t;




void test_handle_cp_rcr(ppp_cp_t *l_lcp, gnrc_pktsnip_t *pkt);
void test_ppp_recv_pkt(ppp_dev_t *dev, gnrc_pktsnip_t *pkt);



void ppp_send(ppp_dev_t *dev, gnrc_pktsnip_t *pkt);

#ifdef __cplusplus
}
#endif

#endif /* GNRC_PPP_H_ */
/** @} */
