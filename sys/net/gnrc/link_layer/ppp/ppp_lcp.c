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

#include "net/ppp/lcp.h"


static void _state_initial(ppp_layer_t *l_lcp))
{
	if(l_lcp->events & S_UP)
	{
		l_lcp->next_state = PPP_STATE_CLOSED;
	}
	else if (l_lcp->events & S_OPEN)
	{
		tls(l_lcp);
		l_lcp->next_state = PPP_STATE_STARTING;
	}
	else if (l_lcp->events & S_CLOSE)
	{
		l_lcp->next_state = PPP_STATE_INITIAL;
	}
}

static void _state_starting(ppp_layer_t *l_lcp))
{
	if(l_lcp->events & S_UP)
	{
		irc(l_lcp);
		scr(l_lcp);
		l_lcp->next_state = PPP_STATE_REQ_SENT;
	}
	else if(l_lcp->events & S_OPEN)
	{
		l_lcp->next_state = PPP_STATE_STARTING;
	}
	else if(l_lcp->events & S_CLOSE)
	{
		tlf(l_lcp);
		l_lcp->next_state = PPP_STATE_INITIAL;
	}
}

static void _state_closed(ppp_layer_t *l_lcp))
{
	if(l_lcp->events & S_DOWN)
	{
		l_lcp->next_state = PPP_STATE_INITIAL;
	}
	else if(l_lcp->events & S_OPEN)
	{
		irc(l_lcp);
		scr(l_lcp);
		l_lcp->next->state = PPP_STATE_REQ_SENT;
	}
	else if(l_lcp->events & (S_CLOSE | S_RTA | S_RXJp | S_RXR))
	{
		l_lcp->next_state = PPP_STATE_CLOSED;
	}
	else if(l_lcp->events & (S_RCRp | S_RCRm | S_RCA | S_RCN | S_RTR))
	{
		sta(l_lcp);
		l_lcp->next_state = PPP_STATE_CLOSED;
	}
	else if(l_lcp->events & S_RUC)
	{
		scj(l_lcp);
		l_lcp->next_state = PPP_STATE_CLOSED;
	}
	else if(l_lcp->events & S_RXJm)
	{
		tlf(l_lcp);
		l_lcp->next_state = PPP_STATE_CLOSED;
	}
}

