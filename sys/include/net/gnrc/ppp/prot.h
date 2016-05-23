#ifndef PPP_PROTOCOL_H
#define PPP_PROTOCOL_H

#include "net/gnrc.h"
#include "xtimer.h"
#include <inttypes.h>
#include <byteorder.h>
#include "net/ppp/hdr.h"
#include "msg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ppp_protocol_t
{
	int (*handler)(struct ppp_protocol_t *protocol, uint8_t ppp_event, void *args);
	uint8_t id;
	msg_t msg;
	struct gnrc_pppdev_t *pppdev;
} ppp_protocol_t;

typedef uint16_t ppp_msg_t;
typedef uint8_t ppp_target_t;
typedef uint8_t ppp_event_t;

static inline ppp_msg_t ppp_msg_set(ppp_target_t target, ppp_event_t ppp_event)
{
	return (target<<8) | ppp_event;
}

static inline ppp_target_t ppp_msg_get_target(ppp_msg_t ppp_msg)
{
	return (ppp_msg>>8);
}

static inline ppp_event_t ppp_msg_get_event(ppp_msg_t ppp_msg)
{
	return (ppp_msg & 0xffff);
}

void send_ppp_event(msg_t *msg, ppp_msg_t ppp_msg);
void send_ppp_event_xtimer(msg_t *msg, xtimer_t *xtimer, ppp_msg_t ppp_msg, int timeout);


#ifdef __cplusplus
}
#endif

#endif
