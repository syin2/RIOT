#include "net/gnrc/ppp/pap.h"
#include "net/gnrc/ppp/ppp.h"

#define ENABLE_DEBUG    (1)
#include "debug.h"


gnrc_pktsnip_t *_pap_payload(pap_t *pap)
{
	uint8_t data[2] = {0,0};
	gnrc_pktsnip_t *pkt = gnrc_pktbuf_add(NULL, (void*) data, 2, GNRC_NETTYPE_UNDEF);
	return pkt;
}

int pap_handler(struct ppp_protocol_t *protocol, uint8_t ppp_event, void *args)
{
	pap_t *pap = (pap_t*) protocol;
	msg_t *msg = &protocol->msg;
	gnrc_pktsnip_t *pkt;
	xtimer_t *xtimer = &pap->xtimer;
	msg_t *timer_msg = &pap->timer_msg;
	switch(ppp_event)
	{
		case PPP_LINKUP:
			pkt = _pap_payload(pap);
			send_pap_request(protocol->pppdev, ++pap->id, pkt);
			send_ppp_event_xtimer(timer_msg, xtimer, ppp_msg_set(ID_PAP,PPP_TIMEOUT), 5000000);
			break;
		case PPP_LINKDOWN:
			send_ppp_event(msg, ppp_msg_set(BROADCAST_NCP, PPP_LINKDOWN));
			break;
		case PPP_RECV:
			xtimer_remove(xtimer);
			send_ppp_event(msg, ppp_msg_set(BROADCAST_NCP, PPP_LINKUP));
			break;
		case PPP_TIMEOUT:
			pkt = _pap_payload(pap);
			send_pap_request(protocol->pppdev, ++pap->id, pkt);
			send_ppp_event_xtimer(timer_msg, xtimer, ppp_msg_set(ID_PAP,PPP_TIMEOUT), 5000000);
			break;
		default:
			DEBUG("PAP: Received unknown msg\n");


	}
	return 0;
}

int pap_init(struct gnrc_pppdev_t *ppp_dev, pap_t *pap)
{
	//pap->username = {0};
	//pap->password = {0};
	ppp_protocol_init((ppp_protocol_t*) pap, ppp_dev, pap_handler, 241);
	pap->counter = 3;
	return 0;
}

