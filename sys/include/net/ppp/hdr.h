#ifndef PPP_HDR_H
#define PPP_HDR_H

#include "byteorder.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __attribute__((packed)) {
	uint8_t flag; // Flag
	uint8_t address; //Address
	uint16_t protocol; //protocol
	network_uint32_t fcs; //Frame Check Sequence
} ppp_hdr_t;

static inline void ppp_hdr_set_flag(ppp_hdr_t *hdr)
{
	hdr->flag = 0x7E;
}

static inline uint8_t ppp_hdr_get_flag(ppp_hdr_t *hdr)
{
	return hdr->flag;
}

static inline void ppp_hdr_set_address(ppp_hdr_t *hdr)
{
	hdr->address = 0xFF;
}

static inline uint8_t ppp_hdr_get_address(ppp_hdr_t *hdr)
{
	return hdr->address;
}

static inline void ppp_hdr_set_protocol(ppp_hdr_t *hdr, uint16_t protocol)
{
	hdr->protocol = protocol;
}

static inline uint16_t ppp_hdr_get_protocol(ppp_hdr_t *hdr)
{
	return hdr->protocol;
}

static inline void ppp_hdr_set_fcs(ppp_hdr_t *hdr, uint16_t fcs_1, uint16_t fcs_2)
{
	hdr->fcs.u16[0] = fcs_1;
	hdr->fcs.u16[1] = fcs_2;
} 

static inline network_uint32_t ppp_hdr_get_fcs(ppp_hdr_t *hdr)
{
	return hdr->fcs;
}

void ppp_hdr_print(ppp_hdr_t *hdr);

#ifdef __cplusplus
}
#endif
#endif