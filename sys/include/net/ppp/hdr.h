/*
 * Copyright (C) 2016 José Ignacio Alamos <jialamos@uc.cl>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    net_ppphdr Point-to-Point Protocol Header
 * @ingroup     net_ppp
 * @brief       PPP header abstraction type and helper functions
 * @{
 *
 * @file
 * @brief   General definitions for PPP header and their helper functions
 *
 * @author  José Ignacio Alamos
 */

#ifndef PPP_HDR_H_
#define PPP_HDR_H_

#include <inttypes.h>

#include "byteorder.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief   Header of a PPP packet
 * @details A PPP packet is transmited as a payload of an HDLC packet. PPP packets only carry information about control protocol
 * of a PPP stack (Link Control Protocol, IP Control Protocol, etc). IP packets encapsulated in HDLC frame are not
 * considered PPP packet.
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
 *
<<<<<<< 274d77e6a5d6ef0e3500f996a7e168f41b0d6563
 * @see <a href="https://tools.ietf.org/html/rfc1661#section-5">
 *          RFC 1661, section 5
 *      </a>
 */
/*  PPP pkt header struct */
typedef struct __attribute__((packed)){
    uint8_t code;               /**< Code of PPP packet*/
    uint8_t id;                 /**< Identifier PPP of packet*/
    network_uint16_t length;    /**< Length of PPP packet including payload*/
} ppp_hdr_t;

=======
 */
/*  PPP pkt header struct */
typedef struct __attribute__((packed)){
    uint8_t code;
    uint8_t id;
    network_uint16_t length;
} ppp_hdr_t;



/**
 * @brief    Get code field of PPP packet
 *
 * @param[in] hdr    PPP header
 *
 * @return    code of PPP header
 */
static inline uint8_t ppp_hdr_get_code(ppp_hdr_t *hdr)
{
    return hdr->code;
}


/**
 * @brief    Set code field of PPP header
 *
 * @param[in] hdr    PPP header
 * @param[in] code    code to be set
 *
 */
static inline void ppp_hdr_set_code(ppp_hdr_t *hdr, uint8_t code)
{
    hdr->code = code;
}


/**
 * @brief    Get id field of PPP header
 *
 * @param[in] hdr    PPP header
 *
 * @return    id of PPP header
 */
static inline uint8_t ppp_hdr_get_id(ppp_hdr_t *hdr)
{
    return hdr->id;
}


/**
 * @brief    Set id field of PPP header
 *
 * @param[in] hdr    PPP header
 * @param[in] id    id to be set
 *
 */
static inline void ppp_hdr_set_id(ppp_hdr_t *hdr, uint8_t id)
{
    hdr->id = id;
}


/**
 * @brief    Get length field of PPP header
 *
 * @param[in] hdr    PPP header
 *
 * @return    length of PPP header
 */
static inline uint16_t ppp_hdr_get_length(ppp_hdr_t *hdr)
{
    return byteorder_ntohs(hdr->length);
}



/**
 * @brief    Set length field of PPP header
 *
 * @param[in] hdr    PPP header
 * @param[in] length    length to be set
 *
 */
static inline void ppp_hdr_set_length(ppp_hdr_t *hdr, uint16_t length)
{
    hdr->length = byteorder_htons(length);
}


>>>>>>> PPP: hdr: added PPP header definition and helpers
#ifdef __cplusplus
}
#endif

#endif /* PPP_HDR_H_ */
<<<<<<< 274d77e6a5d6ef0e3500f996a7e168f41b0d6563
/** @} */
=======
>>>>>>> PPP: hdr: added PPP header definition and helpers
