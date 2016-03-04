/*
 * Copyright (C) 2015 José Ignacio Alamos
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for
 * more details.
 */

/**
 * @defgroup    net_ppptypes PPP types
 * @ingroup     net
 * @brief       PPP types

 * @note        Last Updated: 2016-02-04
 * @{
 *
 * @file
 * @brief       PPP type definitions
 *
 * @author      José Ignacio Alamos <jialamos@uc.cl>
 */


#ifndef PPPTYPE_H_
#define PPPTYPE_H_

#ifdef __cplusplus
extern "C" {
#endif


#define PPPTYPE_IPV6       (0x0057)    /**< IPv6 packet in PPP*/
#define PPPTYPE_NCP_IPV6       (0x8057)    /**< NCP packet*/
#define PPPTYPE_LCP       (0xC021)    /**< LCP packet  */
#define PPPTYPE_UNKNOWN       (0xFFFF)    /**<Unknown  packet (Random code) */

#ifdef __cplusplus
}
#endif

#endif /* PPPTYPE_H_ */
/**
 * @}
 */
