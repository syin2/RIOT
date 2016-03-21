/*
 * Copyright (C) 2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 * @ingroup     ppp_lcp
 * @file
 * @brief       Implementation of PPP's LCP protocol
 *
 * @author      José Ignacio Alamos <jialamos@uc.cl>
 * @}
 */

#include "net/gnrc/ppp/lcp.h"
#include "net/gnrc/ppp/ppp.h"
#include "net/gnrc/ppp/cp.h"
#include "net/ppp/opt.h"

#define ENABLE_DEBUG    (0)
#include "debug.h"

#if ENABLE_DEBUG
/* For PRIu16 etc. */
#include <inttypes.h>
#endif

/* Negotiate local LCP options with NAK opts sent by peer */
void lcp_negotiate_nak(void *lcp_opt, cp_pkt_metadata_t *metadata)
{
	/* Cast lcp_opt to corresponding struct */
	lcp_opt_t *opts = (lcp_opt_t*) lcp_opt;

	void *curr_opt;
	uint8_t ctype;
	uint16_t suggested_value;
	uint8_t *payload;

	opt_metadata_t *opts_handler = &metadata->opts;

	curr_opt = ppp_opts_get_head(opts_handler);
	int num_opts = ppp_opts_get_num(opts_handler);
	/* Iterate through every pkt option */
	for(int i=0;i<num_opts;i++)
	{
		ctype = ppp_opt_get_type(curr_opt);
		payload = (uint8_t*) ppp_opt_get_payload(curr_opt);
		switch(ctype)
		{
			case LCP_OPT_MRU:
				suggested_value = ((*payload)<<8) + *(payload+1);

				if(suggested_value > LCP_MAX_MRU){
					opts->mru = LCP_MAX_MRU;
				}
				else
				{
					opts->mru = suggested_value;
				}
				break;
			default:
				break;
		}
		curr_opt = ppp_opts_next(opts_handler);
	}
}
