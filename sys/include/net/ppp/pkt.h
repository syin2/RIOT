
#ifndef PPP_PKT_H_
#define PPP_PKT_H_

#include <inttypes.h>

#include "net/gnrc.h"
#include "net/gnrc/netdev2.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Control protocol header struct */
typedef struct __attribute__((packed)){
	uint8_t code;
	uint8_t id;
	uint16_t length;
} cp_hdr_t;

/* A Control Protocol packet*/
typedef struct cp_pkt_t
{
	cp_hdr_t hdr;
	opt_stack_t opts;
} cp_pkt_t;

/*Control Protocol configure option*/
typedef struct cp_opt_t{
	uint8_t type;
	uint8_t status;
	uint8_t payload[OPT_PAYLOAD_SIZE];
	size_t p_size;
	cp_opt_t *next;
} cp_opt_t;


static int ppp_pkt_populate_from_pktsnip(gnrc_pktsnipt_t pkt, cp_pkt_t *cp_pkt);
int ppp_cp_populate_options(opt_stack *o_stack, uint8_t *payload, size_t p_size);
int ppp_cp_opts_are_equal(cp_opt_t *o1, cp_opt_t *o2);
int ppp_cp_optstacks_are_equal(opt_stack_t *s1, opt_stack_t *s2);

#ifdef __cplusplus
}
#endif

#endif /* PPP_PKT_H_ */
/** @} */
