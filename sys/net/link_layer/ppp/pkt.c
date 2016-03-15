
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

#include "msg.h"
#include "thread.h"
#include "net/gnrc.h"
#include "net/ppptype.h"
#include "net/gnrc/ppp/ppp.h"
#include "net/gnrc/ppp/lcp.h"
#include "net/hdlc/hdr.h"


int ppp_cp_populate_options(opt_stack_t *o_stack, uint8_t *payload, size_t p_size)
{
	o_stack->num_opts = 0;

	/*Start iterating over options */
	uint16_t cursor = 0;
	
	uint8_t curr_type, curr_len;
	uint8_t curr_status;

	/* Current option status*/
	cp_opt_t curr_opt_status;

	cp_opt_t *last_opt= NULL;

	o_stack->content_flag=0;

	/* TODO: Check default value (no opts sent)*/
	while(cursor < p_size) {
		/* Read current option type */
		curr_type = *(payload+cursor);
		curr_len = *(payload+cursor+1);
		
		/* TODO: If cursor + len > total_length, discard pkt*/

		curr_opt_status.next = NULL;
		_read_lcp_pkt(curr_type, payload+cursor+2, (size_t) curr_len, &curr_opt_status);
		if(last_opt != NULL)
		{
			last_opt->next = &curr_opt_status;
		}
		curr_status =curr_opt_status.status;

		DEBUG("Current status: %i\n",curr_status);

		o_stack->content_flag |= 1<<curr_status;

		o_stack->_opt_buf[o_stack->num_opts] = curr_opt_status;
		o_stack->num_opts+=1;
		cursor = cursor + curr_len;
		last_opt = &curr_opt_status;
	}

	return 0; /*TODO: Check return*/
}

int ppp_cp_opts_are_equal(cp_opt_t *o1, cp_opt_t *o2)
{
	if (o1->type != o2->type || o1->status != o2->status || o1->p_size != o2->p_size || memcmp(o1->payload,
	o2->payload,o1->p_size)){
		return false;
	}
	return true;
}

static int ppp_cp_optstacks_are_equal(opt_stack_t *o1, opt_stack_t *o2)
{
	uint8_t len_1 = o1->num_opts;
	uint8_t len_2= o2->num_opts;

	if (len_1 != len_2)
	{
		return false;
	}
	for(int i=0;i<len_1;i++)
	{
		if(!_opt_are_equal(o1->_opt_buf[i], o2->_opt_buf[i]))
		{
			return false;
		}
	}
	return true;
}

/*TODO return error if populate went bad */
static int ppp_pkt_populate_from_pktsnip(gnrc_pktsnipt_t pkt, cp_pkt_t *cp_pkt)
{
	cp_hdr_t hdr = (cp_hdr_t*) pkt->data;
	cp_pkt->hdr = &cp_data;

	if (hdr->length != pkt->size) {
		/* TODO: Error code*/
		return 0;
	}

	int status = _parse_cp_options(cp_pkt->opts, pkt->data+sizeof(cp_hdr_t),(size_t) pkt->length-sizeof(cp_hdr_t));
	return 0;
}
