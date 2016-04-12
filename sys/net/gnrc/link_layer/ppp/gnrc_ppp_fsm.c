#include <errno.h>

#include "msg.h"
#include "thread.h"
#include "net/gnrc.h"
#include "net/ppptype.h"
#include "net/gnrc/ppp/ppp.h"
#include "net/gnrc/ppp/lcp.h"
#include "net/hdlc/hdr.h"
#include "net/ppp/hdr.h"
#include <errno.h>
#include <string.h>

#define ENABLE_DEBUG    (1)
#include "debug.h"

#if ENABLE_DEBUG
/* For PRIu16 etc. */
#include <inttypes.h>
#endif

gnrc_pktsnip_t *build_options(ppp_cp_t *cp)
{
	size_t size=0;
	cp_conf_t *opt = cp->conf;

	while(opt)
	{
		if(opt->flags & OPT_ENABLED)
		{
			size += 2+opt->size;
		}
		opt = opt->next;
	}

	if(!size)
		return NULL;

	gnrc_pktsnip_t *opts = gnrc_pktbuf_add(NULL, NULL, size, GNRC_NETTYPE_UNDEF);

	opt = cp->conf;

	int cursor=0;
	while(opt)
	{
		if(opt->flags & OPT_ENABLED)
		{
			*(((uint8_t*)opts->data)+cursor) = opt->type;
			*(((uint8_t*)opts->data)+cursor+1) = opt->size+2;
			memcpy(opts->data+2+cursor, opt->value, opt->size);	
			cursor+=2+opt->size;
		}
		opt = opt->next;
	}
	return opts;
}

uint8_t build_nakrej(ppp_cp_t *cp, gnrc_pktsnip_t *pkt, gnrc_pktsnip_t **nak)
{
	uint8_t code = PPP_CONF_NAK;
	uint8_t status;
	int pkt_length = pkt->size;
	*nak = gnrc_pktbuf_add(NULL, NULL,pkt_length, GNRC_NETTYPE_UNDEF);

	ppp_option_t *head = (ppp_option_t*) pkt->data;
	ppp_option_t *curr_opt = head;

	uint8_t opt_length;
	int cursor = 0;

	print_pkt(*nak);
	while(curr_opt)
	{
		opt_length = ppp_opt_get_length(curr_opt);

		status = cp->get_opt_status(curr_opt, false);
		switch(status)
		{
			case CP_CREQ_ACK:
				break;
			case CP_CREQ_NAK:
				if(code == PPP_CONF_NAK)
				{
					cp->get_opt_status(curr_opt, true);
					memmove((*nak)->data+cursor, curr_opt, opt_length);
					cursor+=opt_length;
				}
				break;
			case CP_CREQ_REJ:
				if(code == PPP_CONF_REJ)
				{
					cursor+=opt_length;
				}
				else
				{
					cursor = 0;
					code = PPP_CONF_REJ;
				}
				memmove((*nak)->data+cursor, curr_opt, opt_length);
				break;
		}
		curr_opt = ppp_opt_get_next(curr_opt, head, pkt_length);
	}
	gnrc_pktbuf_realloc_data(*nak, (size_t) cursor);
	return code;
}
static void print_state(int state)
{
	switch(state)
	{
		case S_UNDEF:
			DEBUG("UNDEF");
			break;
		case S_INITIAL:
			DEBUG("INITIAL");
			break;
		case S_STARTING:
			DEBUG("STARTING");
			break;
		case S_CLOSED:
			DEBUG("CLOSED");
			break;
		case S_STOPPED:
			DEBUG("STOPPED");
			break;
		case S_CLOSING:
			DEBUG("CLOSING");
			break;
		case S_STOPPING:
			DEBUG("STOPPING");
			break;
		case S_REQ_SENT:
			DEBUG("REQ_SENT");
			break;
		case S_ACK_RCVD:
			DEBUG("ACK_RECV");
			break;
		case S_ACK_SENT:
			DEBUG("ACK_SENT");
			break;
		case S_OPENED:
			DEBUG("OPENED");
			break;
	}
}
static void print_event(uint8_t event)
{
	switch(event)
	{
		case E_UP:
			DEBUG("UP");
			break;
		case E_DOWN:
			DEBUG("DOWN");
			break;
		case E_OPEN:
			DEBUG("OPEN");
			break;
		case E_CLOSE:
			DEBUG("CLOSE");
			break;
		case E_TOp:
			DEBUG("TO+");
			break;
		case E_TOm:
			DEBUG("TO-");
			break;
		case E_RCRp:
			DEBUG("RCR+");
			break;
		case E_RCRm:
			DEBUG("RCR-");
			break;
		case E_RCA:
			DEBUG("RCA");
			break;
		case E_RCN:
			DEBUG("RCN");
			break;
		case E_RTR:
			DEBUG("RTR");
			break;
		case E_RTA:
			DEBUG("RTA");
			break;
		case E_RUC:
			DEBUG("RUC");
			break;
		case E_RXJp:
			DEBUG("RXJ+");
			break;
		case E_RXJm:
			DEBUG("RXJ-");
			break;
		case E_RXR:
			DEBUG("RXR");
			break;
	}
}
static void print_transition(int state, uint8_t event, int next_state)
{
	DEBUG("From state ");
	print_state(state);
	DEBUG(" with event ");
	print_event(event);
	DEBUG(". Next state is ");
	print_state(next_state);
	DEBUG("\n");
}
/* Call functions depending on function flag*/
static void _event_action(ppp_cp_t *cp, uint8_t event, gnrc_pktsnip_t *pkt) 
{
	int flags;

	flags = actions[event][cp->state];

	if(flags & F_TLU) tlu(cp, NULL);
	if(flags & F_TLD) tld(cp, NULL);
	if(flags & F_TLS) tls(cp, NULL);
	if(flags & F_TLF) tlf(cp, NULL);
	if(flags & F_IRC) irc(cp, (void*) &flags);
	if(flags & F_ZRC) zrc(cp, NULL);
	if(flags & F_SCR) scr(cp, (void*) pkt);
	if(flags & F_SCA) sca(cp, (void*) pkt);
	if(flags & F_SCN) scn(cp, (void*) pkt);
	if(flags & F_STR) str(cp, NULL);
	if(flags & F_STA) sta(cp, (void*) pkt);
	if(flags & F_SCJ) scj(cp, (void*) pkt);
	if(flags & F_SER) ser(cp, (void*) pkt);
}

