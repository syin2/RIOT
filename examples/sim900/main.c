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
    msg.type = dev->state;
	msg.content.value = data;
    msg_send_int(&msg, dev->mac_pid);
}

static int _at_cmd(sim900_t *dev, const char *cmd,  uint8_t lim_esc, raw_t* raw)
{
	//Copy command to buffer
	int cmd_size = strlen(cmd);
	memcpy(dev->tx_buf,cmd,cmd_size);

	//Add AT specific CR & LF
	dev->tx_buf[cmd_size] = '\r';
	dev->tx_buf[cmd_size+1] = '\n';

	//Send AT command through UART
	uart_write(dev->uart,dev->tx_buf,cmd_size+2);


	dev->resp_count = 0;
	dev->_num_esc = 0;
	dev->_lim_esc = lim_esc;
	//Wait for reply (flag)
	dev->prev_state = dev->state;
	dev->state = AT_STATE_CMD;
	//mutex_lock(&(dev->resp_lock));
	msg_t msg;
	while(1){
		msg_receive(&msg);
		dev->resp_buf[dev->resp_count++] = msg.content.value;
    		// \todo{Check buffer overflow}
    		//If CR, set CR flag
    		if (dev->b_CR == TRUE && msg.content.value == '\n')
    		{
    			//We have an escape string at this point... increment _num_esc
    			dev->_num_esc++;
    		}
    		if(dev->_num_esc == dev->_lim_esc)
    		{
    			//mutex_unlock(&(dev->resp_lock));
    			dev->state = dev->prev_state;
    			break;
    		}
    		if (msg.content.value == '\r')
    		{
    			dev->b_CR = TRUE;
    		}
	}

	//Populate response
	uint8_t data_len = strlen((char*) dev->resp_buf);
	memcpy(raw->data,dev->resp_buf,sizeof(raw->data));
	raw->data_len = data_len;
	memset(dev->resp_buf,'\0',SIM900_MAX_RESP_SIZE);
	//DEBUG("Data: %s",raw->data);
	return 0;
}
/*
static int test_at_cmd(sim900_t *dev)
{
	raw_t raw;
	_at_cmd(dev,"AT",2,&raw);
	//LED_RED_OFF;
	puts("_at_cmd working");
	return 0;
}*/
static int get_AT(sim900_t *dev, resp_t *resp)
{
	raw_t raw;
	_at_cmd(dev,"AT",3,&raw);
	//Populate
	if(strstr((char*)raw.data,(char*) "\r\nOK")) {
		resp->status = 1;
		resp->data[0] = 'O';
		resp->data[1] = 'K';
		resp->data_len = 2;
	}
	else {
		resp->status = 0;
		resp->data[0] = 'E';
		resp->data[1] = 'R';
		resp->data_len = 2;
	}
	//LED_RED_OFF;
	return 0;
}

static int AT_CPIN(sim900_t *dev, resp_t *resp)
{
	raw_t raw;
	_at_cmd(dev,"AT+CPIN?",5,&raw);
	//Treat CME ERROR of no SIM
	return 0;
}

static int AT_CREG(sim900_t *dev, resp_t *resp)
{
	raw_t raw;
	_at_cmd(dev, "AT+CREG?",5,&raw);
	return 0;
}

static int AT_CGATT(sim900_t *dev, resp_t *resp)
{
	raw_t raw;
	_at_cmd(dev, "AT+CGATT?",5,&raw);
	return 0;
}

static int AT_CIPSHUT(sim900_t *dev, resp_t *resp)
{
	raw_t raw;
	_at_cmd(dev, "AT+CIPSHUT",3,&raw);
	return 0;
}
static int AT_CIPSTATUS(sim900_t *dev, resp_t *resp)
{
	raw_t raw;
	_at_cmd(dev, "AT+CIPSTATUS",5,&raw);
	return 0;
}
static int AT_CIPMUX(sim900_t *dev, resp_t *resp)
{
	raw_t raw;
	_at_cmd(dev, "AT+CIPMUX=0",3,&raw);
	return 0;
}
static void unlock_sim(sim900_t *dev)
{
	raw_t raw;
	char cmd[] = "AT+CPIN=0000";
	memcpy(cmd+sizeof(cmd)-4, dev->pin, 4);
	_at_cmd(dev, cmd. &raw);
}
static int AT_CSTT(sim900_t *dev, resp_t *resp)
{
	raw_t raw;
	_at_cmd(dev, "AT+CSTT=\"web.vodafone.de\",\"vodafone\",\"vodafone\"",3,&raw);
	return 0;
}

