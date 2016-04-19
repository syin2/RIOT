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

void set_timeout(ppp_cp_t *cp, uint32_t time)
{
	cp->msg.type = NETDEV2_MSG_TYPE_EVENT;
	cp->msg.content.value = (cp->id<<8) +PPP_TIMEOUT;
	xtimer_set_msg(&cp->xtimer, cp->restart_timer, &cp->msg, thread_getpid());
}

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
			memcpy(opts->data+2+cursor, ((uint8_t*) &opt->value)+sizeof(uint32_t)-opt->size, opt->size);	
			cursor+=2+opt->size;
		}
		opt = opt->next;
	}
	return opts;
}

static uint8_t get_scnpkt_data(ppp_cp_t *cp, gnrc_pktsnip_t *pkt, uint16_t *n)
{
	ppp_option_t *head = pkt->data;
	ppp_option_t *curr_opt = head;

	uint8_t rej_size = 0;
	uint8_t nak_size = 0;
	uint8_t curr_type;

	cp_conf_t *curr_conf;
	uint8_t curr_size;

	while(curr_opt)
	{
		curr_type = ppp_opt_get_type(curr_opt);
		curr_conf = cp->get_conf_by_code(cp, curr_type);
		curr_size = ppp_opt_get_length(curr_opt);
		if(curr_conf == NULL)
		{
			rej_size += curr_size;
		}
		else if(!curr_conf->is_valid(curr_opt))
		{
			nak_size += curr_conf->build_nak_opts(NULL);
		}
		curr_opt = ppp_opt_get_next(curr_opt, head, pkt->size);
	}

	/* Append required options */
	curr_conf = cp->conf;

	while(curr_conf)
	{
		if(curr_conf->flags & OPT_REQUIRED)
			nak_size += curr_conf->size;
		curr_conf = curr_conf->next;
	}

	*n = rej_size ? rej_size : nak_size;
	return rej_size ? PPP_CONF_REJ : PPP_CONF_NAK;
}

static void build_nak_pkt(ppp_cp_t *cp, gnrc_pktsnip_t *pkt, uint8_t *buf)
{
	ppp_option_t *head = pkt->data;
	ppp_option_t *curr_opt = head;
	cp_conf_t *curr_conf;

	uint8_t curr_type;

	uint8_t cursor = 0;
	while(curr_opt)
	{
		curr_type = ppp_opt_get_type(curr_opt);
		curr_conf = cp->get_conf_by_code(cp, curr_type);

		if (curr_conf && !curr_conf->is_valid(curr_opt))
		{
			cursor += curr_conf->build_nak_opts(buf+cursor);	
		}
		curr_opt = ppp_opt_get_next(curr_opt, head, pkt->size);
	}
}

static void build_rej_pkt(ppp_cp_t *cp, gnrc_pktsnip_t *pkt, uint8_t *buf)
{
	ppp_option_t *head = pkt->data;
	ppp_option_t *curr_opt = head;
	cp_conf_t *curr_conf;

	uint8_t curr_type;
	uint16_t curr_size;

	uint8_t cursor = 0;
	while(curr_opt)
	{
		curr_type = ppp_opt_get_type(curr_opt);
		curr_conf = cp->get_conf_by_code(cp, curr_type);
		curr_size = ppp_opt_get_length(curr_opt);

		if(curr_conf == NULL)
		{
			memcpy(buf+cursor, curr_opt, curr_size);
			cursor += curr_size;
		}
		curr_opt = ppp_opt_get_next(curr_opt, head, pkt->size);
	}
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
	DEBUG("%i> ", cp->id);
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
	DEBUG("%i", cp->id);
	DEBUG("> This layer up (a.k.a Successfully negotiated Link)\n");
	broadcast_lower_layer(&cp->msg, cp->id, PPP_LINKUP);
	(void) cp;
}