int trigger_event(ppp_cp_t *cp, int event, gnrc_pktsnip_t *pkt)
{
	if (event < 0)
	{
		return -EBADMSG;
	}
	int next_state;
	next_state = state_trans[event][cp->state];
	print_transition(cp->state, event, next_state);

	/* Keep in same state if there's something wrong (RFC 1661) */
	if(next_state != S_UNDEF){
		_event_action(cp, event, pkt);
		cp->state = next_state;
	}
	else
	{
		DEBUG("Received illegal transition!\n");
	}
	/*Check if next state doesn't have a running timer*/
	if (cp->state < S_CLOSING || cp->state == S_OPENED)
		xtimer_remove(&cp->xtimer);
	return 0;
}

void tlu(ppp_cp_t *cp, void *args)
{
	DEBUG("%i", cp->prot);
	DEBUG("> This layer up (a.k.a Successfully negotiated Link)\n");
	(void) cp;
	//cp->l_upper_msg |= PPP_MSG_UP;
}

void tld(ppp_cp_t *cp, void *args)
{
	DEBUG("%i", cp->prot);
	DEBUG("> This layer down\n");
	(void) cp;
	//cp->l_upper_msg |= PPP_MSG_DOWN;
}

void tls(ppp_cp_t *cp, void *args)
{
	DEBUG("%i", cp->prot);
	DEBUG(">  This layer started\n");
	(void) cp;
	//cp->l_lower_msg |= PPP_MSG_UP;
}

void tlf(ppp_cp_t *cp, void *args)
{
	DEBUG("%i", cp->prot);
	DEBUG(">  This layer finished\n");
	(void) cp;
	//cp->l_lower_msg |= PPP_MSG_DOWN;
}

void irc(ppp_cp_t *cp, void *args)
{
	DEBUG("%i", cp->prot);
	DEBUG(">  Init Restart Counter\n");
	uint8_t cr = *((int*) args) & F_SCR; 

	if(cr)
	{
		cp->restart_counter = PPP_MAX_CONFIG;
	}
	else
	{
		cp->restart_counter = PPP_MAX_TERMINATE;
	}
}
void zrc(ppp_cp_t *cp, void *args)
{
	DEBUG("%i", cp->prot);
	DEBUG(">  Zero restart counter\n ");
	(void) cp;
	//cp->restart_counter = 0;
	/* Set timer to appropiate value TODO*/
}

