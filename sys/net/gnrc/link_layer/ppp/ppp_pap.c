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
	gnrc_pktsnip_t *pkt, *sent_pkt;
	xtimer_t *xtimer = &pap->xtimer;
	msg_t *timer_msg = &pap->timer_msg;
	switch(ppp_event)
	{
		case PPP_LINKUP:
			pkt = _pap_payload(pap);
			send_pap_request(protocol->pppdev, ++pap->id, pkt);
			timer_msg->type = GNRC_PPPDEV_MSG_TYPE_EVENT;
			timer_msg->content.value = (ID_PAP<<8) | PPP_TIMEOUT;		
			xtimer_set_msg(xtimer, 5000000, timer_msg, thread_getpid());
			break;
		case PPP_LINKDOWN:
			msg->type = GNRC_PPPDEV_MSG_TYPE_EVENT;
			msg->content.value = (BROADCAST_NCP << 8) | (PPP_LINKDOWN & 0xFFFF);
			msg_send(msg, thread_getpid());
			break;
		case PPP_RECV:
			xtimer_remove(xtimer);
			msg->type = GNRC_PPPDEV_MSG_TYPE_EVENT;
			msg->content.value = (BROADCAST_NCP << 8) | (PPP_LINKUP & 0xFFFF);
			msg_send(msg, thread_getpid());
			break;
		case PPP_TIMEOUT:
			pkt = _pap_payload(pap);
			sent_pkt = pkt_build(GNRC_NETTYPE_PAP, 1, ++pap->id, pkt);
			gnrc_ppp_send(protocol->pppdev, sent_pkt);
			timer_msg->type = GNRC_PPPDEV_MSG_TYPE_EVENT;
			timer_msg->content.value = (ID_PAP<<8) | PPP_TIMEOUT;		
			xtimer_set_msg(xtimer, 5000000, timer_msg, thread_getpid());
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

