
#ifndef PPP_PKT_H_
#define PPP_PKT_H_

#include <inttypes.h>

#include "net/gnrc.h"
#include "net/gnrc/netdev2.h"
#include "byteorder.h"

#ifdef __cplusplus
extern "C" {
#endif
#define PPP_PAYLOAD_SIZE (2000)

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


int ppp_pkt_populate(uint8_t *data, size_t length, cp_pkt_t *cp_pkt);

uint8_t ppp_pkt_get_code(cp_pkt_t *cp_pkt);
void ppp_pkt_set_code(cp_pkt_t *cp_pkt, uint8_t code);
uint8_t ppp_pkt_get_id(cp_pkt_t *cp_pkt);
void ppp_pkt_set_id(cp_pkt_t *cp_pkt, uint8_t id);
uint16_t ppp_pkt_get_length(cp_pkt_t *cp_pkt);
void ppp_pkt_set_length(cp_pkt_t *cp_pkt, uint16_t length);
uint8_t *ppp_pkt_get_payload(cp_pkt_t *cp_pkt);
void ppp_pkt_set_payload(cp_pkt_t *cp_pkt, uint8_t *payload);

#ifdef __cplusplus
}
#endif

#endif /* PPP_PKT_H_ */
/** @} */
