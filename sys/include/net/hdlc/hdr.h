
#ifndef HDLC_HDR_H
#define HDLC_HDR_H

#include <stdint.h>
#include "byteorder.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __attribute__((packed)) {
	uint8_t address; //Address
	uint16_t protocol; //protocol
	network_uint16_t fcs; //Frame Check Sequence
} hdlc_hdr_t;

static inline void hdlc_hdr_set_address(hdlc_hdr_t *hdr, uint8_t address)
{
	hdr->address = address;
}

static inline uint8_t hdlc_hdr_get_address(hdlc_hdr_t *hdr)
{
	return hdr->address;
}

static inline void hdlc_hdr_set_protocol(hdlc_hdr_t *hdr, uint16_t protocol)
{
	hdr->protocol = protocol;
}

static inline uint16_t hdlc_hdr_get_protocol(hdlc_hdr_t *hdr)
{
	return hdr->protocol;
}

static inline void hdlc_hdr_set_fcs(hdlc_hdr_t *hdr, uint16_t fcs)
{
	hdr->fcs.u16 = fcs;
} 

static inline network_uint16_t hdlc_hdr_get_fcs(hdlc_hdr_t *hdr)
{
	return hdr->fcs;
}


uint16_t hdlc_fcs16(uint16_t, uint8_t*, int);
uint32_t hdlc_fcs32(uint32_t, uint8_t*, int);

#ifdef __cplusplus
}
#endif

#endif
