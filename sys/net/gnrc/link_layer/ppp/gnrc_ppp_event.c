
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


static int _opt_is_ack(ppp_cp_t *cp, ppp_option_t *opt)
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

int handle_rcr(ppp_cp_t *cp, ppp_hdr_t *hdr, gnrc_pktsnip_t *pkt)
{
	ppp_hdr_t *ppp_hdr;
	int has_options = _pkt_get_ppp_header(pkt, &ppp_hdr);

	/* If the packet doesn't have options, it's considered as valid. */
	if(!has_options)
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

		curr_opt = ppp_opt_get_next(curr_opt, head, pkt->size);;
	}

	return E_RCRp;
}

int handle_rca(ppp_cp_t *cp, ppp_hdr_t *hdr, gnrc_pktsnip_t *pkt)
{
	ppp_hdr_t *ppp_hdr;
	_pkt_get_ppp_header(pkt, &ppp_hdr);

	uint8_t pkt_id = ppp_hdr_get_id(ppp_hdr);
	uint8_t pkt_length = ppp_hdr_get_length(ppp_hdr);

	void *opts;

	int has_options = _pkt_get_ppp_header(pkt, &ppp_hdr);
	if (has_options)
	{
		if (ppp_conf_opts_valid(pkt, pkt->size) <= 0)
		{
			return -EBADMSG;
		}
		opts = pkt->data;
	}
	else
	{
		opts = pkt->next->data;
	}

	if (pkt_id != cp->cr_sent_identifier || memcmp(cp->cr_sent_opts, opts, pkt_length-sizeof(ppp_hdr_t)))
	{
		return -EBADMSG;
	}
	return E_RCA;
}

int handle_rcn_nak(ppp_cp_t *cp, ppp_hdr_t *hdr, gnrc_pktsnip_t *pkt)
{
	ppp_hdr_t *ppp_hdr;
	_pkt_get_ppp_header(pkt, &ppp_hdr);

	int has_options = _pkt_get_ppp_header(pkt, &ppp_hdr);

	if(!has_options)
	{
		/* If the packet doesn't have options, it's considered as invalid. */
		return -EBADMSG;
	}

	/* Check if options are valid */
	if (ppp_conf_opts_valid(pkt, pkt->size) <= 0)
	{
		return -EBADMSG;
	}


	if (ppp_hdr_get_id(ppp_hdr) != cp->cr_sent_identifier)
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

int handle_rcn_rej(ppp_cp_t *cp, ppp_hdr_t *hdr, gnrc_pktsnip_t *pkt)
{
	ppp_hdr_t *ppp_hdr;
	int has_options = _pkt_get_ppp_header(pkt, &ppp_hdr);

	if(!has_options)
		return -EBADMSG;

	if (ppp_hdr_get_id(ppp_hdr) != cp->cr_sent_identifier)
		return -EBADMSG;

	/* Check if options are valid */
	if (ppp_conf_opts_valid(pkt, pkt->size) <= 0)
	{
		return -EBADMSG;
	}

	/* Check if opts are subset of sent options */
	if(ppp_hdr_get_length(ppp_hdr)-sizeof(ppp_hdr_t) > cp->cr_sent_size)
	{
		return -EBADMSG;
	}

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


int handle_term_ack(ppp_cp_t *cp, ppp_hdr_t *hdr, gnrc_pktsnip_t *pkt)
{
	ppp_hdr_t *ppp_hdr;
	_pkt_get_ppp_header(pkt, &ppp_hdr);
	
	int id = ppp_hdr_get_id(ppp_hdr);
	if(id == cp->tr_sent_identifier)
	{
		return E_RTA;
	}
	return -EBADMSG;
}


int fsm_event_from_pkt(ppp_cp_t *cp, ppp_hdr_t *hdr, gnrc_pktsnip_t *pkt)
{
	int code = ppp_hdr_get_code(hdr);
	int supported = cp->supported_codes & (1<<(code-1));
	DEBUG("Supported: %i\n", supported);
	int type = supported ? code : PPP_UNKNOWN_CODE; 

	int event;
	switch(type){
		case PPP_CONF_REQ:
			event = handle_rcr(cp, hdr, pkt);
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
		case PPP_TERM_REQ:
			event = E_RTR;
			break;
		case PPP_TERM_ACK:
			event = handle_term_ack(cp, hdr, pkt);
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

