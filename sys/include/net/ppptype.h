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

//Get PPP type from protocol 
#define PPPTYPE(x)(x>>14)

#define PPP_PKT       (0x0)    /**< NCP related packet segment */
#define PPP_NONCP       (0x1)    /**< NO NCP segment */
#define PPP_NCP       (0x2)    /**< NCP segment */
#define PPP_LCP       (0x3)    /**< LCP segment */

#ifdef __cplusplus
}
#endif

#endif /* PPPTYPE_H_ */
/**
 * @}
 */
