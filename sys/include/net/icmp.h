#ifndef ICMPV6_H_
#define ICMPV6_H_

#include "byteorder.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct __attribute__((packed))
{
	uint8_t type;
	uint8_t code;
	network_uint16_t csum;
	network_uint16_t id;
	network_uint16_t sn;
} icmp_hdr_t;


#ifdef __cplusplus
}
#endif

#endif
