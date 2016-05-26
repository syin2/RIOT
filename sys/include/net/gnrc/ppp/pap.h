#ifndef PPP_PAP_H
#define PPP_PAP_H

#include "net/gnrc/ppp/prot.h"
#include "xtimer.h"
#include "msg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pap_t
{
	ppp_protocol_t prot;
	char username[20];
	size_t user_size;
	char password[20];
	size_t pass_size;
	uint8_t counter;
	uint8_t id;
	xtimer_t xtimer;
	msg_t timer_msg;
} pap_t;

int pap_init(struct gnrc_pppdev_t *ppp_dev, ppp_protocol_t *pap);
ppp_protocol_t *pap_get_static_pointer(void);

#ifdef __cplusplus
}
#endif

#endif
