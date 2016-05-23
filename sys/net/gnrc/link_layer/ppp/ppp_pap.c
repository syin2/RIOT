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
	gnrc_pktsnip_t *recv_pkt = (gnrc_pktsnip_t*) args;
	ppp_hdr_t *hdr;
	switch(ppp_event)
	{
		case PPP_LINKUP:
			DEBUG("Starting PAP\n");
			pkt = _pap_payload(pap);
			sent_pkt = pkt_build(GNRC_NETTYPE_PAP, 1, ++pap->id, pkt);
			gnrc_ppp_send(protocol->pppdev, sent_pkt);
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
			hdr = (ppp_hdr_t*) recv_pkt->data;
			DEBUG("Received: %i\n", ppp_hdr_get_code(hdr));
			DEBUG("HEX: ");
			for(int i=0;i<4;i++)
			{
				DEBUG("%02x ", *(((uint8_t*) recv_pkt->next->data)+i));
			}
			for(int j=0;j<recv_pkt->size;j++)
			{
				DEBUG("%02x ", *(((uint8_t*) recv_pkt->data)+j));
			}
			DEBUG("\n");
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
	pap->counter = 3;
	pap->id = 0;
	pap->prot.handler = &pap_handler;
	((ppp_protocol_t*) pap)->pppdev = ppp_dev;
	return 0;
}

