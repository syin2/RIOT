/*
 * Copyright (C) 2014 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       SIM900 module
 *
 * @author      José Ignacio Alamos <jialamos@uc.cl>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>

#include "periph/uart.h"
#include "board.h"
#include "sim900.h"
#include "mutex.h"
#include "kernel.h"
#include "xtimer.h"

#define ENABLE_DEBUG    (1)
#include "debug.h"


char thread_stack[THREAD_STACKSIZE_MAIN];	
//Received bytes. Handle states
 /*
 Data could be:
 - Response of AT command
 - URC
 - Rx
 */
static void rx_cb(void *arg, char data)
{
	//UART device that sent data.
    sim900_t *dev = (sim900_t*) arg;
    msg_t msg;
    msg.type = NETDEV2_MSG_TYPE_EVENT;
	msg.content.value = data;
    msg_send_int(&msg, dev->mac_pid);
}

static int sim900_init(sim900_t *dev, uart_t uart, uint32_t baud)
{
	dev->uart = (uart_t) uart;
	memset(dev->resp_buf,'\0',SIM900_MAX_RESP_SIZE);
    memset(dev->tx_buf,'\0',SIM900_MAX_CMD_SIZE);
	dev->state=AT_STATE_IDLE;
	dev->resp_count = 0;
	dev->_lim_esc = 0; //Number of escape strings of current AT command
	dev->_num_esc = 0; //Count of escape strings;
	dev->b_CR = FALSE; //flag for receiving a CR.
	dev->urc_counter = 0;
	dev->pdp_state = PDP_IDLE;

	//mutex_init(&(dev->resp_lock));
	//mutex_lock(&(dev->resp_lock));
	/* initialize UART */
	uint8_t res;
    res = uart_init(dev->uart, baud, rx_cb, dev);
    if (res == -1) {
        return 1;
    }
    else if (res < -1) {
        return 1;
    }
    //Set current thread to mac_pid
    dev->mac_pid = thread_getpid();
    xtimer_usleep(100);
    //Initiate response buffer
	return 0;
}

void connection(sim900_t *dev)
{
	switch(dev->pdp_state)
	{
		case PDP_NOTSIM:
			if(dev->at_status == AT_OK)
			{
				printf("Worked!");
			}
			break;
	}
}

void isr(void *args)
{
	sim900_t *dev = (sim900_t*) dev;

	uint8_t has_esc = (dev->_stream & 0x00FF) == STREAM_CR;
	uint8_t has_ok = (dev->_stream) == STREAM_OK;
		
	switch(dev->state)
	{
		case AT_STATE_CMD:
			dev->_num_esc -= (has_esc>0)
			if(has_ok)
			{
				dev->at_status = AT_STATUS_OK;
			}
			if(!dev->_num_esc)
			{
				connection(dev);
			}

	}
	dev->_stream = (dev->_stream << 8) + c;
}
int main(void)
{
    sim900_t dev;
	kernel_pid_t pid = thread_create(thread_stack, sizeof(thread_stack), THREAD_PRIORITY_MAIN-1, THREAD_CREATE_STACKTEST, sim900_thread, &dev, "sim900");

	while(1)
	{
		
	}

    return 0;
}

void *sim900_thread(void *args)
{
    //Setup a new sim900 devide

	sim900_t *dev = (sim900_t*) args;
    sim900_init(dev,1,96000);

	msg_t msg, msg_queue[SIM900_MSG_QUEUE];;
	msg_init_queue(msg_queue, SIM900_MSG_QUEUE);
	char cmd[] = "AT+CPIN=0000\r\n";
	uart_write(dev->uart, cmd, sizeof(cmd)-1); 
	dev->state = AT_STATE_CMD;
	dev->pdp_state = PDP_NOTSIM;

    while(1)
    {
    	msg_receive(&msg);
		switch(msg->type){
			case NETDEV2_MSG_TYPE_EVENT:
				dev->_stream += msg.content.value;
				isr(dev);
				break;
    	}
#endif
			
    }
}
