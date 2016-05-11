#ifndef PPP_PROTOCOL_H
#define PPP_PROTOCOL_H

#include "net/gnrc.h"
#include "xtimer.h"
#include <inttypes.h>
#include <byteorder.h>
#include "net/ppp/hdr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ppp_protocol_t
{
	int (*handler)(struct ppp_protocol_t *protocol, uint8_t ppp_event, void *args);
	uint8_t id;
} ppp_protocol_t;



#ifdef __cplusplus
}
#endif

#endif
