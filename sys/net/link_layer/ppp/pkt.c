
/*
 * Copyright (C) 2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 * @ingroup     net_ppp_pkt
 * @file
 * @brief       Implementation of the of Generic Control Protocol for PPP
 *
 * @author      José Ignacio Alamos <jialamos@uc.cl>
 * @}
 */

#include <errno.h>
#include <string.h>

#include "net/ppp/pkt.h"



/*TODO return error if populate went bad */
int ppp_pkt_populate(uint8_t *data, size_t length, cp_pkt_t *cp_pkt)
{
	cp_hdr_t *hdr = (cp_hdr_t*) data;
	cp_pkt->hdr = hdr;

	if (hdr->length != length) {
		return EBADMSG;
	}

	memcpy(cp_pkt->payload, data, length-sizeof(cp_hdr_t));
	return 0;
}
