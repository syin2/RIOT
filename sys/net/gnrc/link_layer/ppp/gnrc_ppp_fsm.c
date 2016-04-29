#include <errno.h>

#include "msg.h"
#include "thread.h"
#include "net/gnrc.h"
#include "net/ppptype.h"
#include "net/gnrc/ppp/ppp.h"
#include "net/gnrc/ppp/lcp.h"
#include "net/gnrc/ppp/ipcp.h"
#include "net/gnrc/ppp/fsm.h"
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

void set_timeout(ppp_fsm_t *cp, uint32_t time)
{
	cp->msg.type = PPPDEV_MSG_TYPE_EVENT;
	cp->msg.content.value = (((ppp_protocol_t*)cp)->id<<8) +PPP_TIMEOUT;
	xtimer_set_msg(&cp->xtimer, cp->restart_timer, &cp->msg, thread_getpid());
}

gnrc_pktsnip_t *build_options(ppp_fsm_t *cp)
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

static uint8_t get_scnpkt_data(ppp_fsm_t *cp, gnrc_pktsnip_t *pkt, uint16_t *n)
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

static void build_nak_pkt(ppp_fsm_t *cp, gnrc_pktsnip_t *pkt, uint8_t *buf)
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

static void build_rej_pkt(ppp_fsm_t *cp, gnrc_pktsnip_t *pkt, uint8_t *buf)
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
static void _event_action(ppp_fsm_t *cp, uint8_t event, gnrc_pktsnip_t *pkt) 
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

