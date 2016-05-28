/*
 * Copyright (C) 2015 José Ignacio Alamos <jialamos@uc.cl>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    net_ipv4_hdr    IPv4 header
 * @ingroup     net_ipv4
 * @brief       IPv4 header types and helper functions
 * @{
 *
 * @file
 * @brief   IPv4 header type and helper function definitions
 *
 * @author  José Ignacio Alamos <jialamos@uc.cl>
 */
#ifndef IPV4_HDR_H
#define IPV4_HDR_H

#include "byteorder.h"
#include "net/ipv4/addr.h"
#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Data type to represent an IPv4 packet header.
 *
 * @details The structure of the header is as follows:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.unparsed}
 *
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |Version|  IHL  |Type of Service|          Total Length         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |         Identification        |Flags|      Fragment Offset    |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  Time to Live |    Protocol   |         Header Checksum       |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                       Source Address                          |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                    Destination Address                        |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                    Options                    |    Padding    |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * @see <a href="https://tools.ietf.org/html/rfc791#section-3.1">
 *              RFC 791, section 3.1
 *      </a>
 */
typedef struct __attribute__((packed))
{
    /**
     * @brief Version, Internet Header Length and Type of Service.
     *
     * @details The version are the 4 most significant bits, the Internet Header Length 
     * the 4 next bit, and the remainding 8 bits are the Type of Service (see
     * above).
     *
     * This module provides helper functions to set, get, and check these
     * fields accordingly:
     * * ipv4_hdr_set_version()
     * * ipv4_hdr_get_version()
     * * ipv4_hdr_set_ihl()
     * * ipv4_hdr_get_ihl()
     * * ipv4_hdr_set_ts()
     * * ipv4_hdr_get_ts()
     */
	network_uint16_t v_ih_ts;
	network_uint16_t tl; /**< total length of the datagram */
	network_uint16_t id; /**< Identification value of packet */
	/**
	 * @brief version control flags and Fragment Offset.
	 *
	 * @details The flags are the 3 most significant bits, and the remaining 13 bits are the fragment offfset
	 *
     * This module provides helper functions to set, get, and check these
     * fields accordingly:
     * * ipv4_hdr_set_flags()
     * * ipv4_hdr_get_flags()
     * * ipv4_hdr_set_fo()
     * * ipv4_hdr_get_fo()
	 */
	network_uint16_t fl_fo; 
	uint8_t ttl; /**< time to live for this packet */
	uint8_t protocol; /**< protocol of this packet */
	network_uint16_t csum; /**< checksum of this packet */
	ipv4_addr_t src; /**< source address of this packet */
	ipv4_addr_t dst; /**< destination address of this packet */
} ipv4_hdr_t;


/**
 * @brief   Sets the version field of @p hdr to 6
 *
 * @param[out] hdr  Pointer to an IPv4 header.
 */
static inline void ipv4_hdr_set_version(ipv4_hdr_t *hdr)
{
	hdr->v_ih_ts.u8[0] &= 0x0f;
	hdr->v_ih_ts.u8[0] |= 0x40;
}

/**
 * @brief   Gets the value of the version field of @p hdr
 *
 * @param[in] hdr   Pointer to an IPv4 header.
 *
 * @return  Value of the version field of @p hdr.
 */
static inline uint8_t ipv4_hdr_get_version(ipv4_hdr_t *hdr)
{
	return ((hdr->v_ih_ts.u8[0]) >> 4);
}

/**
 * @brief   Sets the Internet Header Length field of @p hdr
 *
 * @param[out] hdr  Pointer to an IPv4 header.
 * @param[in] ihl  The new value of Internet Header Length.
 */
static inline void ipv4_hdr_set_ihl(ipv4_hdr_t *hdr, uint8_t ihl)
{
	hdr->v_ih_ts.u8[0] &= 0xf0;
	hdr->v_ih_ts.u8[0] |= 0x0f & ihl;
}