void scr(ppp_cp_t *cp, void *args)
{
	DEBUG("%i", cp->prot);
	DEBUG(">  Sending Configure Request\n");
	/* Decrement configure counter */
	cp->restart_counter -= 1;

	gnrc_pktsnip_t *opts = build_options(cp);
	gnrc_pktsnip_t *pkt = pkt_build(cp->prot, PPP_CONF_REQ, ++cp->cr_sent_identifier,opts);
	
	if(opts)
		memcpy(cp->cr_sent_opts, opts->data, opts->size);

	/*Send packet*/
	gnrc_ppp_send(cp->dev->netdev, pkt);

	cp->msg.type = NETDEV2_MSG_TYPE_EVENT;
	cp->msg.content.value = 0x0100 +PPP_TIMEOUT;
	xtimer_set_msg(&cp->xtimer, cp->restart_timer, &cp->msg, thread_getpid());
}

void sca(ppp_cp_t *cp, void *args)
{
	gnrc_pktsnip_t *pkt = (gnrc_pktsnip_t*) args;
	DEBUG("%i", cp->prot);
	DEBUG(">  Sending Configure Ack\n");
	ppp_hdr_t *recv_ppp_hdr;

	gnrc_pktsnip_t *opts = NULL;
	int has_options = _pkt_get_ppp_header(pkt, &recv_ppp_hdr);

	if(has_options)
	{
		DEBUG(">> Received pkt asked for options. Send them back, with ACK pkt\n");
		opts = gnrc_pktbuf_add(NULL, pkt->data, pkt->size, GNRC_NETTYPE_UNDEF);;
	}
	else
	{
		DEBUG(">> Received pkt didn't ask for options -> So just ACK\n");
	}

	gnrc_pktsnip_t *send_pkt = pkt_build(cp->prot, PPP_CONF_ACK, ppp_hdr_get_id(recv_ppp_hdr),opts);
	
	/*Send packet*/
	gnrc_ppp_send(cp->dev->netdev, send_pkt);
}

void scn(ppp_cp_t *cp, void *args)
{
	gnrc_pktsnip_t *pkt = (gnrc_pktsnip_t*) args;
	DEBUG("%i", cp->prot);
	DEBUG(">  Sending Configure Nak/Rej\n");

	gnrc_pktsnip_t *nak;
	uint8_t type = build_nakrej(cp, pkt, &nak);

	ppp_hdr_t *recv_ppp_hdr = (ppp_hdr_t*) pkt->next->data;
	gnrc_pktsnip_t *send_pkt = pkt_build(cp->prot, type, ppp_hdr_get_id(recv_ppp_hdr),nak);
	
	/*Send packet*/
	gnrc_ppp_send(cp->dev->netdev, send_pkt);
}

void str(ppp_cp_t *cp, void *args)
{
	DEBUG("%i", cp->prot);
	DEBUG(">  Sending Terminate Request\n");
	(void) cp;
#if 0
	int id = 666; /*TODO*/
	gnrc_pktsnip_t pkt;
	pkt->hdr->code = PPP_CP_TERM_REQUEST;
	pkt->hdr->id = id;
	pkt->hdr->length = 4;
	pkt->opts->num_opts = 0;
	send_cp(cp, pkt);
#endif
}

void sta(ppp_cp_t *cp, void *args)
{ 
	gnrc_pktsnip_t *pkt = (gnrc_pktsnip_t*) args;
	DEBUG("%i", cp->prot);
	DEBUG(">  Sending Terminate Ack\n");
	(void) cp;
	(void) pkt;
#if 0
	int id = 666; /*TODO*/
	gnrc_pktsnip_t pkt;
	pkt->hdr->code = PPP_CP_TERM_ACK;
	pkt->hdr->id = id;
	pkt->hdr->length = 4;
	pkt->opts->num_opts = 0;
	send_cp(cp, pkt);
#endif
}
void scj(ppp_cp_t *cp, void *args)
{
	gnrc_pktsnip_t *pkt = (gnrc_pktsnip_t*) args;
	DEBUG("%i", cp->prot);
	DEBUG(">  Sending Code Rej\n");
	(void) cp;
	(void) pkt;
	//send_cp(cp, PPP_CP_CODE_REJ);
}
void ser(ppp_cp_t *cp, void *args)
{
	gnrc_pktsnip_t *pkt = (gnrc_pktsnip_t*) args;
	DEBUG("%i", cp->prot);
	DEBUG(">  Sending Echo/Discard/Replay\n");
	(void) cp;
	(void) pkt;
	//send_cp(cp,PPP_CP_SER);
}

int cp_init(ppp_dev_t *ppp_dev, ppp_cp_t *cp)
{
	cp->state = S_INITIAL;
	cp->cr_sent_identifier = 0;
	cp->dev = ppp_dev;
	return 0;
}
