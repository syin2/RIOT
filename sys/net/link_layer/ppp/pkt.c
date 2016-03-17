
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
#include <errno.h>



uint8_t ppp_pkt_get_code(cp_pkt_t *cp_pkt)
{
	return cp_pkt->hdr->code;
}

void ppp_pkt_set_code(cp_pkt_t *cp_pkt, uint8_t code)
{
	cp_pkt->hdr->code = code;
}

uint8_t ppp_pkt_get_id(cp_pkt_t *cp_pkt)
{
	return cp_pkt->hdr->id;
}

void ppp_pkt_set_id(cp_pkt_t *cp_pkt, uint8_t id)
{
	cp_pkt->hdr->id = id;
}

uint16_t ppp_pkt_get_length(cp_pkt_t *cp_pkt)
{
	return byteorder_ntohs(cp_pkt->hdr->length);
}

void ppp_pkt_set_length(cp_pkt_t *cp_pkt, uint16_t length)
{
	cp_pkt->hdr->length = byteorder_htons(length);
}

/*TODO return error if populate went bad */
int ppp_pkt_init(uint8_t *data, size_t length, cp_pkt_t *cp_pkt)
{
	cp_hdr_t *hdr = (cp_hdr_t*) data;
	cp_pkt->hdr = hdr;
	cp_pkt->payload = data+sizeof(cp_hdr_t);

	(void) length;
	return 0;
}

int ppp_pkt_is_configure(cp_pkt_t *pkt)
{
	uint8_t code = ppp_pkt_get_code(pkt);
	return (code == PPP_CONF_REQ || code == PPP_CONF_ACK || code == PPP_CONF_NAK || code == PPP_CONF_REJ);
}

void ppp_pkt_gen_metadata(cp_pkt_metadata_t *metadata, cp_pkt_t *pkt, int (*get_opt_status)(cp_opt_hdr_t*))
{
	uint8_t code = ppp_pkt_get_code(pkt);
	metadata->pkt = pkt;
	metadata->opts_status_content=0;

	cp_opt_hdr_t *curr_opt;
	uint16_t curr_status;
	uint16_t cursor = 0;

	uint16_t num_tagged = 0;

	/* Check if current code has options */
	if (code == PPP_CONF_REQ)
	{
		/* Iterate through options */
		curr_opt = (cp_opt_hdr_t*) (pkt->payload+cursor);
		curr_status = get_opt_status(curr_opt);
		switch(curr_status)
		{
			case CP_CREQ_ACK:
				metadata->opts_status_content |= OPT_HAS_ACK;
				break;
			case CP_CREQ_NAK:
				metadata->opts_status_content |= OPT_HAS_NAK;
				break;
			case CP_CREQ_REJ:
				metadata->opts_status_content |= OPT_HAS_REJ;
				break;
		}
		metadata->tagged_opts[num_tagged].status = curr_status;
		metadata->tagged_opts[num_tagged].opt = curr_opt;
		cursor+=curr_opt->length;
		num_tagged+=1;
	}
	metadata->num_tagged_opts = num_tagged;
}