static void _state_stopped(ppp_layer_t *l_lcp))
{
	if(l_lcp->events & S_DOWN)
	{
		tls(l_lcp);
		l_lcp->next_state = PPP_STATE_STARTING;
	}
	else if(l_lcp->events & S_OPEN)
	{
		/* Passive? */
		l_lcp->next_state = PPP_STATE_STOPPED;
	}
	else if(l_lcp->events & S_CLOSE)
	{
		l_lcp->next_state = PPP_STATE_CLOSED;
	}
	else if(l_lcp->events & S_RCRp)
	{
		irc(l_lcp);
		scr(l_lcp);
		sca(l_lcp);
		lcp->next_state = PPP_STATE_ACK_SENT;
	}
	else if(l_lcp->events & S_RCRm)
	{
		irc(l_lcp);
		scr(l_lcp);
		sca(l_lcp);
		lcp->next_state = PPP_STATE_REQ_SENT;
	}
	else if(l_lcp->events & (S_RCA | S_RCN | S_RTR)
	{
		sta(l_lcp);
		lcp->next_state = PPP_STATE_STOPPED;
	}
	else if(l_lcp->events & (S_RTA | S_RXJp | S_RXR))
	{
		l_lcp->next_state = PPP_STATE_STOPPED;
	}
	else if(l_lcp->events & S_RUC)
	{
		scj(l_lcp);
		l_lcp->next_state = PPP_STATE_STOPPED;
	}
	else if(l_lcp->events & S_RXJm)
	{
		tlf(l_lcp);
		l_lcp->next_state = PPP_STATE_STOPPED;
	}
}

static void _state_closing(ppp_layer_t *l_lcp))
{
	if(l_lcp->events & S_DOWN)
	{
		l_lcp->next_state = PPP_STATE_INITIAL;
	}
	else if(l_lcp->events & S_OPEN)
	{
		/* Passive? */
		l_lcp->next_state = PPP_STATE_STOPPING;
	}
	else if(l_lcp->events & S_CLOSE)
	{
		l_lcp->next_state = PPP_STATE_CLOSING;
	}
	else if(l_lcp->events & S_TOp)
	{
		str(l_lcp);
		l_lcp->next_state = PPP_STATE_CLOSING;
	}
	else if(l_lcp->events & S_TOm)
	{
		tlf(l_lcp);
		l_lcp->next_state = PPP_STATE_CLOSED;
	}
	else if(l_lcp->events & (S_RCRp | S_RCRm | S_RCA | S_RCN | S_RXJp | S_RXR))
	{
		l_lcp->next_state = PPP_STATE_CLOSING;
	}
	else if(l_lcp->events & S_RTR)
	{
		sta(l_lcp);
		l_lcp->next_state = PPP_STATE_CLOSING;
	}
	else if(l_lcp->events & S_RTA)
	{
		tlf(l_lcp);
		l_lcp->next_state = PPP_STATE_CLOSED;
	}
	else if(l_lcp->events & S_RUC)
	{
		scj(l_lcp);
		l_lcp->next_state = PPP_STATE_CLOSING;
	}
	else if(l_lcp->events & S_RXJm)
	{
		tlf(l_lfc);
		l_lcp->next_state = PPP_STATE_CLOSED;
	}
}
static void _state_stopping(ppp_layer_t *l_lcp))
{
	if(l_lcp->events & S_DOWN)
	{
		l_lcp->next_state = PPP_STATE_STARTING;	
	}
	else if(l_lcp->events & S_OPEN)
	{
		/* Passive? */
		l_lcp->next_state = PPP_STATE_STOPPING;
	}
	else if(l_lcp->events & S_CLOSE)
	{
		l_lcp->next_state = PPP_STATE_CLOSING;
	}
	else if(l_lcp->events & S_TOp)
	{
		str(l_lcp9;
		l_lcp->next_state = PPP_STATE_STOPPING;
	}
	else if(l_lcp->events & S_TOm)
	{
		tlf(l_lcp);
		l_lcp->next_state = PPP_STATE_STOPPED;
	}
	else if(l_lcp->events & (S_RCRp | S_RCRm | S_RCA | S_RCN | S_RXJp | S_RXR))
	{
		l_lcp->next_state = PPP_STATE_STOPPING;
	}
	else if(l_lcp->events & S_RTR)
	{
		sta(l_lcp);
		l_lcp->next_state = PPP_STATE_STOPPING;
	}
	else if(l_lcp->events & S_RTA)
	{
		tlf(l_lcp);
		l_lcp->next_state = PPP_STATE_STOPPED;
	}
	else if(l_lcp->events & S_RUC)
	{
		scj(l_lcp);
		l_lcp->next_state = PPP_STATE_STOPPING;
	}
	else if(l_lcp->events & S_RXJm)
	{
		tlf(l_lcp);
		l_lcp->next_state = PPP_STATE_STOPPED;
	}
}
static void _state_req_sent(ppp_layer_t *l_lcp))
{
	if(l_lcp->events & S_DOWN)
	{
		
	}
	else if(l_lcp->events & S_OPEN)
	{
		
	}
	else if(l_lcp->events & S_CLOSE)
	{
		
	}
	else if(l_lcp->events & S_TOp)
	{
		
	}
	else if(l_lcp->events & S_TOm)
	{
		
	}
	else if(l_lcp->events & S_RCRp)
	{
		
	}
	else if(l_lcp->events & S_RCRm)
	{
		
	}
	else if(l_lcp->events & S_RCA)
	{
		
	}
	else if(l_lcp->events & S_RCN)
	{
		
	}
	else if(l_lcp->events & S_RTR)
	{
		
	}
	else if(l_lcp->events & S_RTA)
	{
		
	}
	else if(l_lcp->events & S_RUC)
	{
		
	}
	else if(l_lcp->events & S_RXJp)
	{
		
	}
	else if(l_lcp->events & S_RXJm)
	{
		
	}
	else if(l_lcp->events & S_RXR)
	{
		
	}
}
static void _state_ack_rcvd(ppp_layer_t *l_lcp))
{
	if(l_lcp->events & S_DOWN)
	{
		
	}
	else if(l_lcp->events & S_OPEN)
	{
		
	}
	else if(l_lcp->events & S_CLOSE)
	{
		
	}
	else if(l_lcp->events & S_RCRp)
	{
		
	}
	else if(l_lcp->events & S_RCRm)
	{
		
	}
	else if(l_lcp->events & S_RCA)
	{
		
	}
	else if(l_lcp->events & S_RCN)
	{
		
	}
	else if(l_lcp->events & S_RTR)
	{
		
	}
	else if(l_lcp->events & S_RTA)
	{
		
	}
	else if(l_lcp->events & S_RUC)
	{
		
	}
	else if(l_lcp->events & S_RXJp)
	{
		
	}
	else if(l_lcp->events & S_RXJm)
	{
		
	}
	else if(l_lcp->events & S_RXR)
	{
		
	}
}

