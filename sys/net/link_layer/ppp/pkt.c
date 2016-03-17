
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



int ppp_cp_optchain_are_equal(cp_opt_t *o1, cp_opt_t *o2)
{
	if (o1->type != o2->type || o1->status != o2->status || o1->p_size != o2->p_size || memcmp(o1->payload,
	o2->payload,o1->p_size)){
		return false;
	}
	return true;
}

int populate_opt_metadata()

	cp_pkt_t *pkt=cp->metadata.pkt;
	uint16_t length = ppp_pkt_get_length(pkt);
	uint16_t cursor=0;
	uint8_t flag=0;

	cp_opt_t *opt;
	opt_metadata_t opt_tag;
	uint8_t *payload = pkt->payload;
	uint8_t status;

	cp_opt **previous_next_pointer=NULL;

	/* Iterate over options */
	while(cursor < length)
	{
		opt = (cp_opt_t*) (payload+cursor);
		status = cp->get_option_status(opt);
		switch(status)
		{
			case CP_CREQ_ACK:
				flag |= OPT_HAS_ACK;
				break;
			case CP_CREQ_NAK:
				flag |= OPT_HAS_NAK;
				break;
			case CP_CREQ_REJ:
				flag |= OPT_HAS_REJ;
				break;
		}
		cursor += opt->length;
		/*Tag option */
		if (previous_next_pointer != NULL)
		{
			*previous_next_pointer = &opt;
		}
		opt_tag.opt = opt;
		opt_tag.status = status;
		previous_next_pointer = &(opt_tag.next);
		cp->metadata.tagget_opts[cp->metadata.num_tagged_opts] = opt_tag;
	}
	/* Fill metadata */
	cp->metadata.opt_status_flag = flag;

}
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
int ppp_pkt_populate(uint8_t *data, size_t length)
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

int ppp_pkt_is_configure(cp_pkt_t *pkt)
{
	uint8_t code = ppp_pkt_get_code(pkt);
	return (code == PPP_CONF_REQ || code == PPP_CONF_ACK || code == PPP_CONF_NAK || code == PPP_CONF_REJ);
}

void ppp_pkt_get_metadata(cp_pkt_metadata_t *metadata, cp_pkt_t *pkt, int (*get_opt_status)(cp_opt*))
{
	uint8_t code = ppp_pkt_get_code(pkt);
	metadata->opt_status_content=0;

	cp_opt_t *curr_opt;
	uint16_t curr_status;
	uint16_t cursor = 0;

	uint16_t num_tagged = 0;

	/* Check if current code has options */
	if (code == PPP_CONF_REQ)
	{
		/* Iterate through options */
		curr_opt = (cp_opt_t*) (pkt->payload+cursor);
		curr_status = get_opt_status(curr_opt);
		switch(curr_status)
		{
			case CP_CREQ_ACK:
				metadata->opt_status_content |= OPT_HAS_ACK;
				break;
			case CP_CREQ_NAK:
				metadata->opt_status_content |= OPT_HAS_NAK;
				break;
			case CP_CREQ_REJ:
				metadata->opt_status_content |= OPT_HAS_REJ;
				break;
		}
		metadata->tagged_opts[num_tagged].status = curr_status
		metadata->tagged_opts[num_tagged].opt = curr_opt;
		cursor+=curr_opt->length;
		num_tagged+=1;
	}
	metadata->num_tagged_opts = num_tagged;
}
