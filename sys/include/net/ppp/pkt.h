
#ifndef PPP_PKT_H_
#define PPP_PKT_H_

#include <inttypes.h>

#include "net/gnrc.h"
#include "net/gnrc/netdev2.h"
#include "byteorder.h"

#ifdef __cplusplus
extern "C" {
#endif
#define PPP_PAYLOAD_SIZE (200)
#define CPOPT_MAX_OPT (25)

#define OPT_HAS_ACK (1)
#define OPT_HAS_NAK (2)
#define OPT_HAS_REJ (4)

#define CP_CREQ_ACK (0)
#define CP_CREQ_NAK (1)
#define CP_CREQ_REJ (2)

#define PPP_CONF_REQ (1)
#define PPP_CONF_ACK (2)
#define PPP_CONF_NAK (3)
#define PPP_CONF_REJ (4)
#define PPP_CP_TERM_REQUEST (5)
#define PPP_CP_TERM_ACK (6)
#define PPP_CP_SER (7)

/*  PPP pkt header struct */
typedef struct __attribute__((packed)){
	uint8_t code;
	uint8_t id;
	network_uint16_t length;
} cp_hdr_t;


/* A PPP packet*/
typedef struct __attribute__((packed))
{
	cp_hdr_t *hdr;
	uint8_t *payload;
} cp_pkt_t;




int ppp_pkt_init(uint8_t *data, size_t length, cp_pkt_t *cp_pkt);

int _ppp_cr_populate_options(uint8_t *payload, size_t p_size);

uint8_t ppp_pkt_get_code(cp_pkt_t *cp_pkt);
void ppp_pkt_set_code(cp_pkt_t *cp_pkt, uint8_t code);
uint8_t ppp_pkt_get_id(cp_pkt_t *cp_pkt);
void ppp_pkt_set_id(cp_pkt_t *cp_pkt, uint8_t id);
uint16_t ppp_pkt_get_length(cp_pkt_t *cp_pkt);
void ppp_pkt_set_length(cp_pkt_t *cp_pkt, uint16_t length);
int ppp_pkt_is_configure(cp_pkt_t *pkt);
uint8_t * ppp_pkt_get_payload(cp_pkt_t *cp_pkt);
void ppp_pkt_set_payload(cp_pkt_t *cp_pkt, uint8_t *data, size_t size);




#ifdef __cplusplus
}
#endif

#endif /* PPP_PKT_H_ */
/** @} */
