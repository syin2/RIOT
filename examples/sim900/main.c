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
#include <sys/uio.h>

#include "periph/uart.h"
#include "board.h"
#include "sim900.h"

#define PPPINITFCS16    0xffff
#define PPPGOODFCS16    0xf0b8

#define ENABLE_DEBUG    (1)
#include "debug.h"

#define TEST_PPP (1)
#define TEST_WRITE (0)


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
	dev->at_status |= (dev->_stream == STREAM_OK)*HAS_OK;
	dev->at_status |= (dev->_stream == STREAM_ERROR)*HAS_ERROR;
	dev->at_status |= (dev->_stream == STREAM_CONN)*HAS_CONN;

	uint8_t c;
	switch(dev->state)
	{
		case AT_STATE_CMD:
			dev->_num_esc -= (dev->_stream & 0xFFFF) == STREAM_CR;
			if(!dev->_num_esc)
			{
				dev->state = AT_STATE_IDLE;
				msg_send_int(&msg, dev->mac_pid);
			}
			break;
		case AT_STATE_RX:
			//If received a flag secuence
			if(data == 0x7e)
			{
				if(!dev->ppp_rx_state == PPP_RX_IDLE)
				{
					//Finished data
					msg.content.value = RX_FINISHED;
					dev->ppp_rx_state = PPP_RX_IDLE;
					dev->rx_count = dev->int_count;
					dev->fcs = dev->int_fcs;
					dev->int_count = 0;
					dev->int_fcs = PPPINITFCS16;
					dev->escape = 0;
					if(dev->fcs == PPPGOODFCS16 && dev->escape == 0)
					{	
						msg_send_int(&msg, dev->mac_pid);
					}
					else
					{
						puts("Bad");
					}
				}
			}
			else if (data == 0x7d) //Escape character
			{
				dev->ppp_rx_state = PPP_RX_STARTED;
				//Escape next character
				dev->escape = 0x20;
			}
			else
			{
				dev->ppp_rx_state = PPP_RX_STARTED;
				//Add XOR'd character
				c = data ^ dev->escape;
				//
				//Checksum
				dev->int_fcs = fcs16_bit(dev->int_fcs, c);
				dev->rx_buf[dev->int_count++] = c;
				dev->escape = 0;
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
	dev->rx_count = 0;
	dev->int_count = 0;
	dev->int_fcs = PPPINITFCS16;
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
	dev->at_status = 0;
	uart_write(dev->uart, (uint8_t*) cmd, size); 
	dev->_cb = cb;
	return 0;
}

void sim900_putchar(uart_t uart, uint8_t c)
{
	uint8_t *p = &c;
	//puts("Called");
	//DEBUG("Printing %i\n",c);
	uart_write(uart, p, 1);
}

int sim900_recv(netdev2_t *ppp_dev, char *buf, int len, void *info)
{
	sim900_t *dev = (sim900_t*) ppp_dev;
	int payload_length = dev->rx_count-2;
	if(buf)
	{
		memcpy(buf, dev->rx_buf, payload_length);
	}
	return payload_length;
}
int sim900_send(netdev2_t *ppp_dev, const struct iovec *vector, int count)
{
	sim900_t *dev = (sim900_t*) ppp_dev;
	uint16_t fcs = PPPINITFCS16;
	/* Send flag */
	sim900_putchar(dev->uart, (uint8_t) 0x7e);
	uint8_t c;
	for(int i=0;i<count;i++)
	{
		for(int j=0;j<vector[i].iov_len;j++)
		{
			c = *(((uint8_t*)vector[i].iov_base)+j);
			if(c == 0x7e || c == 0x7d || c == 0x03)
			{
				sim900_putchar(dev->uart, (uint8_t) 0x7d);
				sim900_putchar(dev->uart, (uint8_t) c ^ 0x20);
			}
			else
			{
				sim900_putchar(dev->uart, c);
			}
			fcs = fcs16_bit(fcs, c);
		}
	}
	/*Write checksum and flag*/
	fcs ^= 0xffff;
	sim900_putchar(dev->uart, (uint8_t) fcs & 0x00ff);
	sim900_putchar(dev->uart, (uint8_t) (fcs >> 8) & 0x00ff);
	sim900_putchar(dev->uart, (uint8_t) 0x7e);
	return 0;
}

void test_sending(sim900_t *dev)
{
	uint8_t pkt[] = {0xff, 0x03, 0xc0, 0x21, 0x01,0x01,0x00,0x04};
	struct iovec vector;
	vector.iov_base = &pkt;
	vector.iov_len = 8;
	sim900_send((netdev2_t*) dev, &vector, 1);
}
void events(sim900_t *dev)
{
	int event = dev->msg.content.value;
	switch(event)
	{
		case MSG_AT_FINISHED:
			dev->_cb(dev);
			break;
		case MSG_AT_TIMEOUT:
			dev->_timer_cb(dev);
			break;
		case PDP_UP:
			DEBUG("Welcome to PPP :)\n");
			/*Trigger LCP up event*/
			//test_sending(dev);
			gnrc_ppp_event_callback(&dev->ppp_dev, PPP_LINKUP);
			break;
		case RX_FINISHED:
			if(dev->rx_count < 4)
			{
				DEBUG("Frame too short!");
			}
			else
			{
				if(dev->fcs == PPPGOODFCS16 && dev->escape == 0)
				{
					DEBUG("Good message! :)\n");
					gnrc_ppp_event_callback(&dev->ppp_dev, PPP_RECV);
					
				}
				else
				{
					DEBUG("Bad message! :(\n");
				}
			}
			break;
		default:
			gnrc_ppp_event_callback(&dev->ppp_dev, event);
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
	if(dev->at_status & HAS_CONN)
	{
		puts("Successfully entered data mode");
		dev->state = AT_STATE_RX;
		dev->ppp_rx_state = PPP_RX_IDLE;
		dev->msg.type = NETDEV2_MSG_TYPE_EVENT;
		dev->msg.content.value = PDP_UP;
		msg_send(&dev->msg, dev->mac_pid);
	}
	else
	{
		puts("Failed to enter data mode");
	}
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
	if(dev->at_status &  HAS_ERROR)
	{
		//Set timeout
		at_timeout(dev, 10000000U, &pdp_netattach_timeout);
	}
	else
	{
		puts("Network attach!.");
		send_at_command(dev, "AT+CGDCONT=1,\"IP\",\"mmsbouygtel.com\"\r\n", 37, 3, &pdp_activate);
	}
}



void pdp_context_attach(sim900_t *dev)
{
	if(dev->at_status & HAS_OK)
	{
		puts("PDP attached!");
	}
}

void pdp_nosim(sim900_t *dev)
{
	if(dev->at_status & HAS_OK)
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

void *sim900_thread(void *args)
{
    //Setup a new sim900 devide

	sim900_t *dev = (sim900_t*) args;
    sim900_init(dev,1,96000);

	msg_t msg_queue[SIM900_MSG_QUEUE];;
	msg_init_queue(msg_queue, SIM900_MSG_QUEUE);
#if TEST_PPP
	dev->state = AT_STATE_RX;
	dev->ppp_rx_state = PPP_RX_IDLE;
	dev->msg.type = NETDEV2_MSG_TYPE_EVENT;
	dev->msg.content.value = PDP_UP;
	msg_send(&dev->msg, dev->mac_pid);
#else
	dev->pdp_state = PDP_NOTSIM;
	/*Start sending an AT command */
	send_at_command(dev, "AT+CPIN=0000\r\n",14, 3, &pdp_nosim);
#endif
#if TEST_WRITE
	test_sending(dev);
#endif

    while(1)
    {
    	msg_receive(&dev->msg);
		switch(dev->msg.type){
			case NETDEV2_MSG_TYPE_EVENT:
				events(dev);
				break;
    	}
			
    }
}

int main(void)
{
	gnrc_pktbuf_init();
	netdev2_driver_t driver;
	driver.send = &sim900_send;
	driver.recv = &sim900_recv;
    sim900_t dev;
	dev.netdev.driver = &driver;
	gnrc_ppp_init(&dev.ppp_dev, (netdev2_t*) &dev);

	xtimer_init();
	kernel_pid_t pid = thread_create(thread_stack, sizeof(thread_stack), THREAD_PRIORITY_MAIN-1, THREAD_CREATE_STACKTEST*2, sim900_thread, &dev, "sim900");
	(void) pid;

	while(1)
	{
		
	}

    return 0;
}

