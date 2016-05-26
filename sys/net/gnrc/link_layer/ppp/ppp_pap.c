#include "net/gnrc/ppp/pap.h"
#include "net/gnrc/ppp/ppp.h"

#define ENABLE_DEBUG    (1)
#include "debug.h"


gnrc_pktsnip_t *_pap_payload(pap_t *pap)
{
	gnrc_pktsnip_t *pkt = gnrc_pktbuf_add(NULL, NULL, 2+pap->user_size+pap->pass_size, GNRC_NETTYPE_UNDEF);

	*((uint8_t*) pkt->data) = pap->user_size;
	*(((uint8_t*) pkt->data)+1+pap->user_size) = pap->pass_size;

	memcpy(((uint8_t*) pkt->data)+1, pap->username, pap->user_size);
	memcpy(((uint8_t*) pkt->data)+2+pap->user_size, pap->password, pap->pass_size);
	return pkt;
}

int pap_handler(struct ppp_protocol_t *protocol, uint8_t ppp_event, void *args)
{
	pap_t *pap = (pap_t*) protocol;
	msg_t *msg = &protocol->msg;
	gnrc_pktsnip_t *pkt;
	gnrc_pktsnip_t *recv_pkt = (gnrc_pktsnip_t*) args;
	ppp_hdr_t *hdr = (ppp_hdr_t*) recv_pkt->data;
	xtimer_t *xtimer = &pap->xtimer;
	msg_t *timer_msg = &pap->timer_msg;
	lcp_t *lcp = (lcp_t*) protocol->pppdev->protocol[PROT_LCP];
	uint8_t local_auth = lcp->local_auth;
	switch(ppp_event)
	{
		case PPP_LINKUP:
			if(local_auth == AUTH_PAP)
			{
				pkt = _pap_payload(pap);
				protocol->state = PROTOCOL_STARTING;
				send_pap_request(protocol->pppdev, ++pap->id, pkt);
				send_ppp_event_xtimer(timer_msg, xtimer, ppp_msg_set(ID_PAP,PPP_TIMEOUT), 5000000);
			}
			else
			{
				protocol->state = PROTOCOL_UP;
				send_ppp_event(msg, ppp_msg_set(BROADCAST_NCP, PPP_LINKUP));
			}
			break;
		case PPP_LINKDOWN:
			protocol->state = PROTOCOL_DOWN;
			send_ppp_event(msg, ppp_msg_set(BROADCAST_NCP, PPP_LINKDOWN));
			break;
		case PPP_RECV:
			xtimer_remove(xtimer);
			if(ppp_hdr_get_code(hdr) != PPP_CONF_ACK) {
				DEBUG("gnrc_ppp: pap: Wrong APN auth. Closing link.\n");
				send_ppp_event(msg, ppp_msg_set(ID_LCP, PPP_LINKDOWN));
			}
			else {
				send_ppp_event(msg, ppp_msg_set(BROADCAST_NCP, PPP_LINKUP));
			}
			break;
		case PPP_TIMEOUT:
			pkt = _pap_payload(pap);
			send_pap_request(protocol->pppdev, ++pap->id, pkt);
			send_ppp_event_xtimer(timer_msg, xtimer, ppp_msg_set(ID_PAP,PPP_TIMEOUT), 5000000);
			break;
		default:
			DEBUG("gnrc_ppp: pap: Received unknown msg\n");


	}
	return 0;
}

int pap_init(struct gnrc_pppdev_t *ppp_dev, pap_t *pap)
{
	pap->user_size = 0;
	pap->pass_size = 0;
	ppp_protocol_init((ppp_protocol_t*) pap, ppp_dev, pap_handler, 241);
	pap->counter = 3;
	return 0;
}

