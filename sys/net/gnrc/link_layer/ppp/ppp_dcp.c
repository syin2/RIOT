#include "net/gnrc/ppp/ppp.h"

int dcp_handler(struct ppp_protocol_t *protocol, uint8_t ppp_event, void *args)
{
	msg_t *msg = &((dcp_t*) protocol)->msg;
	msg_t *timer_msg = &((dcp_t*) protocol)->timer_msg;
	xtimer_t *xtimer = &((dcp_t*) protocol)->xtimer;
	dcp_t *dcp = (dcp_t*) protocol;
	pppdev_t *pppdev = dcp->pppdev->netdev;
	DEBUG("In DCP handler\n");
	switch(ppp_event)
	{
		case PPP_UL_STARTED:
			DEBUG("Driver: PPP_UL_STARTED\n");
			break;
		case PPP_UL_FINISHED:
			DEBUG("Driver: PPP_UL_FINISHED\n");
			/*Remove timer*/
			xtimer_remove(xtimer);

			pppdev->driver->link_down(pppdev);
			break;
		case PPP_LINKUP:
			DEBUG("Driver: PPP_LINKUP\n");
			msg->type = GNRC_PPPDEV_MSG_TYPE_EVENT;
			msg->content.value = (ID_LCP << 8) | (PPP_LINKUP & 0xFFFF);
			msg_send(msg, thread_getpid());

			/*Start monitor*/
			timer_msg->type = GNRC_PPPDEV_MSG_TYPE_EVENT;
			timer_msg->content.value = (ID_PPPDEV<<8) | PPP_MONITOR;		
			xtimer_set_msg(xtimer, 5000000, timer_msg, thread_getpid());
			break;
		case PPP_LINKDOWN:
			DEBUG("Driver: PPP_LINKDOWN\n");
			msg->type = GNRC_PPPDEV_MSG_TYPE_EVENT;
			msg->content.value = (ID_LCP << 8) | (PPP_LINKDOWN & 0xFFFF);
			msg_send(msg, thread_getpid());
			break;
		case PPP_MONITOR:
			if(dcp->dead_counter)
			{
				msg->type = GNRC_PPPDEV_MSG_TYPE_EVENT;
				msg->content.value = (ID_LCP<<8) | PPP_MONITOR;		
				msg_send(msg, thread_getpid());

				timer_msg->type = GNRC_PPPDEV_MSG_TYPE_EVENT;
				timer_msg->content.value = (ID_PPPDEV<<8) | PPP_MONITOR;		
				xtimer_set_msg(xtimer, 5000000, timer_msg, thread_getpid());
				dcp->dead_counter -= 1;
			}
			else
			{
				msg->type = GNRC_PPPDEV_MSG_TYPE_EVENT;
				msg->content.value = (ID_PPPDEV<<8) | PPP_UL_FINISHED;		
				msg_send(msg, thread_getpid());
				dcp->dead_counter = DCP_DEAD_COUNTER;
			}
			break;
		case PPP_LINK_ALIVE:
			DEBUG("Received ECHO request! Link working! :)\n");
			dcp->dead_counter = DCP_DEAD_COUNTER;
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
	((dcp_t*) dcp)->dead_counter = DCP_DEAD_COUNTER;
	return 0;
}
