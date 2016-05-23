#ifndef PPP_FSM_H_
#define PPP_FSM_H_

#include "net/gnrc/ppp/opt.h"
#include "net/gnrc/ppp/prot.h"

#ifdef __cplusplus
extern "C" {
#endif


/*Function flags*/
#define F_TLU (1U<<0)
#define F_TLD (1U<<1)
#define F_TLS (1U<<2)
#define F_TLF (1U<<3)
#define F_IRC (1U<<4)
#define F_ZRC (1U<<5)
#define F_SCR (1U<<6)
#define F_SCA (1U<<7)
#define F_SCN (1U<<8)
#define F_STR (1U<<9)
#define F_STA (1U<<10)
#define F_SCJ (1U<<11)
#define F_SER (1U<<12)

#define PPP_MAX_TERMINATE (3)
#define PPP_MAX_CONFIG (10)

#define OPT_PAYLOAD_BUF_SIZE (100)
typedef enum{
	E_UP,
	E_DOWN,
	E_OPEN,
	E_CLOSE,
	E_TOp,
	E_TOm,
	E_RCRp,
	E_RCRm,
	E_RCA,
	E_RCN,
	E_RTR,
	E_RTA,
	E_RUC,
	E_RXJp,
	E_RXJm,
	E_RXR,
	PPP_NUM_EVENTS
} fsm_event_t;


typedef enum{
	S_UNDEF=-1,
	S_INITIAL=0,
	S_STARTING,
	S_CLOSED,
	S_STOPPED,
	S_CLOSING,
	S_STOPPING,
	S_REQ_SENT,
	S_ACK_RCVD,
	S_ACK_SENT,
	S_OPENED,
	PPP_NUM_STATES
} fsm_state_t;


/* state transition for control layer FSM */
static const int8_t state_trans[PPP_NUM_EVENTS][PPP_NUM_STATES] = {
{S_CLOSED,S_REQ_SENT,S_UNDEF,S_UNDEF,S_UNDEF,S_UNDEF,S_UNDEF,S_UNDEF,S_UNDEF,S_UNDEF},
{S_UNDEF,S_UNDEF,S_INITIAL,S_STARTING,S_INITIAL,S_STARTING,S_STARTING,S_STARTING,S_STARTING,S_STARTING},
{S_STARTING,S_STARTING,S_REQ_SENT,S_STOPPED,S_STOPPING,S_STOPPING,S_REQ_SENT,S_ACK_RCVD,S_ACK_SENT,S_OPENED},
{S_INITIAL,S_INITIAL,S_CLOSED,S_CLOSED,S_CLOSING,S_CLOSING,S_CLOSING,S_CLOSING,S_CLOSING,S_CLOSING},
{S_UNDEF,S_UNDEF,S_UNDEF,S_UNDEF,S_CLOSING,S_STOPPING,S_REQ_SENT,S_REQ_SENT,S_ACK_SENT,S_UNDEF},
{S_UNDEF,S_UNDEF,S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_STOPPED,S_STOPPED,S_STOPPED,S_UNDEF},
{S_UNDEF,S_UNDEF,S_CLOSED,S_ACK_SENT,S_CLOSING,S_STOPPING,S_ACK_SENT,S_OPENED,S_ACK_SENT,S_ACK_SENT},
{S_UNDEF,S_UNDEF,S_CLOSED,S_REQ_SENT,S_CLOSING,S_STOPPING,S_REQ_SENT,S_ACK_RCVD,S_REQ_SENT,S_REQ_SENT},
{S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_CLOSING,S_STOPPING,S_ACK_RCVD,S_UNDEF,S_OPENED,S_UNDEF},
{S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_CLOSING,S_STOPPING,S_REQ_SENT,S_UNDEF,S_ACK_SENT,S_UNDEF},
{S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_CLOSING,S_STOPPING,S_REQ_SENT,S_REQ_SENT,S_REQ_SENT,S_STOPPING},
{S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_CLOSED,S_STOPPED,S_REQ_SENT,S_REQ_SENT,S_ACK_SENT,S_REQ_SENT},
{S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_CLOSING,S_STOPPING,S_REQ_SENT,S_ACK_RCVD,S_ACK_SENT,S_OPENED},
{S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_CLOSING,S_STOPPING,S_REQ_SENT,S_REQ_SENT,S_ACK_SENT,S_OPENED},
{S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_CLOSED,S_STOPPED,S_STOPPED,S_STOPPED,S_STOPPED,S_STOPPING},
{S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_CLOSING,S_STOPPING,S_REQ_SENT,S_ACK_RCVD,S_ACK_SENT,S_OPENED}};



typedef struct ppp_fsm_t ppp_fsm_t;
typedef struct cp_conf_t cp_conf_t;


/* Functions flags for each state */
static const uint16_t actions[PPP_NUM_EVENTS][PPP_NUM_STATES] = {
{0,F_IRC | F_SCR,0,0,0,0,0,0,0,0},
{0,0,0,F_TLS,0,0,0,0,0,F_TLD},
{F_TLS,0,F_IRC | F_SCR,0,0,0,0,0,0,0},
{0,F_TLF,0,0,0,0,F_IRC | F_STR,F_IRC | F_STR,F_IRC | F_STR,F_TLD | F_IRC | F_STR},
{0,0,0,0,F_STR,F_STR,F_SCR,F_SCR,F_SCR,0},
{0,0,0,0,F_TLF,F_TLF,F_TLF,F_TLF,F_TLF,0},
{0,0,F_STA,F_IRC | F_SCR | F_SCA,0,0,F_SCA,F_SCA | F_TLU,F_SCA,F_TLD | F_SCR | F_SCA},
{0,0,F_STA,F_IRC | F_SCR | F_SCN,0,0,F_SCN,F_SCN,F_SCN,F_TLD | F_SCR | F_SCN},
{0,0,F_STA,F_STA,0,0,F_IRC,F_SCR,F_IRC | F_TLU,F_TLD | F_SCR},
{0,0,F_STA,F_STA,0,0,F_IRC | F_SCR,F_SCR,F_IRC | F_SCR,F_TLD | F_SCR},
{0,0,F_STA,F_STA,F_STA,F_STA,F_STA,F_STA,F_STA,F_TLD | F_ZRC | F_STA},
{0,0,0,0,F_TLF,F_TLF,0,0,0,F_TLD | F_SCR},
{0,0,F_SCJ,F_SCJ,F_SCJ,F_SCJ,F_SCJ,F_SCJ,F_SCJ,F_SCJ},
{0,0,0,0,0,0,0,0,0,0},
{0,0,F_TLF,F_TLF,F_TLF,F_TLF,F_TLF,F_TLF,F_TLF,F_TLD | F_IRC | F_STR},
{0,0,0,0,0,0,0,0,0,F_SER}};

/* Control Protocol struct*/
typedef struct ppp_fsm_t{
	ppp_protocol_t prot;
	gnrc_nettype_t prottype;
	uint16_t supported_codes;
	uint8_t state;
	uint8_t restart_counter;
	uint32_t restart_timer;
	xtimer_t xtimer;
	uint8_t cr_sent_identifier;
	uint8_t cr_sent_opts[OPT_PAYLOAD_BUF_SIZE];
	uint16_t cr_sent_size;
	uint8_t tr_sent_identifier;
	cp_conf_t* (*get_conf_by_code)(ppp_fsm_t *cp, uint8_t code);
	cp_conf_t *conf;
	uint16_t targets;
} ppp_fsm_t;

typedef struct cp_conf_t
{
	uint8_t type;
	network_uint32_t value;
	network_uint32_t default_value;
	size_t size;
	uint8_t flags;
	uint8_t (*is_valid)(ppp_option_t *opt);
	uint8_t (*build_nak_opts)(ppp_option_t *opt);
	void (*set)(ppp_fsm_t *t, ppp_option_t *opt, uint8_t peer);
	struct cp_conf_t *next;
} cp_conf_t;

int fsm_init(struct gnrc_pppdev_t *ppp_dev, ppp_fsm_t *cp);
int trigger_fsm_event(ppp_fsm_t *cp, int event, gnrc_pktsnip_t *pkt);
int fsm_handle_ppp_msg(struct ppp_protocol_t *protocol, uint8_t ppp_event, void *args); 

#ifdef __cplusplus
}
#endif
#endif