/**
 * brief Gets the value of the Internet Header Length field of @p hdr
 *
 * @param[in] hdr   Pointer to an IPv4 header.
 *
 * @return Value of the Internet Header Length field of @p hdr
 */
static inline uint8_t ipv4_hdr_get_ihl(ipv4_hdr_t *hdr)
{
	return (hdr->v_ih_ts.u8[0] & 0x0f);
}

/**
 * @brief   Sets the Type of Service field of @p hdr
 *
 * @param[out] hdr  Pointer to an IPv4 header.
 * @param[in] ts  The new value of Type of Service
 */
static inline void ipv4_hdr_set_ts(ipv4_hdr_t *hdr, uint8_t ts)
{
	hdr->v_ih_ts.u8[1] &= 0;	
	hdr->v_ih_ts.u8[1] |= ts;	
}

/**
 * brief Gets the value of the Type of Service field of @p hdr
 *
 * @param[in] hdr   Pointer to an IPv4 header.
 *
 * @return Value of the Type of Service field of @p hdr
 */
static inline uint8_t ipv4_hdr_get_ts(ipv4_hdr_t *hdr)
{
	return (hdr->v_ih_ts.u8[1]);
}

/**
 * @brief   Sets the Total Length field of @p hdr
 *
 * @param[out] hdr  Pointer to an IPv4 header.
 * @param[in] tl  The new value of Total Length
 */
static inline void ipv4_hdr_set_tl(ipv4_hdr_t *hdr, uint16_t tl)
{
	hdr->tl = byteorder_htons(tl);
}

/**
 * brief Gets the value of the Total Length field of @p hdr
 *
 * @param[in] hdr   Pointer to an IPv4 header.
 *
 * @return Value of the Total Length field of @p hdr
 */
static inline uint16_t ipv4_hdr_get_tl(ipv4_hdr_t *hdr)
{
	return byteorder_ntohs(hdr->tl);
}

/**
 * @brief   Sets the ID field of @p hdr
 *
 * @param[out] hdr  Pointer to an IPv4 header.
 * @param[in] id  The new value of id
 */
static inline void ipv4_hdr_set_id(ipv4_hdr_t *hdr, uint16_t id)
{
	hdr->id = byteorder_htons(id);
}

/**
 * brief Gets the value of the ID field of @p hdr
 *
 * @param[in] hdr   Pointer to an IPv4 header.
 *
 * @return Value of the ID field of @p hdr
 */
static inline uint16_t ipv4_hdr_get_id(ipv4_hdr_t *hdr)
{
	return byteorder_ntohs(hdr->id);
}

/**
 * @brief   Sets the Version Control Flags field of @p hdr
 *
 * @param[out] hdr  Pointer to an IPv4 header.
 * @param[in] flags  The new value of flags
 */
static inline void ipv4_hdr_set_flags(ipv4_hdr_t *hdr, uint8_t flags)
{
	hdr->fl_fo.u8[0] &= 0x1f;
	hdr->fl_fo.u8[0] |= (0xe0 & (flags << 5));
}

/**
 * brief Gets the value of the Version Control Flags field of @p hdr
 *
 * @param[in] hdr   Pointer to an IPv4 header.
 *
 * @return Value of the Version Control field of @p hdr
 */
static inline uint8_t ipv4_hdr_get_flags(ipv4_hdr_t *hdr)
{
	return (((hdr->fl_fo.u8[0]) >> 5) & 0x07);
}

/**
 * @brief   Sets the Fragment Offset field of @p hdr
 *
 * @param[out] hdr  Pointer to an IPv4 header.
 * @param[in] fo  The new value of fragment offset
 */
static inline void ipv4_hdr_set_fo(ipv4_hdr_t *hdr, uint16_t fo)
{
	hdr->fl_fo.u8[0] &= 0xe0; 
	hdr->fl_fo.u8[0] |= (0x1f & (fo >> 8)); 
	hdr->fl_fo.u8[1] = 0xff & fo;
}

