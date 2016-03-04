
#ifndef HDLC_HDR_H
#define HDLC_HDR_H

#include <stdint.h>
#include "byteorder.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __attribute__((packed)) {
	uint8_t address; //Address
	uint8_t control; //Control
	uint16_t protocol; //Protocol
} hdlc_hdr_t;

static inline void hdlc_hdr_set_address(hdlc_hdr_t *hdr, uint8_t address)
{
	hdr->address = address;
}

static inline uint8_t hdlc_hdr_get_address(hdlc_hdr_t *hdr)
{
	return hdr->address;
}

static inline void hdlc_hdr_set_control(hdlc_hdr_t *hdr, uint8_t control)
{
	hdr->control = control;
}

static inline uint8_t hdlc_hdr_get_control(hdlc_hdr_t *hdr)
{
	return hdr->control;
}

static inline void hdlc_hdr_set_protocol(hdlc_hdr_t *hdr, uint16_t protocol)
{
	hdr->protocol = protocol;
}

static inline uint16_t hdlc_hdr_get_protocol(hdlc_hdr_t *hdr)
{
	return hdr->protocol;
}

uint16_t hdlc_fcs16(uint16_t, uint8_t*, int);
uint32_t hdlc_fcs32(uint32_t, uint8_t*, int);

#ifdef __cplusplus
}
#endif

#endif