static void _state_ack_sent(ppp_layer_t *l_lcp))
{
	if(l_lcp->events & S_DOWN)
	{
		
	}
	else if(l_lcp->events & S_OPEN)
	{
		
	}
	else if(l_lcp->events & S_CLOSE)
	{
		
	}
	else if(l_lcp->events & S_RCRp)
	{
		
	}
	else if(l_lcp->events & S_RCRm)
	{
		
	}
	else if(l_lcp->events & S_RCA)
	{
		
	}
	else if(l_lcp->events & S_RCN)
	{
		
	}
	else if(l_lcp->events & S_RTR)
	{
		
	}
	else if(l_lcp->events & S_RTA)
	{
		
	}
	else if(l_lcp->events & S_RUC)
	{
		
	}
	else if(l_lcp->events & S_RXJp)
	{
		
	}
	else if(l_lcp->events & S_RXJm)
	{
		
	}
	else if(l_lcp->events & S_RXR)
	{
		
	}
}
static void _state_opened(ppp_layer_t *l_lcp))
{
	if(l_lcp->events & S_DOWN)
	{
		
	}
	else if(l_lcp->events & S_OPEN)
	{
		
	}
	else if(l_lcp->events & S_CLOSE)
	{
		
	}
	else if(l_lcp->events & S_RCRp)
	{
		
	}
	else if(l_lcp->events & S_RCRm)
	{
		
	}
	else if(l_lcp->events & S_RCA)
	{
		
	}
	else if(l_lcp->events & S_RCN)
	{
		
	}
	else if(l_lcp->events & S_RTR)
	{
		
	}
	else if(l_lcp->events & S_RTA)
	{
		
	}
	else if(l_lcp->events & S_RUC)
	{
		
	}
	else if(l_lcp->events & S_RXJp)
	{
		
	}
	else if(l_lcp->events & S_RXJm)
	{
		
	}
	else if(l_lcp->events & S_RXR)
	{
		
	}
}

/* Call functions depending on function flag*/
static void _event_action(ppp_dev_t *dev,uint16_t flags) 
{
	if(flags & F_TLU) _tlu(dev);
	if(flags & F_TLD) _tld(dev);
	if(flags & F_TLS) _tls(dev);
	if(flags & F_TLF) _tlf(dev);
	if(flags & F_IRC) _irc(dev);
	if(flags & F_ZRC) _zrc(dev);
	if(flags & F_SRC) _src(dev);
	if(flags & F_SCA) _sca(dev);
	if(flags & F_SCN) _scn(dev);
	if(flags & F_STR) _str(dev);
	if(flags & F_STA) _sta(dev);
	if(flags & F_SCJ) _scj(dev);
	if(flags & F_SER) _ser(dev);
}
static int lcp_recv(ppp_layer_t *l_lcp)
{
	uint8_t next_state;

	/* Call the appropiate functions for each state */
	_event_action(actions[l_lcp->state][l_lcp->event])
	/* Set next state */
	next_state = state_trans[l_lcp->state][l_lcp->event]

	/* Shouldn't happen, but won't take the risk */
	/* Keep in same state if there's something wrong */
	if(next_state != S_UNDEF){
		l_lcp->state = next_state;
	}
}
static int _handle_conf_ack(ppt_dev_t *dev, gnrc_pktsnip_t *pkt)
{
	/* Retrieve peer negotiation */
	lcp_conf_t *opts = _retr_peer_neg(pkt);
	
	/* Shouldn't be necessary... but check if the response has the right negotiation params */
	if (opts->opts == dev->lcp_conf->opts)
	{
		/* Auth still not implemented. Go to NCP state...*/
		/* Change state to LCP's NCP */
		dev->state = LCPSTATE_NCP;

		/* Start negotiation of choosen NCP protocol*/
		ppp_ncp_start(dev)
	}
	else
	{
	}
	return 0;
}