/**
 * brief Gets the value of the Fragment Offset field of @p hdr
 *
 * @param[in] hdr   Pointer to an IPv4 header.
 *
 * @return Value of the Fragment Offset field of @p hdr
 */
static inline uint16_t ipv4_hdr_get_fo(ipv4_hdr_t *hdr)
{
	return (((hdr->fl_fo.u8[0] & 0x1f) << 8) + hdr->fl_fo.u8[1]);
}

/**
 * @brief   Sets the Time to Live field of @p hdr
 *
 * @param[out] hdr  Pointer to an IPv4 header.
 * @param[in] ttl  The new value of Time to Live
 */
static inline void ipv4_hdr_set_ttl(ipv4_hdr_t *hdr, uint8_t ttl)
{
	hdr->ttl = ttl;
}

/**
 * brief Gets the value of the Time To Live field of @p hdr
 *
 * @param[in] hdr   Pointer to an IPv4 header.
 *
 * @return Value of the Time To Live field of @p hdr
 */
static inline uint8_t ipv4_hdr_get_ttl(ipv4_hdr_t *hdr)
{
	return hdr->ttl;
}

/**
 * @brief   Sets the protocol field of @p hdr
 *
 * @param[out] hdr  Pointer to an IPv4 header.
 * @param[in] protocol  The new value of protocol
 */
static inline void ipv4_hdr_set_protocol(ipv4_hdr_t *hdr, uint8_t protocol)
{
	hdr->protocol = protocol;
}

/**
 * brief Gets the value of the Protocol field of @p hdr
 *
 * @param[in] hdr   Pointer to an IPv4 header.
 *
 * @return Value of the Protocol field of @p hdr
 */
static inline uint8_t ipv4_hdr_get_protocol(ipv4_hdr_t *hdr)
{
	return hdr->protocol;
}

/**
 * @brief   Sets the checksum field of @p hdr
 *
 * @param[out] hdr  Pointer to an IPv4 header.
 * @param[in] csum  The new value of checksum
 */
static inline void ipv4_hdr_set_csum(ipv4_hdr_t *hdr, uint16_t csum)
{
	hdr->csum = byteorder_htons(csum);
}

/**
 * brief Gets the value of the Checksum field of @p hdr
 *
 * @param[in] hdr   Pointer to an IPv4 header.
 *
 * @return Value of the Checksum field of @p hdr
 */
static inline uint16_t ipv4_hdr_get_csum(ipv4_hdr_t *hdr)
{
	return byteorder_ntohs(hdr->csum);
}

/**
 * @brief   Sets the source address field of @p hdr
 *
 * @param[out] hdr  Pointer to an IPv4 header.
 * @param[in] src  The new value of source address
 */
static inline void ipv4_hdr_set_src(ipv4_hdr_t *hdr, ipv4_addr_t src)
{
	hdr->src = src;
}

/**
 * brief Gets the value of the source address field of @p hdr
 *
 * @param[in] hdr   Pointer to an IPv4 header.
 *
 * @return Value of the source address field of @p hdr
 */
static inline ipv4_addr_t ipv4_hdr_get_src(ipv4_hdr_t *hdr)
{
	return hdr->src;
}

/**
 * @brief   Sets the destination address field of @p hdr
 *
 * @param[out] hdr  Pointer to an IPv4 header.
 * @param[in] dst  The new value of destination address
 */
static inline void ipv4_hdr_set_dst(ipv4_hdr_t *hdr, ipv4_addr_t dst)
{
	hdr->dst = dst;
}

/**
 * brief Gets the value of the destination address field of @p hdr
 *
 * @param[in] hdr   Pointer to an IPv4 header.
 *
 * @return Value of the destination address field of @p hdr
 */
static inline ipv4_addr_t ipv4_hdr_get_dst(ipv4_hdr_t *hdr)
{
	return hdr->dst;
}


#ifdef __cplusplus
}
#endif

#endif /* IPV4_HDR_H */
/** @} */
