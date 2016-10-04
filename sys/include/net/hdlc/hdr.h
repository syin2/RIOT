/*
 * Copyright (C) 2015 José Ignacio Alamos <jialaos@uc.cl>
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for
 * more details.
 */

/**
 * @defgroup    net_hdlc_hdr    HDLC header
 * @ingroup     net_hdlc
 * @brief       HDLC header architecture
 *
 * @{
 *
 * @file
 * @brief       Definitions for HDLC header
 *
 * @author      José Ignacio Alamos <jialamos@uc.cl>
 */
#ifndef HDLC_HDR_H
#define HDLC_HDR_H

#include <stdint.h>
#include "byteorder.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Data type to represent an HDLC header.
 */
typedef struct __attribute__((packed)) {
    uint8_t address;            /**< Address field oh HDLC header */
    uint8_t control;            /**< Control field of HDLC header */
    network_uint16_t protocol;  /**< Protocol field of HDLC header */
} hdlc_hdr_t;

/**
 * @brief Sets the address field of HDLC header
 *
 * @param hdr pointer to HDLC header
 * @param address address of HDLC header
 */
static inline void hdlc_hdr_set_address(hdlc_hdr_t *hdr, uint8_t address)
{
    hdr->address = address;
}

/**
 * @brief Gets the address field of HDLC header
 *
 * @param hdr pointer to HDLC header
 *
 * @return address of HDLC header
 */
static inline uint8_t hdlc_hdr_get_address(hdlc_hdr_t *hdr)
{
    return hdr->address;
}

/**
 * @brief Sets the control field of HDLC header
 *
 * @param hdr pointer to HDLC header
 * @param control control field of hdlc header
 */
static inline void hdlc_hdr_set_control(hdlc_hdr_t *hdr, uint8_t control)
{
    hdr->control = control;
}

/**
 * @brief Gets the control field of HDLC header
 *
 * @param hdr pointer to HDLC header
 *
 * @return control field of HDLC header
 */
static inline uint8_t hdlc_hdr_get_control(hdlc_hdr_t *hdr)
{
    return hdr->control;
}

/**
 * @brief Sets the protocol of HDLC header
 *
 * @param hdr pointer to HDLC header
 * @param protocol protocol of HDLC header
 */
static inline void hdlc_hdr_set_protocol(hdlc_hdr_t *hdr, uint16_t protocol)
{
    hdr->protocol = byteorder_htons(protocol);
}

/**
 * @brief Gets the protocol of HDLC header
 *
 * @param hdr pointer to HDLC header
 *
 * @return protocol of HDLC header
 */
static inline uint16_t hdlc_hdr_get_protocol(hdlc_hdr_t *hdr)
{
    return byteorder_ntohs(hdr->protocol);
}

#ifdef __cplusplus
}
#endif

#endif /* HDLC_HDR_H */
/** @} */
