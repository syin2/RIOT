
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
#include "byteorder.h"



uint8_t ppp_pkt_get_code(cp_pkt_t *cp_pkt)
{
	return cp_pkt->hdr.code;
}

void ppp_pkt_set_code(cp_pkt_t *cp_pkt, uint8_t code)
{
	cp_pkt->hdr.code = code;
}

uint8_t ppp_pkt_get_id(cp_pkt_t *cp_pkt)
{
	return cp_pkt->hdr.id;
}

void ppp_pkt_set_id(cp_pkt_t *cp_pkt, uint8_t id)
{
	cp_pkt->hdr.id = id;
}

uint16_t ppp_pkt_get_length(cp_pkt_t *cp_pkt)
{
	return byteorder_ntohs(cp_pkt->hdr.length);
}

void ppp_pkt_set_length(cp_pkt_t *cp_pkt, uint16_t length)
{
	cp_pkt->hdr.length = byteorder_htons(length);
}

/*TODO return error if populate went bad */
int ppp_pkt_populate(uint8_t *data, size_t length, cp_pkt_t *cp_pkt)
{
	cp_hdr_t *hdr = (cp_hdr_t*) data;
	cp_pkt->hdr = *hdr;

	int pkt_length = byteorder_ntohs(hdr->length);
	/*TODO Padding... */
	if (pkt_length != (int)length) {
		return EBADMSG;
	}

	memcpy(cp_pkt->payload, data+sizeof(cp_hdr_t), pkt_length-sizeof(cp_hdr_t));
	/* Copy payload */
	return 0;
}