int trigger_event(ppp_fsm_t *cp, int event, gnrc_pktsnip_t *pkt)
{
	if (event < 0)
	{
		return -EBADMSG;
	}
	int next_state;
	next_state = state_trans[event][cp->state];
	DEBUG("%i> ", ((ppp_protocol_t*)cp)->id);
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

void tlu(ppp_fsm_t *cp, void *args)
{
	DEBUG("%i", ((ppp_protocol_t*) cp)->id);
	DEBUG("> This layer up (a.k.a Successfully negotiated Link)\n");
	send_fsm_msg(&cp->msg, (cp->targets) & 0xffff, PPP_LINKUP);
	(void) cp;
}

void tld(ppp_fsm_t *cp, void *args)
{
	DEBUG("%i", ((ppp_protocol_t*) cp)->id);
	DEBUG("> This layer down\n");
	(void) cp;
}

void tls(ppp_fsm_t *cp, void *args)
{
	DEBUG("%i", ((ppp_protocol_t*) cp)->id);
	DEBUG(">  This layer started\n");
	(void) cp;
}

void tlf(ppp_fsm_t *cp, void *args)
{
	DEBUG("%i", ((ppp_protocol_t*) cp)->id);
	DEBUG(">  This layer finished\n");
	send_fsm_msg(&cp->msg, (cp->targets) & 0xffff, PPP_LINKDOWN);
	(void) cp;
}

void irc(ppp_fsm_t *cp, void *args)
{
	DEBUG("%i", ((ppp_protocol_t*) cp)->id);
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
void zrc(ppp_fsm_t *cp, void *args)
{
	DEBUG("%i", ((ppp_protocol_t*) cp)->id);
	DEBUG(">  Zero restart counter\n ");
	(void) cp;
	cp->restart_counter = 0;
	set_timeout(cp, cp->restart_timer);
}


void scr(ppp_fsm_t *cp, void *args)
{
	DEBUG("%i", ((ppp_protocol_t*) cp)->id);
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

void sca(ppp_fsm_t *cp, void *args)
{
	gnrc_pktsnip_t *pkt = (gnrc_pktsnip_t*) args;
	DEBUG("%i", ((ppp_protocol_t*) cp)->id);
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

void scn(ppp_fsm_t *cp, void *args)
{
	gnrc_pktsnip_t *pkt = (gnrc_pktsnip_t*) args;
	DEBUG("%i", ((ppp_protocol_t*) cp)->id);
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

void str(ppp_fsm_t *cp, void *args)
{
	DEBUG("%i", ((ppp_protocol_t*) cp)->id);
	DEBUG(">  Sending Terminate Request\n");
	(void) cp;
	gnrc_pktsnip_t *send_pkt = pkt_build(cp->prottype, PPP_TERM_REQ, cp->tr_sent_identifier++, NULL);
	gnrc_ppp_send(cp->dev->netdev, send_pkt);
}

void sta(ppp_fsm_t *cp, void *args)
{ 
	gnrc_pktsnip_t *pkt = (gnrc_pktsnip_t*) args;
	DEBUG("%i", ((ppp_protocol_t*) cp)->id);
	DEBUG(">  Sending Terminate Ack\n");
	gnrc_pktsnip_t *recv_pkt = NULL;

	ppp_hdr_t *recv_ppp_hdr;
	int has_data = _pkt_get_ppp_header(pkt, &recv_ppp_hdr);
	if(has_data)
	{
		recv_pkt = gnrc_pktbuf_add(NULL, pkt->data, pkt->size, GNRC_NETTYPE_UNDEF);	
	}
	gnrc_pktsnip_t *send_pkt = pkt_build(cp->prottype, PPP_TERM_ACK, ppp_hdr_get_id(recv_ppp_hdr), recv_pkt);
	gnrc_ppp_send(cp->dev->netdev, send_pkt);
}
void scj(ppp_fsm_t *cp, void *args)
{
	gnrc_pktsnip_t *pkt = (gnrc_pktsnip_t*) args;
	(void) pkt;
	DEBUG("%i", ((ppp_protocol_t*) cp)->id);
	DEBUG(">  Sending Code Rej\n");
}
void ser(ppp_fsm_t *cp, void *args)
{
	gnrc_pktsnip_t *pkt = (gnrc_pktsnip_t*) args;
	DEBUG("%i", ((ppp_protocol_t*) cp)->id);
	DEBUG(">  Sending Echo/Discard/Replay\n");
	(void) cp;
	(void) pkt;
	//send_cp(cp,PPP_CP_SER);
}

int cp_init(gnrc_pppdev_t *ppp_dev, ppp_fsm_t *cp)
{
	cp->state = S_INITIAL;
	cp->cr_sent_identifier = 0;
	cp->dev = ppp_dev;
	return 0;
}

static int _opt_is_ack(ppp_fsm_t *cp, ppp_option_t *opt)
{
	cp_conf_t *curr_conf=NULL;
	curr_conf = cp->get_conf_by_code(cp, ppp_opt_get_type(opt));
	return curr_conf && curr_conf->is_valid(opt);
}

int _pkt_get_ppp_header(gnrc_pktsnip_t *pkt, ppp_hdr_t **ppp_hdr)
{
	if(pkt->type == GNRC_NETTYPE_UNDEF)
	{
		*ppp_hdr = (ppp_hdr_t*) pkt->next->data;
		return true;
	}
	else
	{
		*ppp_hdr = (ppp_hdr_t*) pkt->data;
		return false;
	}
}

int handle_rcr(ppp_fsm_t *cp, gnrc_pktsnip_t *pkt)
{
	/* If the packet doesn't have options, it's considered as valid. */
	if(!pkt)
	{
		return E_RCRp;
	}

	/* Check if options are valid */
	if (ppp_conf_opts_valid(pkt, pkt->size) <= 0)
	{
		return -EBADMSG;
	}

	ppp_option_t *head = (ppp_option_t*) pkt->data;
	ppp_option_t *curr_opt = head;


	while(curr_opt)
	{
		if(!_opt_is_ack(cp, curr_opt))
			return E_RCRm;
		curr_opt = ppp_opt_get_next(curr_opt, head, pkt->size);
	}

	/*Check if there's an option that is required but not sent */
	curr_opt = head;
	cp_conf_t *curr_conf = cp->conf;

	
	uint8_t found;
	while(curr_conf)
	{
		if(!(curr_conf->flags & OPT_REQUIRED))
		{
			curr_conf = curr_conf->next;
			continue;
		}
		found = false;
		while(curr_opt)
		{
			if(curr_conf->type == ppp_opt_get_type(curr_opt))
			{
				found = true;
				break;
			}
		}
		if(!found)
			return E_RCRm;

		curr_opt = ppp_opt_get_next(curr_opt, head, pkt->size);
	}

	/* Valid options... set them before SCA */
	curr_opt = head;
	while(curr_opt)
	{
		curr_conf = cp->get_conf_by_code(cp, ppp_opt_get_type(curr_opt));
		if(curr_conf)
			curr_conf->set(cp, curr_opt, true);
		else
			DEBUG("handle_rcr inconsistency in pkt. Shouldn't happen\n");
		curr_opt = ppp_opt_get_next(curr_opt, head, pkt->size);
	}

	return E_RCRp;
}

int handle_rca(ppp_fsm_t *cp, ppp_hdr_t *hdr, gnrc_pktsnip_t *pkt)
{
	uint8_t pkt_id = ppp_hdr_get_id(hdr);
	uint8_t pkt_length = ppp_hdr_get_length(hdr);

	void *opts = NULL;

	if (pkt)
	{
		if (ppp_conf_opts_valid(pkt, pkt->size) <= 0)
		{
			return -EBADMSG;
		}
		opts = pkt->data;
	}

	if (pkt_id != cp->cr_sent_identifier || (pkt && memcmp(cp->cr_sent_opts, opts, pkt_length-sizeof(ppp_hdr_t))))
	{
		return -EBADMSG;
	}

	/*Write options in corresponding devices*/
	if (pkt)
	{
		ppp_option_t *head = (ppp_option_t*) opts;
		ppp_option_t *curr_opt = head;
		cp_conf_t *conf;
		while(curr_opt)
		{
			conf = cp->get_conf_by_code(cp, ppp_opt_get_type(curr_opt));
			if(!conf)
			{
				/*Received invalid ACK*/
				DEBUG("Peer sent inconsistent ACK\n");
				return -EBADMSG;
			}
			conf->set(cp, curr_opt, false);
			curr_opt = ppp_opt_get_next(curr_opt, head, pkt->size);
		}
	}
	return E_RCA;
}

int handle_rcn_nak(ppp_fsm_t *cp, ppp_hdr_t *hdr, gnrc_pktsnip_t *pkt)
{
	if(!pkt)
	{
		/* If the packet doesn't have options, it's considered as invalid. */
		return -EBADMSG;
	}

	/* Check if options are valid */
	if (ppp_conf_opts_valid(pkt, pkt->size) <= 0)
	{
		return -EBADMSG;
	}


	if (ppp_hdr_get_id(hdr) != cp->cr_sent_identifier)
		return -EBADMSG;

	/*Handle nak for each option*/
	ppp_option_t *head = pkt->data;
	ppp_option_t *curr_opt = head;

	cp_conf_t *curr_conf;
	while(curr_opt)
	{
		curr_conf = cp->get_conf_by_code(cp, ppp_opt_get_type(curr_opt));
		if(curr_conf != NULL)
			curr_conf->handle_nak(curr_conf, curr_opt);
		curr_opt = ppp_opt_get_next(curr_opt, head, pkt->size);
	}
	return E_RCN;
}

int handle_rcn_rej(ppp_fsm_t *cp, ppp_hdr_t *hdr, gnrc_pktsnip_t *pkt)
{
	if(!pkt || ppp_hdr_get_id(hdr) != cp->cr_sent_identifier || ppp_conf_opts_valid(pkt, pkt->size) <= 0 || ppp_hdr_get_length(hdr)-sizeof(ppp_hdr_t) > cp->cr_sent_size)
		return -EBADMSG;


	ppp_option_t *head = pkt->data;
	ppp_option_t *curr_opt = head;
	uint16_t size = cp->cr_sent_size;

	while(curr_opt)
	{
		if(!ppp_opt_is_subset(curr_opt, cp->cr_sent_opts, size))
			return -EBADMSG;
		curr_opt = ppp_opt_get_next(curr_opt, head, size);
	}

	/* Disable every REJ option */
	curr_opt = head;
	cp_conf_t *curr_conf;
	while(curr_opt)
	{
		curr_conf = cp->get_conf_by_code(cp, ppp_opt_get_type(curr_opt));
		if(curr_conf == NULL)
		{
			DEBUG("This shouldn't happen...\n");
			return -EBADMSG;
		}
		curr_conf->flags &= ~OPT_ENABLED;
		curr_opt = ppp_opt_get_next(curr_opt, head, pkt->size);
	}
	return E_RCN;
}

int handle_coderej(ppp_hdr_t *hdr, gnrc_pktsnip_t *pkt)
{
	/* Generate ppp packet from payload */
	/* Mark ppp headr */

	gnrc_pktbuf_mark(pkt, sizeof(ppp_hdr_t), GNRC_NETTYPE_UNDEF);
	ppp_hdr_t *rej_hdr = (ppp_hdr_t*) pkt->data;

	uint8_t code = ppp_hdr_get_code(rej_hdr);
	if (code >= PPP_CONF_REQ && code <= PPP_TERM_ACK)
	{
		return E_RXJm;
	}
	else
	{
		return E_RXJp;
	}

}


int handle_term_ack(ppp_fsm_t *cp, gnrc_pktsnip_t *pkt)
{
	ppp_hdr_t *ppp_hdr = pkt->data;
	
	int id = ppp_hdr_get_id(ppp_hdr);
	if(id == cp->tr_sent_identifier)
	{
		return E_RTA;
	}
	return -EBADMSG;
}


static int handle_conf(ppp_fsm_t *cp, int type, gnrc_pktsnip_t *pkt)
{
	gnrc_pktsnip_t *ppp_hdr = gnrc_pktbuf_mark(pkt, sizeof(ppp_hdr_t), cp->prottype);
	gnrc_pktsnip_t *payload = pkt->type == cp->prottype ? NULL : pkt;
	print_pkt(ppp_hdr->next, ppp_hdr, payload);

	ppp_hdr_t *hdr = (ppp_hdr_t*) ppp_hdr->data;

	int event;

	switch(type)
	{
		case PPP_CONF_REQ:
			event = handle_rcr(cp, pkt);
			break;
		case PPP_CONF_ACK:
			event = handle_rca(cp, hdr, pkt);
			break;
		case PPP_CONF_NAK:
			event = handle_rcn_nak(cp, hdr, pkt);
			break;
		case PPP_CONF_REJ:
			event = handle_rcn_rej(cp, hdr, pkt);
			break;
		default:
			DEBUG("Shouldn't be here...\n");
			return -EBADMSG;
			break;
	}
	return event;
}

int fsm_event_from_pkt(ppp_fsm_t *cp, gnrc_pktsnip_t *pkt)
{
	ppp_hdr_t *hdr = (ppp_hdr_t*) pkt->data;

	int code = ppp_hdr_get_code(hdr);
	int supported = cp->supported_codes & (1<<(code-1));
	int type = supported ? code : PPP_UNKNOWN_CODE; 

	int event;

	DEBUG("<<<<<<<<<< RECV:");
	switch(type){
		case PPP_CONF_REQ:
		case PPP_CONF_ACK:
		case PPP_CONF_NAK:
		case PPP_CONF_REJ:
			event = handle_conf(cp, type, pkt);
			break;
		case PPP_TERM_REQ:
			event = E_RTR;
			break;
		case PPP_TERM_ACK:
			event = handle_term_ack(cp, pkt);
			break;
		case PPP_CODE_REJ:
			event = handle_coderej(hdr, pkt);
			break;
		case PPP_ECHO_REQ:
		case PPP_ECHO_REP:
		case PPP_DISC_REQ:
			event = E_RXR;
			break;
		default:
			event = E_RUC;
			break;
	}

	return event;
}


int fsm_handle_ppp_msg(struct ppp_protocol_t *protocol, uint8_t ppp_event, void *args)
{
	ppp_fsm_t *target = (ppp_fsm_t*) protocol;
	uint8_t event;
	gnrc_pktsnip_t *pkt = (gnrc_pktsnip_t*) args;
	switch(ppp_event)
	{
		case PPP_RECV:
			event = fsm_event_from_pkt(target, pkt);
			trigger_event(target, event, pkt);
			/*TODO: Fix this*/
			gnrc_pktbuf_release(pkt);
			break;
		case PPP_LINKUP:
			DEBUG("Event: PPP_LINKUP\n");
			/*Set here PPP states...*/
			trigger_event(target, E_OPEN, NULL);
			trigger_event(target, E_UP, NULL);
			break;
		case PPP_LINKDOWN:
			/* Just to test, print message when this happens */
			DEBUG("Some layer finished\n");
			break;
		case PPP_TIMEOUT:
			if(target->restart_counter)
			{
				DEBUG("Event: TO+\n");
				trigger_event(target, E_TOp, NULL);
			}
			else
			{
				DEBUG("Event: TO-\n");
				trigger_event(target, E_TOm, NULL);
			}
			break;
	}
	return 0;
}
void send_fsm_msg(msg_t *msg, uint8_t target, uint8_t event)
{
	DEBUG("Sending msg to upper layer...\n");
	msg->type = PPPDEV_MSG_TYPE_EVENT;
	msg->content.value = (target<<8) + event;
	msg_send(msg, thread_getpid());
}
