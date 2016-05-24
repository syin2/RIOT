#include "net/gnrc/ppp/ppp.h"

#define ENABLE_DEBUG    (1)
#define ENABLE_MONITOR (1)
#include "debug.h"

int dcp_handler(struct ppp_protocol_t *protocol, uint8_t ppp_event, void *args)
{
	msg_t *msg = &protocol->msg;
	msg_t *timer_msg = &((dcp_t*) protocol)->timer_msg;
	xtimer_t *xtimer = &((dcp_t*) protocol)->xtimer;
	dcp_t *dcp = (dcp_t*) protocol;
	pppdev_t *pppdev = protocol->pppdev->netdev;

	switch(ppp_event)
	{
		case PPP_UL_STARTED:
			DEBUG("gnrc_ppp: dcp: PPP_UL_STARTED\n");
			break;

		case PPP_UL_FINISHED:
			DEBUG("gnrc_ppp:: dcp: PPP_UL_FINISHED\n");
			/*Remove timer*/
			xtimer_remove(xtimer);
			pppdev->driver->link_down(pppdev);
			break;

		case PPP_LINKUP:
			DEBUG("gnrc_ppp: dcp: PPP_LINKUP\n");
			send_ppp_event(msg, ppp_msg_set(ID_LCP, PPP_LINKUP));
#if ENABLE_MONITOR
			/*Start monitor*/
			protocol->state = PROTOCOL_UP;
			send_ppp_event_xtimer(timer_msg, xtimer, ppp_msg_set(ID_PPPDEV, PPP_MONITOR), 5000000);
#endif
			break;

		case PPP_LINKDOWN:
			DEBUG("gnrc_ppp: dcp: PPP_LINKDOWN\n");
			protocol->state = PROTOCOL_DOWN;
			send_ppp_event(msg, ppp_msg_set(ID_LCP, PPP_LINKDOWN));
			break;

		case PPP_MONITOR:
			if(dcp->dead_counter)
			{
				send_ppp_event(msg, ppp_msg_set(ID_LCP, PPP_MONITOR));
				send_ppp_event_xtimer(timer_msg, xtimer, ppp_msg_set(ID_PPPDEV, PPP_MONITOR), 5000000);
				dcp->dead_counter -= 1;
			}
			else
			{
				send_ppp_event(msg, ppp_msg_set(ID_PPPDEV, PPP_UL_FINISHED));
				dcp->dead_counter = DCP_DEAD_COUNTER;
			}
			break;

		case PPP_LINK_ALIVE:
			DEBUG("Received ECHO request! Link working! :)\n");
			dcp->dead_counter = DCP_DEAD_COUNTER;
			break;

		case PPP_DIALUP:
			DEBUG("Dialing device driver\n");
			pppdev->driver->dial_up(pppdev);
			DEBUG("Break\n");
			break;
		default:
			DEBUG("DCP: Receive unknown message\n");
			break;
	}
	return 0;
}
int dcp_init(gnrc_pppdev_t *ppp_dev, ppp_protocol_t *dcp)
{
	ppp_protocol_init(dcp, ppp_dev, dcp_handler, ID_PPPDEV); 
	((dcp_t*) dcp)->sent_id = 0;
	((dcp_t*) dcp)->dead_counter = DCP_DEAD_COUNTER;
	return 0;
}
