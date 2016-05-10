#ifndef IPV4_HDR_H
#define IPV4_HDR_H

#include "byteorder.h"
#include "net/ipv4/addr.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct __attribute__((packed))
{
	network_uint16_t v_ih_ts;
	network_uint16_t tl;
	network_uint16_t id;
	network_uint16_t fl_fo;
	uint8_t ttl;
	uint8_t protocol;
	network_uint16_t csum;
	ipv4_addr_t src;
	ipv4_addr_t dst;
} ipv4_hdr_t;


static inline void ipv4_hdr_set_version(ipv4_hdr_t *hdr)
{
	hdr->v_ih_ts.u8[0] &= 0x0f;
	hdr->v_ih_ts.u8[0] |= 0x40;
}

static inline uint8_t ipv4_hdr_get_version(ipv4_hdr_t *hdr)
{
	return ((hdr->v_ih_ts.u8[0]) >> 4);
}

static inline void ipv4_hdr_set_ihl(ipv4_hdr_t *hdr, uint8_t ihl)
{
	hdr->v_ih_ts.u8[0] &= 0xf0;
	hdr->v_ih_ts.u8[0] |= 0x0f & ihl;
}

static inline uint8_t ipv4_hdr_get_ihl(ipv4_hdr_t *hdr)
{
	return (hdr->v_ih_ts.u8[0] & 0x0f);
}

static inline void ipv4_hdr_set_ts(ipv4_hdr_t *hdr, uint8_t ts)
{
	hdr->v_ih_ts.u8[1] &= 0;	
	hdr->v_ih_ts.u8[1] |= ts;	
}

static inline uint8_t ipv4_hdr_get_ts(ipv4_hdr_t *hdr)
{
	return (hdr->v_ih_ts.u8[1]);
}

static inline void ipv4_hdr_set_tl(ipv4_hdr_t *hdr, uint16_t tl)
{
	hdr->tl = byteorder_htons(tl);
}

static inline uint16_t ipv4_hdr_get_tl(ipv4_hdr_t *hdr)
{
	return byteorder_ntohs(hdr->tl);
}

static inline void ipv4_hdr_set_id(ipv4_hdr_t *hdr, uint16_t id)
{
	hdr->id = byteorder_htons(id);
}

static inline uint16_t ipv4_hdr_get_id(ipv4_hdr_t *hdr)
{
	return byteorder_ntohs(hdr->id);
}

static inline void ipv4_hdr_set_flags(ipv4_hdr_t *hdr, uint8_t flags)
{
	hdr->fl_fo.u8[0] &= 0x1f;
	hdr->fl_fo.u8[0] |= (0xe0 & (flags << 5));
}

static inline uint8_t ipv4_hdr_get_flags(ipv4_hdr_t *hdr)
{
	return (((hdr->fl_fo.u8[0]) >> 5) & 0x07);
}

static inline void ipv4_hdr_set_fo(ipv4_hdr_t *hdr, uint16_t fo)
{
	hdr->fl_fo.u8[0] &= 0xe0; 
	hdr->fl_fo.u8[0] |= (0x1f & (fo >> 8)); 
	hdr->fl_fo.u8[1] = 0xff & fo;
}

static inline uint16_t ipv4_hdr_get_fo(ipv4_hdr_t *hdr)
{
	return (((hdr->fl_fo.u8[0] & 0x1f) << 8) + hdr->fl_fo.u8[1]);
}

static inline void ipv4_hdr_set_ttl(ipv4_hdr_t *hdr, uint8_t ttl)
{
	hdr->ttl = ttl;
}

static inline uint8_t ipv4_hdr_get_ttl(ipv4_hdr_t *hdr)
{
	return hdr->ttl;
}

static inline void ipv4_hdr_set_protocol(ipv4_hdr_t *hdr, uint8_t protocol)
{
	hdr->protocol = protocol;
}

static inline uint8_t ipv4_hdr_get_protocol(ipv4_hdr_t *hdr)
{
	return hdr->protocol;
}

static inline void ipv4_hdr_set_csum(ipv4_hdr_t *hdr, uint16_t csum)
{
	hdr->csum = byteorder_htons(csum);
}

static inline uint16_t ipv4_hdr_get_csum(ipv4_hdr_t *hdr)
{
	return byteorder_ntohs(hdr->csum);
}

static inline void ipv4_hdr_set_src(ipv4_hdr_t *hdr, ipv4_addr_t src)
{
	hdr->src = src;
}

static inline ipv4_addr_t ipv4_hdr_get_src(ipv4_hdr_t *hdr)
{
	return hdr->src;
}

static inline void ipv4_hdr_set_dst(ipv4_hdr_t *hdr, ipv4_addr_t dst)
{
	hdr->dst = dst;
}

static inline ipv4_addr_t ipv4_hdr_get_dst(ipv4_hdr_t *hdr)
{
	return hdr->dst;
}


#ifdef __cplusplus
}
#endif

#endif
