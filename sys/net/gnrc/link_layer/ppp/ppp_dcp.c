#include "net/gnrc/ppp/ppp.h"

#define ENABLE_DEBUG    (1)
#define ENABLE_MONITOR (1)
#include "debug.h"

#define MONITOR_TIMEOUT (5000000)

static dcp_t static_dcp;
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
			break;

		case PPP_UL_FINISHED:
			/*Remove timer*/
			xtimer_remove(xtimer);
			pppdev->driver->link_down(pppdev);
			break;

		case PPP_LINKUP:
			protocol->state = PROTOCOL_UP;
			send_ppp_event(msg, ppp_msg_set(PROT_LCP, PPP_LINKUP));
#if ENABLE_MONITOR
			/*Start monitor*/
			send_ppp_event_xtimer(timer_msg, xtimer, ppp_msg_set(PROT_DCP, PPP_MONITOR), MONITOR_TIMEOUT);
#endif
			break;

		case PPP_LINKDOWN:
			protocol->state = PROTOCOL_DOWN;
			send_ppp_event(msg, ppp_msg_set(PROT_LCP, PPP_LINKDOWN));
			break;

		case PPP_MONITOR:
			if(dcp->dead_counter)
			{
				DEBUG("gnrc_ppp: dcp: No response from modem. Send echo request.\n");
				send_ppp_event(msg, ppp_msg_set(PROT_LCP, PPP_MONITOR));
				send_ppp_event_xtimer(timer_msg, xtimer, ppp_msg_set(PROT_DCP, PPP_MONITOR), MONITOR_TIMEOUT);
				dcp->dead_counter -= 1;
			}
			else
			{
				send_ppp_event(msg, ppp_msg_set(PROT_DCP, PPP_UL_FINISHED));
				dcp->dead_counter = DCP_DEAD_COUNTER;
			}
			break;

		case PPP_LINK_ALIVE:
			DEBUG("gnrc_ppp: received echo reply. Link is working.\n");
			dcp->dead_counter = DCP_DEAD_COUNTER;
			break;

		case PPP_DIALUP:
			pppdev->driver->dial_up(pppdev);
			break;
		default:
			DEBUG("gnrc_ppp: dcp: Receive unknown message\n");
			break;
	}
	return 0;
}
int dcp_init(gnrc_pppdev_t *ppp_dev, ppp_protocol_t *dcp)
{
	ppp_protocol_init(dcp, ppp_dev, dcp_handler, PROT_DCP); 
	((dcp_t*) dcp)->dead_counter = DCP_DEAD_COUNTER;
	return 0;
}

ppp_protocol_t *dcp_get_static_pointer(void)
{
	return (ppp_protocol_t*) &static_dcp;
}
