
#ifndef PPP_PKT_H_
#define PPP_PKT_H_

#include <inttypes.h>

#include "net/gnrc.h"
#include "net/gnrc/netdev2.h"

#ifdef __cplusplus
extern "C" {
#endif


/* A Control Protocol packet*/
typedef struct c__attribute__((packed))
{
	uint8_t code;
	uint8_t id;
	uint16_t length;
	void *payload; /* options, data */
} cp_pkt_t;


static int ppp_pkt_populate(uint8_t *data, size_t length, cp_pkt_t *cp_pkt);

#ifdef __cplusplus
}
#endif

#endif /* PPP_PKT_H_ */
/** @} */
