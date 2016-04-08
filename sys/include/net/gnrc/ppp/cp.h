#ifndef GNRC_PPP_CP_H_
#define GNRC_PPP_CP_H_

#include "net/gnrc/pkt.h"
#include "net/gnrc/pktbuf.h"
#include "xtimer.h"
#include "thread.h"
#include "net/gnrc/ppp/opt.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PPP_CONF_REQ (1)
#define PPP_CONF_ACK (2)
#define PPP_CONF_NAK (3)
#define PPP_CONF_REJ (4)
#define PPP_TERM_REQ (5)
#define PPP_TERM_ACK (6)
#define PPP_CODE_REJ (7)
#define PPP_PROT_REJ (8)
#define PPP_ECHO_REQ (9)
#define PPP_ECHO_REP (10)
#define PPP_DISC_REQ (11)
#define PPP_IDENT (12)
#define PPP_TIME_REM (13)

#define CP_CREQ_ACK (0)
#define CP_CREQ_NAK (1)
#define CP_CREQ_REJ (2)

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
} ppp_event_t;


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
} ppp_state_t;



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


#define PPP_MSG_UP (1)
#define PPP_MSG_DOWN (2)

#define PPP_MAX_TERMINATE (3)
#define PPP_MAX_CONFIG (3)

#define PPP_MSG_TIMEOUT (1)

#define PPP_CP_HDR_BASE_SIZE (4)

#define PPP_PAYLOAD_BUF_SIZE (256)

#define GNRC_PPP_MSG_QUEUE_SIZE (20)



#define MAX_CP_OPTIONS (20)
#define OPT_PAYLOAD_SIZE (20)



#define RC_SEL_CONF (0)
#define RC_SEL_TERM (1)

#define OPT_PAYLOAD_BUF_SIZE (100)

#define CP_OPT_MAX (20)


/* Control Protocol struct*/
typedef struct ppp_cp_t{
	gnrc_nettype_t prot;
	uint8_t state;

	uint8_t restart_counter;
	uint8_t counter_failure;
	uint32_t restart_timer;

	struct ppp_dev_t *dev;
	xtimer_t xtimer;

	/* For Configure Request */
	uint8_t cr_sent_identifier;

	uint8_t cr_sent_opts[OPT_PAYLOAD_BUF_SIZE];
	uint16_t cr_sent_size;

	/* For terminate request */
	uint8_t tr_sent_identifier;

	msg_t msg;

	int (*get_opt_status)(ppp_option_t *opt);
	int (*handle_pkt)(struct ppp_cp_t *cp, gnrc_pktsnip_t *pkt);
} ppp_cp_t;

/* Implementation of LCP fsm actions */
void tlu(ppp_cp_t *lcp, void *args);
void tld(ppp_cp_t *lcp, void *args);
void tls(ppp_cp_t *lcp, void *args);
void tlf(ppp_cp_t *lcp, void *args);
void irc(ppp_cp_t *lcp, void *args);
void zrc(ppp_cp_t *lcp, void *args);
void scr(ppp_cp_t *lcp, void *args);
void sca(ppp_cp_t *lcp, void *args);
void scn(ppp_cp_t *lcp, void *args);
void str(ppp_cp_t *lcp, void *args);
void sta(ppp_cp_t *lcp, void *args);
void scj(ppp_cp_t *lcp, void *args);
void ser(ppp_cp_t *lcp, void *args);

int cp_init(struct ppp_dev_t *ppp_dev, ppp_cp_t *cp);
int trigger_event(ppp_cp_t *cp, uint8_t event, gnrc_pktsnip_t *pkt);
int handle_rcr(ppp_cp_t *cp, gnrc_pktsnip_t *pkt);
int handle_rca(ppp_cp_t *cp, gnrc_pktsnip_t *pkt);
int handle_rcn_nak(ppp_cp_t *cp, gnrc_pktsnip_t *pkt);
int handle_rcn_rej(ppp_cp_t *cp, gnrc_pktsnip_t *pkt);
int handle_coderej(gnrc_pktsnip_t *pkt);
int handle_term_ack(ppp_cp_t *cp, gnrc_pktsnip_t *pkt);

#ifdef __cplusplus
}
#endif
#endif /* GNRC_PPP_CP_H_ */