void tld(ppp_cp_t *cp, void *args)
{
	DEBUG("%i", cp->id);
	DEBUG("> This layer down\n");
	(void) cp;
}

void tls(ppp_cp_t *cp, void *args)
{
	DEBUG("%i", cp->id);
	DEBUG(">  This layer started\n");
	(void) cp;
}

void tlf(ppp_cp_t *cp, void *args)
{
	DEBUG("%i", cp->id);
	DEBUG(">  This layer finished\n");
	(void) cp;
}

void irc(ppp_cp_t *cp, void *args)
{
	DEBUG("%i", cp->id);
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
	DEBUG("%i", cp->id);
	DEBUG(">  Zero restart counter\n ");
	(void) cp;
	//cp->restart_counter = 0;
	/* Set timer to appropiate value TODO*/
}


void scr(ppp_cp_t *cp, void *args)
{
	DEBUG("%i", cp->id);
	DEBUG(">  Sending Configure Request\n");

	/* Decrement configure counter */
	cp->restart_counter -= 1;

	/* Build options */
	gnrc_pktsnip_t *opts = build_options(cp);

	/*In case there are options, copy to sent opts*/
	if(opts)
	{
		memcpy(cp->cr_sent_opts, opts->data, opts->size);
		cp->cr_sent_size = opts->size;
	}

	/*Build pkt*/
	gnrc_pktsnip_t *pkt = pkt_build(cp->prottype, PPP_CONF_REQ, ++cp->cr_sent_identifier,opts);
	

	/*Send packet*/
	gnrc_ppp_send(cp->dev->netdev, pkt);
	set_timeout(cp, cp->restart_timer);
}

void sca(ppp_cp_t *cp, void *args)
{
	gnrc_pktsnip_t *pkt = (gnrc_pktsnip_t*) args;
	DEBUG("%i", cp->id);
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

	gnrc_pktsnip_t *send_pkt = pkt_build(cp->prottype, PPP_CONF_ACK, ppp_hdr_get_id(recv_ppp_hdr),opts);
	
	/*Send packet*/
	gnrc_ppp_send(cp->dev->netdev, send_pkt);
}

void scn(ppp_cp_t *cp, void *args)
{
	gnrc_pktsnip_t *pkt = (gnrc_pktsnip_t*) args;
	DEBUG("%i", cp->id);
	DEBUG(">  Sending Configure Nak/Rej\n");

	gnrc_pktsnip_t *opts;

	uint16_t scn_len; 
	uint8_t type = get_scnpkt_data(cp, pkt, &scn_len);

	opts = gnrc_pktbuf_add(NULL, NULL, scn_len, GNRC_NETTYPE_UNDEF);
	
	switch(type)
	{
		case PPP_CONF_NAK:
			build_nak_pkt(cp, pkt, (uint8_t*)opts->data);
			break;
		case PPP_CONF_REJ:
			build_rej_pkt(cp, pkt, (uint8_t*) opts->data);
			break;
		default:
			DEBUG("Shouldn't be here...\n");
			break;
	}

	ppp_hdr_t *recv_ppp_hdr = (ppp_hdr_t*) pkt->next->data;
	gnrc_pktsnip_t *send_pkt = pkt_build(cp->prottype, type, ppp_hdr_get_id(recv_ppp_hdr),opts);
	gnrc_ppp_send(cp->dev->netdev, send_pkt);
}

void str(ppp_cp_t *cp, void *args)
{
	DEBUG("%i", cp->id);
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
	DEBUG("%i", cp->id);
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
	DEBUG("%i", cp->id);
	DEBUG(">  Sending Code Rej\n");
	(void) cp;
	(void) pkt;
	//send_cp(cp, PPP_CP_CODE_REJ);
}
void ser(ppp_cp_t *cp, void *args)
{
	gnrc_pktsnip_t *pkt = (gnrc_pktsnip_t*) args;
	DEBUG("%i", cp->id);
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