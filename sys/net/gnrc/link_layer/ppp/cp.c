
/*
 * Copyright (C) 2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 * @ingroup     net_ppp_cp
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
#include "net/gnrc/ppp/cp.h"
#include "net/gnrc/ppp/cp_fsm.h"
#include "net/ppp/pkt.h"

#define ENABLE_DEBUG    (1)
#include "debug.h"

#if ENABLE_DEBUG
/* For PRIu16 etc. */
#include <inttypes.h>
#endif




void handle_cp_pkt(ppp_cp_t *cp, cp_pkt_t *pkt)
{
	cp->metadata.pkt = pkt;
	ppp_pkt_gen_metadata(&cp->metadata, pkt, cp->get_option_status);

	/* Check options recv are subset of opts sent */

	int type = ppp_pkt_get_code(pkt);
	
	switch(type){
		case PPP_CONF_REQ:
		case PPP_CONF_ACK:
		case PPP_CONF_NAK:
		case PPP_CONF_REJ:
			cp->handle_conf(ppp_cp_t *cp);
		case PPP_TERM_REQ:
			break;
		case PPP_TERM_ACK:
			break;
		case PPP_CODE_REJ:
			break;
		case PPP_PROT_REJ:
			break;
		case PPP_ECHO_REQ:
			break;
		case PPP_ECHO_REP:
			break;
		case PPP_DISC_REQ:
			break;
		case PPP_IDENT:
			break;
		case PPP_TIME_REM:
			break;
	}
}

void ppp_pkt_gen_metadata(cp_pkt_metadata_t *metadata, cp_pkt_t *pkt, int (*get_opt_status)(void*))
{
	uint8_t code = ppp_pkt_get_code(pkt);
	metadata->pkt = pkt;
	metadata->opts_status_content=0;

	void *curr_opt;
	uint16_t curr_status;

	/* Check if current code has optio=ns */
	if (code == PPP_CONF_REQ || code == PPP_CONF_NAK || code == PPP_CONF_REJ)
	{
		ppp_opts_init(&metadata->opts, pkt);
		curr_opt = ppp_opts_get_head(&metadata->opts);
		for(int i=0; i<ppp_opts_get_num(&metadata->opts); i++)
		{
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
			curr_opt = ppp_opts_next(&metadata->opts);
			metadata->tagged_opts[i] = curr_status;
		}
	}
	ppp_opts_reset(&metadata->opts);
}


/* Call functions depending on function flag*/
static void _event_action(ppp_cp_t *cp) 
{
	uint8_t flags;
	/* Reset link status */
	cp->l_upper_msg = 0;
	cp->l_lower_msg = 0;

	flags = actions[cp->state][cp->event];

	if(flags & F_TLU) _tlu(cp);
	if(flags & F_TLD) _tld(cp);
	if(flags & F_TLS) _tls(cp);
	if(flags & F_TLF) _tlf(cp);
	if(flags & F_IRC) _irc(cp);
	if(flags & F_ZRC) _zrc(cp);
	if(flags & F_SRC) _src(cp);
	if(flags & F_SCA) _sca(cp);
	if(flags & F_SCN) _scn(cp);
	if(flags & F_STR) _str(cp);
	if(flags & F_STA) _sta(cp);
	if(flags & F_SCJ) _scj(cp);
	if(flags & F_SER) _ser(cp);
}

static int fsm(ppp_cp_t *cp)
{
	int8_t next_state;

	/* Call the appropiate functions for each state */
	_event_action(cp);
	/* Set next state */
	next_state = state_trans[cp->state][cp->event];

	/* Keep in same state if there's something wrong (RFC 1661) */
	if(next_state != S_UNDEF){
		cp->state = next_state;
	}
	return 0; /*TODO*/
}