static int AT_CIICR(sim900_t *dev, resp_t *resp)
{
	raw_t raw;
	_at_cmd(dev, "AT+CIICR",3,&raw);
	if(raw.data[13] == 'K')
	{
		resp->status = 1;
	}
	else
	{
		resp->status = 0;
	}
	return 0;
}

static int AT_CIFSR(sim900_t *dev, resp_t *resp)
{
	raw_t raw;
	_at_cmd(dev, "AT+CIFSR",3,&raw);
	return 0;
}

static int AT_CIPSTART(sim900_t *dev, resp_t *resp)
{
	raw_t raw;
	_at_cmd(dev, "AT+CIPSTART=\"TCP\",\"www.github.com\",\"80\"",3,&raw);
	return 0;
}

static int AT_CIPSEND(sim900_t *dev, resp_t *resp)
{
	raw_t raw;
	_at_cmd(dev, "AT+CIPSEND=6\r\n",3,&raw);
	_at_cmd(dev, "prueba",3,&raw);
	return 0;
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

static int get_PDP(sim900_t *dev, resp_t *resp)
{
	    AT_CPIN(dev, resp);
	    AT_CREG(dev, resp);
	    AT_CGATT(dev, resp);
	    AT_CIPSHUT(dev, resp);
	    AT_CIPSTATUS(dev, resp);
	    AT_CIPMUX(dev, resp);
	    AT_CSTT(dev, resp);
	    AT_CIPSTATUS(dev, resp);
	    AT_CIICR(dev, resp);
	    return 0;
}

static pdp_state_t pdp_idle(sim900_t *dev, int event)
{
	/* Check SIM state */
	if(event == E_SIMREADY)
	{
		/* Set timeout for network attach */
		return PDP_SIMREADY;
	}


}
static int TCP_open(sim900_t *dev, const uint8_t *address, uint8_t port) {
    resp_t resp;
    do{
    	get_AT(dev, &resp);
		get_PDP(dev, &resp);
    }
	while(resp.status == 0);
	
	AT_CIPSTATUS(dev, &resp);
	AT_CIFSR(dev, &resp);
	AT_CIPSTART(dev, &resp);
	dev->state = AT_STATE_RX;
	return 0;
}
int main(void)
{
    //Setup a new sim900 devide
    sim900_t dev;

    //Setup uart
        //LED_RED_ON;
    sim900_init(&dev,1,96000);

    //Test get_AT
	resp_t resp;
	AT_CIPSEND(&dev,&resp);
	
	msg_t msg;
    while(1)
    {
    	msg_receive(&msg);
    	//Check if a URC was sent.
#if 0 switch(dev.state) {
    		case AT_STATE_RX:
    			DEBUG("%c",(char)msg.content.value);
    			if(msg.content.value == '\r') {
	    			//URC
	    			LED_RED_OFF;
	    			dev.state = AT_STATE_URC;
	    			dev.urc_counter = 0;
	    			dev.urc_buf[dev.urc_counter++] = '\r';
    			}
    			break;
    		case AT_STATE_URC:
    			//DEBUG("%s",dev.urc_buf);
    			dev.urc_buf[dev.urc_counter++] = msg.content.value;
    			if(strncmp((char*) dev.urc_buf,"\r\nCLOSED\r\n",10) == 0)
    			{
	    			//URC
	    			LED_RED_ON;
	    			TCP_open(&dev,(uint8_t*) "www.github.com",80);
	    			xtimer_usleep(1000000);
					AT_CIPSEND(&dev,&resp);
    			}
    			break;
    		default:
    			break;
    	}
#endif
			
    }
    return 0;
}
