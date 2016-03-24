/*
 * Copyright (C) 2016 José Ignacio Alamos <jialamos@uc.cl>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    net_ppp Packet
 * @ingroup     net_ppp
 * @brief       PPP packet abstraction type and helper functions
 * @{
 *
 * @file
 * @brief   General definitions for PPP packets and their helper functions
 *
 * @author  José Ignacio Alamos
 */

#ifndef PPP_PKT_H_
#define PPP_PKT_H_

#include <inttypes.h>

#include "net/gnrc.h"
#include "net/gnrc/netdev2.h"
#include "byteorder.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief   Header of a PPP packet
 * @details A PPP packet is transmited as a payload of an HDLC packet. PPP packets only carry information about control protocols
 * of a PPP stack (Link Control Protocol, IP Control Protocol, etc). IP packets encapsulated in HDLC frame are not
 * considered PPP packet for RIOT context.
 *
 * The format of PPP header plus payload is:
 *
 *
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |     Code      |  Identifier   |            Length             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |    Payload ...
 * +-+-+-+-+ 
 *
 * These fields are encoded in a convenient way in this struct. A PPP packet doesn't have its own buffer, it must be assigned with ppp_pkt_init function.
 * MEMBERS OF PACKET STRUCTURE SHOULDN'T BE MODIFIED WITHOUT FUNCTION HELPERS! 
 *
 */
/*  PPP pkt header struct */
typedef struct __attribute__((packed)){
	uint8_t code;
	uint8_t id;
	network_uint16_t length;
} cp_hdr_t;


typedef struct ppp_pkt_buffer_t
{
	uint8_t *_payload;
	size_t _size;
} ppp_pkt_buffer_t;

/**
 * @brief   Type to represent a PPP packet
 * @details PPP packets are not subdivided in gnrc pktsnips due to the number of possA PPP packet doesn't have its own buffer, so it's always pointing to a buffer with known size. 
 * These fields are encoded in a convenient way in this struct. A PPP packet doesn't have its own buffer, it must be assigned with ppp_pkt_init function.
 * MEMBERS OF PACKET STRUCTURE SHOULDN'T BE MODIFIED WITHOUT FUNCTION HELPERS! 
 *
 */
/* A PPP packet*/
typedef struct ppp_pkt_t
{
	cp_hdr_t *hdr;
	ppp_pkt_buffer_t _buf;
} ppp_pkt_t;


/**
 * @brief Init a PPP packet
 *
 * @param[in] buffer   Buffer where pkt data is stored
 * @param[in] size   Size available in buffer
 * @param[in] pkt   Pointer to PPP packet
 *
 * @return 0, if packet was successfully initialized
 * @return -ENOMEM, if buffer size was not enough for allocating a pkt
 */

int ppp_pkt_init(uint8_t *buffer, size_t size, ppp_pkt_t *pkt);

/**
 * @brief	Get code field of PPP packet
 * 
 * @param[in] pkt	PPP packet
 *
 * @return	code of PPP packet
 */
uint8_t ppp_pkt_get_code(ppp_pkt_t *pkt);


/**
 * @brief	Set code field of PPP packet
 * 
 * @param[in] pkt	PPP packet
 * @param[in] code	code to be set
 *
 */
void ppp_pkt_set_code(ppp_pkt_t *pkt, uint8_t code);


/**
 * @brief	Get id field of PPP packet
 * 
 * @param[in] pkt	PPP packet
 *
 * @return	id of PPP packet
 */
uint8_t ppp_pkt_get_id(ppp_pkt_t *pkt);


/**
 * @brief	Set id field of PPP packet
 * 
 * @param[in] pkt	PPP packet
 * @param[in] id	id to be set
 *
 */
void ppp_pkt_set_id(ppp_pkt_t *pkt, uint8_t id);


/**
 * @brief	Get length field of PPP packet
 * 
 * @param[in] pkt	PPP packet
 *
 * @return	length of PPP packet
 */
uint16_t ppp_pkt_get_length(ppp_pkt_t *pkt);


/**
 * @brief	Set length field of PPP packet
 * 
 * @param[in] pkt	PPP packet
 * @param[in] length	length to be set
 *
 */
void ppp_pkt_set_length(ppp_pkt_t *pkt, uint16_t length);


/**
 * @brief	Get payload pointer of PPP packet
 * 
 * @param[in] pkt	PPP packet
 *
 * @return	Pointer to payload of PPP packet
 */
uint8_t * ppp_pkt_get_payload(ppp_pkt_t *pkt);


/**
 * @brief	Set payload of a PPP packet. As the PPP packet knows the buffer size, it will fail if there's no more space available.
 * 
 * @param[in] pkt	PPP packet
 * @param[in] payload	payload to be set
 * @param[in] size	size of the payload to be set
 *
 * @return	0, if payload was set
 * @return -ENOMEM, if error
 */
int ppp_pkt_set_payload(ppp_pkt_t *pkt, uint8_t *data, size_t size);





#ifdef __cplusplus
}
#endif

#endif /* PPP_PKT_H_ */
/** @} */
