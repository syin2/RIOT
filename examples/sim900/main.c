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
#include <errno.h>

#include "periph/uart.h"
#include "board.h"
#include "sim900.h"

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

static void rx_cb(void *arg, uint8_t data)
{
	//UART device that sent data.
	
	sim900_t *dev = (sim900_t*) arg;
    msg_t msg;
    msg.type = NETDEV2_MSG_TYPE_EVENT;
	msg.content.value = MSG_AT_FINISHED;

	dev->_stream += data;

	switch(dev->state)
	{
		case AT_STATE_CMD:
			dev->_num_esc -= (dev->_stream & 0xFFFF) == STREAM_CR;
			if(dev->_stream == STREAM_OK)
			{
				dev->at_status = AT_STATUS_OK;
			}
			else if (dev->_stream == STREAM_ERROR)
			{
				dev->at_status = AT_STATUS_ERROR;
			}
			if(!dev->_num_esc)
			{
				dev->state = AT_STATE_IDLE;
				msg_send_int(&msg, dev->mac_pid);
			}
			break;
		default:
			break;

	}
	dev->_stream = (dev->_stream << 8);
}

static int sim900_init(sim900_t *dev, uart_t uart, uint32_t baud)
{
	dev->uart = (uart_t) uart;
	memset(dev->resp_buf,'\0',SIM900_MAX_RESP_SIZE);
    memset(dev->tx_buf,'\0',SIM900_MAX_CMD_SIZE);
	dev->resp_count = 0;
	dev->_num_esc = 0; //Count of escape strings;
	dev->b_CR = FALSE; //flag for receiving a CR.
	dev->urc_counter = 0;
	dev->pdp_state = PDP_IDLE;
	dev->_stream = 0;

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

int send_at_command(sim900_t *dev, char *cmd, size_t size, uint8_t ne, void (*cb)(sim900_t *dev))
{
	if(dev->state == AT_STATE_CMD)
	{
		return -EBUSY;
	}
	dev->state = AT_STATE_CMD;
	dev->_num_esc = ne;
	uart_write(dev->uart, (uint8_t*) cmd, size); 
	dev->_cb = cb;
	return 0;
}

void attach_pdp(sim900_t *dev)
{
}

void events(sim900_t *dev)
{
	puts("H");
	DEBUG("Msg content value: %i\n", (int)dev->msg.content.value); 
	switch(dev->msg.content.value)
	{
		case MSG_AT_FINISHED:
			dev->_cb(dev);
			break;
		case MSG_AT_TIMEOUT:
			dev->_timer_cb(dev);
	}
}

void at_timeout(sim900_t *dev, uint32_t ms, void (*cb)(sim900_t *dev))
{
	dev->msg.type = NETDEV2_MSG_TYPE_EVENT;
	dev->msg.content.value = MSG_AT_TIMEOUT;
	dev->_timer_cb = cb;
	xtimer_set_msg(&dev->xtimer, ms, &dev->msg, dev->mac_pid);
}

void pdp_netattach_timeout(sim900_t *dev)
{
	puts("Still don't get network attach... let's try again");
	send_at_command(dev, "AT+CGATT=1\r\n", 12, 3, &pdp_netattach);
}

void check_data_mode(sim900_t *dev)
{
	puts("Should be in data mode");
}

void pdp_enter_data_mode(sim900_t *dev)
{
	puts("Entering data mode");
	send_at_command(dev, "AT+CGDATA=\"PPP\",1\r\n", 19, 3, &check_data_mode);
}

void pdp_activate(sim900_t *dev)
{
	send_at_command(dev, "AT+CGACT=1,1\r\n",14,3, &pdp_enter_data_mode);
}

void pdp_netattach(sim900_t *dev)
{
	if(dev->at_status == AT_STATUS_ERROR)
	{
		//Set timeout
		puts("HELLO");
		at_timeout(dev, 10000000U, &pdp_netattach_timeout);
	}
	else
	{
		puts("Got connection.");
		send_at_command(dev, "AT+CGDCONT=1,\"IP\",\"mmsbouygtel.com\"\r\n", 37, 3, &pdp_activate);
	}
}


void pdp_set_pdp(sim900_t *dev)
{
	attach_pdp(dev);
}

void pdp_context_attach(sim900_t *dev)
{
	if(dev->at_status == AT_STATUS_OK)
	{
		puts("PDP attached!");
	}
}

void pdp_nosim(sim900_t *dev)
{
	if(dev->at_status == (int) AT_STATUS_OK)
	{
		//Switch to next state
		//dev->pdp_state = PDP_NETATTACH;
		send_at_command(dev, "AT+CGATT=1\r\n", 12, 3, &pdp_netattach);
		puts("Sim working! :)");
	}
	else
	{
		puts("Error unlocking SIM");
	}
}
/*
void connection(sim900_t *dev, msg_t *msg)
{
	uint8_t event = msg->content.value;
	switch(dev->pdp_state)
	{
		case PDP_NOTSIM:
			if(dev->at_status == (int) AT_STATUS_OK)
			{
				//Switch to next state
				dev->pdp_state = PDP_NETATTACH;
				send_attach(dev);
			}
			else
			{
				puts("Error unlocking SIM");
			}
			break;
		case PDP_NETATTACH:
			if(event == 0)
			{
				if(dev->at_status == AT_STATUS_ERROR)
				{
					//Set timeout
					msg->type = NETDEV2_MSG_TYPE_EVENT;
					msg->content.value = 1;
					xtimer_set_msg(&dev->xtimer, 10000000U, msg, dev->mac_pid);

				}
				else
				{
					puts("Got connection.");
					dev->pdp_state = PDP_ACT;
					dev->pdp_set = false;
					send_pdp(dev);
				}
			}
			else
			{
				puts("Still don't get network attach... let's try again");
				send_attach(dev);
			}

			break;
		case PDP_ACT:
		default:
			break;
	}
}
*/

void *sim900_thread(void *args)
{
    //Setup a new sim900 devide

	sim900_t *dev = (sim900_t*) args;
    sim900_init(dev,1,115200);

	msg_t msg_queue[SIM900_MSG_QUEUE];;
	msg_init_queue(msg_queue, SIM900_MSG_QUEUE);
	dev->pdp_state = PDP_NOTSIM;
	send_at_command(dev, "AT+CPIN=0000\r\n",14, 3, &pdp_nosim);

    while(1)
    {
    	msg_receive(&dev->msg);
		switch(dev->msg.type){
			case NETDEV2_MSG_TYPE_EVENT:
				puts("Event\n");
				events(dev);
				break;
    	}
			
    }
}

int main(void)
{
    sim900_t dev;
	xtimer_init();
	kernel_pid_t pid = thread_create(thread_stack, sizeof(thread_stack), THREAD_PRIORITY_MAIN-1, THREAD_CREATE_STACKTEST*2, sim900_thread, &dev, "sim900");
	(void) pid;

	while(1)
	{
		
	}

    return 0;
}

