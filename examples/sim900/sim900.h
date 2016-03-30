#ifndef SIM900_H
#define SIM900_H

#include "kernel.h"
#include "mutex.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SIM900_MAX_RESP_SIZE (100U)
#define SIM900_MAX_CMD_SIZE (100U)
#define SIM900_URC_SIZE (16U)

#define AT_OK "\r\nOK"
#define AT_ST "\r\nSTATUS:"



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
	AT_STATE_URC
} dev_state_t;

typedef enum {
	PDP_IDLE,
	PDP_SIMREADY,
	PDP_NETATTACH,
	PDP_ACT,
} pdp_state_t;

typedef struct {
	uint8_t status;			/**< status of AT command */
    uint8_t data[100];        /**< returned data from the AT command */
    uint8_t data_len;       /**< number ob bytes written to @p data */
    uint8_t raw[100];
    //uint8_t raw[256];
} resp_t;

typedef struct {
	uint8_t data[256];
	uint8_t data_len;
} raw_t;

typedef struct {
	uart_t uart;
	uint8_t resp_buf[SIM900_MAX_RESP_SIZE];
	uint8_t tx_buf[SIM900_MAX_CMD_SIZE];
	uint8_t urc_buf[SIM900_URC_SIZE];
	uint8_t urc_counter;
	dev_state_t state;
	uint8_t resp_count;
	uint8_t _lim_esc; //Number of escape strings of current AT command
	uint8_t _num_esc; //Count of escape strings;
	uint8_t b_CR; //flag for receiving a CR.
	mutex_t resp_lock;
	uint8_t prev_state;
	uint8_t pin[4];
	kernel_pid_t mac_pid;
	pdp_state_t pdp_state;

} sim900_t;

//static int get_AT(sim900_t*, resp_t*);
static int sim900_init(sim900_t*, uart_t, uint32_t);

#ifdef __cplusplus
}
#endif


#endif
