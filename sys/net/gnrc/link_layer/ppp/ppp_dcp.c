#include "net/gnrc/ppp/ppp.h"

int dcp_handler(struct ppp_protocol_t *protocol, uint8_t ppp_event, void *args)
{
	msg_t *msg = &((dcp_t*) protocol)->msg;
	msg_t *timer_msg = &((dcp_t*) protocol)->timer_msg;
	xtimer_t *xtimer = &((dcp_t*) protocol)->xtimer;
	DEBUG("In DCP handler\n");
	switch(ppp_event)
	{
		case PPP_UL_STARTED:
			DEBUG("Driver: PPP_UL_STARTED\n");
			break;
		case PPP_UL_FINISHED:
			DEBUG("Driver: PPP_UL_FINISHED\n");
			msg->type = PPPDEV_MSG_TYPE_EVENT;
			msg->content.value = PPPDEV_LINK_DOWN_EVENT;
			msg_send(msg, thread_getpid());
			break;
		case PPP_LINKUP:
			DEBUG("Driver: PPP_LINKUP\n");
			msg->type = GNRC_PPPDEV_MSG_TYPE_EVENT;
			msg->content.value = (ID_LCP << 8) | (PPP_LINKUP & 0xFFFF);
			msg_send(msg, thread_getpid());

			/*Start monitor*/
			timer_msg->type = GNRC_PPPDEV_MSG_TYPE_EVENT;
			timer_msg->content.value = (ID_LCP<<8) | PPP_MONITOR;		
			xtimer_set_msg(xtimer, 5000000, timer_msg, thread_getpid());
			break;
		case PPP_LINKDOWN:
			DEBUG("Driver: PPP_LINKDOWN\n");
			msg->type = GNRC_PPPDEV_MSG_TYPE_EVENT;
			msg->content.value = (ID_LCP << 8) | (PPP_LINKDOWN & 0xFFFF);
			msg_send(msg, thread_getpid());
			break;
		case PPP_MONITOR:
			DEBUG("Received ECHO request! Link working! :)\n");
			timer_msg->type = GNRC_PPPDEV_MSG_TYPE_EVENT;
			timer_msg->content.value = (ID_LCP<<8) | PPP_MONITOR;		
			xtimer_set_msg(xtimer, 5000000, timer_msg, thread_getpid());
			break;
		default:
			break;
	}
	return 0;
}
int dcp_init(gnrc_pppdev_t *ppp_dev, ppp_protocol_t *dcp)
{
	dcp->handler = &dcp_handler;
	dcp->id = ID_PPPDEV;
	((dcp_t*) dcp)->sent_id = 0;
	((dcp_t*) dcp)->pppdev = ppp_dev;
	return 0;
}
