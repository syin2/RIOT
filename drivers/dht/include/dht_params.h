/*
 * Copyright (C) 2016 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     drivers_dht
 *
 * @{
 * @file
 * @brief       Default configuration for DHT devices
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 */

#ifndef DHT_PARAMS_H
#define DHT_PARAMS_H

#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Set default configuration parameters for the DHT devices
 * @{
 */
#ifndef DHT_PARAM_PIN
#define DHT_PARAM_PIN               (GPIO_PIN(0, 0))
#endif
#ifndef DHT_PARAM_TYPE
#define DHT_PARAM_TYPE              (DHT11)
#endif
#ifndef DHT_PARAM_PULL
#define DHT_PARAM_PULL              (GPIO_PULLUP)
#endif

#define DHT_PARAMS_DEFAULT          {.pin = DHT_PARAM_PIN, \
                                     .type = DHT_PARAM_TYPE, \
                                     .pull = DHT_PARAM_PULL}
/**@}*/

/**
 * @brief   Configure DHT devices
 */
static const dht_params_t dht_params[] =
{
#ifdef DHT_PARAMS_BOARD
    DHT_PARAMS_BOARD,
#else
    DHT_PARAMS_DEFAULT,
#endif
};

/**
 * @brief   Get the number of configured DHT devices
 */
#define DHT_NUMOF       (sizeof(dht_params) / sizeof(dht_params[0]))

#ifdef __cplusplus
}
#endif

#endif /* DHT_PARAMS_H */
/** @} */
