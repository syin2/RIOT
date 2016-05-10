/*
 * Copyright (C) 2015 José Ignacio Alamos <jialamos@uc.cl>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 */

/**
 * @ingroup auto_init_gnrc_netif
 * @{
 *
 * @file
 * @brief   Auto initialization for SIM900 module
 *
 * @author  José Ignacio Alamos   <jialamos@uc.cl>
 */

#ifdef MODULE_SIM900

#include "sim900.h"
#include "periph/uart.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

/**
 * @brief   Define stack parameters for the PPP layer thread
 * @{
 */
#define SIM900_STACKSIZE           (THREAD_STACKSIZE_DEFAULT + DEBUG_EXTRA_STACKSIZE)
#define SIM900_PRIO                (THREAD_PRIORITY_MAIN - 4)

/**
 * @brief   Stacks for the MAC layer threads
 */
static char _pppdev_stack[MAC_STACKSIZE];
static gnrc_pppdev_t _gnrc_sim900;

static uint8_t _inbuf[2048];

void auto_init_sim900(void)
{
    DEBUG("auto_init_sim900(): initializing device...\n");

    /* setup netdev2 device */
    ethos_params_t p;
    p.uart      = ETHOS_UART;
    p.baudrate  = ETHOS_BAUDRATE;
    p.buf       = _inbuf;
    p.bufsize   = sizeof(_inbuf);
    ethos_setup(&ethos, &p);

    /* initialize netdev2<->gnrc adapter state */
    gnrc_netdev2_eth_init(&_gnrc_ethos, (netdev2_t*)&ethos);

    /* start gnrc netdev2 thread */
    gnrc_netdev2_init(_netdev2_eth_stack, MAC_STACKSIZE,
            MAC_PRIO, "gnrc_ethos", &_gnrc_ethos);
}

#else
typedef int dont_be_pedantic;
#endif
