#ifndef SIM900_H
#define SIM900_H

#include "mutex.h"
#include "thread.h"
#include "xtimer.h"
#include "net/netdev2.h"
#include "net/gnrc.h"
#include "net/gnrc/ppp/ppp.h"
#include "net/hdlc/fcs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SIM900_MAX_RESP_SIZE (100U)
#define SIM900_MAX_CMD_SIZE (100U)
#define SIM900_URC_SIZE (16U)


#define AT_STATUS_OK (1)
#define AT_STATUS_ERROR (2)

#define STREAM_CR (0x0D0A)
#define STREAM_OK (0x0D0A4F4B)
#define STREAM_ERROR (0x4552524F)
#define STREAM_CONN (0x434F4E4E)

#define HAS_OK (1)
#define HAS_ERROR (2)
#define HAS_CONN (4)


#define RX_CTRL (1)
#define RX_FLAG (2)

#define SIM900_MSG_QUEUE 64 

#define MSG_AT_FINISHED (0)
#define MSG_AT_TIMEOUT (1)
#define PDP_UP (2)
#define RX_FINISHED (3)

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef enum {
	SIM900_INT_STATE_IP_INITIAL,
	SIM900_INT_STATE_PDP_DEACT
} sim900_rx_state_t;

typedef enum {
	AT_STATE_IDLE,
	AT_STATE_CMD,
	AT_STATE_RX,
} dev_state_t;

typedef enum{
	PPP_RX_STARTED,
	PPP_RX_IDLE
} ppp_rx_state_t;

typedef enum {
	PDP_NOTSIM,
	PDP_IDLE,
	PDP_NETATTACH,
	PDP_ACT,
} pdp_state_t;


typedef struct resp_t{
	uint8_t status;			/**< status of AT command */
    uint8_t data[100];        /**< returned data from the AT command */
    uint8_t data_len;       /**< number ob bytes written to @p data */
    uint8_t raw[100];
    //uint8_t raw[256];
} resp_t;

typedef struct raw_t{
	uint8_t data[256];
	uint8_t data_len;
} raw_t;

typedef struct sim900_t {
	pppdev_t netdev;
	uart_t uart;
	uint8_t resp_buf[SIM900_MAX_RESP_SIZE];
	uint8_t tx_buf[SIM900_MAX_CMD_SIZE];
	uint8_t rx_buf[SIM900_MAX_CMD_SIZE];
	uint16_t rx_count;
	uint16_t int_count;
	uint8_t urc_counter;
	dev_state_t state;
	uint8_t resp_count;
	uint8_t b_CR; //flag for receiving a CR.
	mutex_t resp_lock;
	uint8_t prev_state;
	uint8_t pin[4];
	kernel_pid_t mac_pid;
	pdp_state_t pdp_state;

	char _c; //Received character;
	uint8_t _num_esc; //Count of escape strings;

	uint32_t _stream;
	uint8_t at_status;

	 xtimer_t xtimer;
	uint8_t pdp_set;
	void (*_cb)(struct sim900_t *dev);
	void (*_timer_cb)(struct sim900_t *dev);
	msg_t msg;

	//PPP
	uint8_t rx_flags;
	ppp_rx_state_t ppp_rx_state;
	uint8_t escape;
	uint16_t fcs;
	uint16_t int_fcs;
	
	uint32_t tx_accm;
	uint32_t rx_accm;
} sim900_t;

typedef struct sim900_params_t
{
	uart_t uart;
} sim900_params_t;

//static int get_AT(sim900_t*, resp_t*);
void pdp_netattach_timeout(sim900_t *dev);
void pdp_netattach(sim900_t *dev);
void pdp_check_netattach(sim900_t *dev);
#ifdef __cplusplus
}
#endif


#endif
