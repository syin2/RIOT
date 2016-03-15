
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

/*Function flags*/
#define F_TLU (1<<0)
#define F_TLD (1<<1)
#define F_TLS (1<<2)
#define F_TLF (1<<3)
#define F_IRC (1<<4)
#define F_ZRC (1<<5)
#define F_SRC (1<<6)
#define F_SCA (1<<7)
#define F_SCN (1<<8)
#define F_STR (1<<9)
#define F_STA (1<<10)
#define F_SCJ (1<<11)
#define F_SER (1<<12)

#define PPP_MSG_UP (1)
#define PPP_MSG_DOWN (2)

#define PPP_MAX_TERMINATE (3)
#define PPP_MAX_CONFIG (3)

#define PPP_MSG_TIMEOUT (1)

#define PPP_CP_HDR_BASE_SIZE (4)

#define PPP_PAYLOAD_BUF_SIZE (256)

#define GNRC_PPP_MSG_QUEUE_SIZE (20)

#define CP_CREQ_ACK (0)
#define CP_CREQ_NAK (1)
#define CP_CREQ_REJ (2)


#define MAX_CP_OPTIONS (20)
#define OPT_PAYLOAD_SIZE (20)

#define PPP_CP_REQUEST_CONFIGURE (1)
#define PPP_CP_REQUEST_ACK (2)
#define PPP_CP_REQUEST_NAK (3)
#define PPP_CP_REQUEST_REJ (4)
#define PPP_CP_TERM_REQUEST (5)
#define PPP_CP_TERM_ACK (6)
#define PPP_CP_SER (7)

#define OPT_HAS_ACK (1)
#define OPT_HAS_NAK (2)
#define OPT_HAS_REJ (4)

#define RC_SEL_CONF (0)
#define RC_SEL_TERM (1)

#define OPT_PAYLOAD_BUF_SIZE (100)

/* Functions flags for each state */
const uint16_t actions[PPP_NUM_EVENTS][PPP_NUM_STATES] = {
{0,F_IRC | F_SRC,0,0,0,0,0,0,0,0},
{0,0,0,F_TLS,0,0,0,0,0,F_TLD},
{F_TLS,0,F_IRC | F_SRC, 0,0,0,0,0,0,0},
{0,F_TLF,0,0,0,0,F_IRC | F_STR, F_IRC | F_STR, F_IRC | F_STR, F_TLD | F_IRC | F_STR},
{0,0,0,0,F_STR,F_STR,F_SRC,F_SRC,F_SRC,0},
{0,0,0,0,F_TLF,F_TLF,F_TLF,F_TLF,F_TLF,0},
{0,0,F_STA,F_IRC | F_SRC | F_SCA,0,0,F_SCA,F_SCA | F_TLU,F_SCA,F_TLD | F_SRC | F_SCA},
{0,0,F_STA,F_IRC | F_SRC | F_SCA,0,0,F_SCN,F_SCN,F_SCN,F_TLD | F_SRC | F_SCN},
{0,0,F_STA,F_STA,0,0,F_IRC | F_SRC,F_SRC,F_IRC | F_SRC, F_TLD | F_SRC},
{0,0,F_STA,F_STA,0,0,F_IRC | F_SRC,F_SRC, F_IRC | F_SRC,F_TLD | F_SRC},
{0,0,F_STA,F_STA,F_STA,F_STA,F_STA,F_STA,F_STA,F_TLD | F_ZRC | F_STA},
{0,0,0,0,F_TLF,F_TLF,0,0,0,F_TLD | F_SRC},
{0,0,F_SCJ,F_SCJ,F_SCJ,F_SCJ,F_SCJ,F_SCJ,F_SCJ,F_SCJ},
{0,0,0,0,0,0,0,0,0,0},
{0,0,F_TLF,F_TLF,F_TLF,F_TLF,F_TLF,F_TLF,F_TLF,F_TLD | F_IRC | F_STR},
{0,0,0,0,0,0,0,0,0,F_SER}
};


/* state transition for control layer FSM */
const int8_t state_trans[PPP_NUM_EVENTS][PPP_NUM_STATES] = {
{S_CLOSED,S_REQ_SENT,S_UNDEF,S_UNDEF,S_UNDEF,S_UNDEF,S_UNDEF,S_UNDEF,S_UNDEF,S_UNDEF},
{S_UNDEF,S_UNDEF,S_INITIAL,S_STARTING,S_INITIAL,S_STARTING,S_STARTING,S_STARTING,S_STARTING,S_STARTING},
{S_STARTING,S_STARTING,S_REQ_SENT,S_STOPPED,S_STOPPING,S_STOPPING,S_REQ_SENT,S_ACK_RCVD,S_ACK_SENT,S_OPENED},
{S_INITIAL,S_INITIAL,S_CLOSED,S_CLOSED,S_CLOSING,S_CLOSING,S_CLOSING,S_CLOSING,S_CLOSING,S_CLOSING},
{S_UNDEF,S_UNDEF,S_UNDEF,S_UNDEF,S_CLOSING,S_STOPPING,S_REQ_SENT,S_REQ_SENT,S_ACK_SENT,S_UNDEF},
{S_UNDEF,S_UNDEF,S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_STOPPED,S_STOPPED,S_STOPPED,S_UNDEF},
{S_UNDEF,S_UNDEF,S_CLOSED,S_ACK_SENT,S_CLOSING,S_STOPPING,S_ACK_SENT,S_OPENED,S_ACK_SENT,S_ACK_SENT},
{S_UNDEF,S_UNDEF,S_CLOSED,S_REQ_SENT,S_CLOSING,S_STOPPING,S_REQ_SENT,S_ACK_RCVD,S_REQ_SENT,S_REQ_SENT},
{S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_CLOSING,S_STOPPING,S_ACK_RCVD,S_REQ_SENT,S_OPENED,S_REQ_SENT},
{S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_CLOSING,S_STOPPING,S_REQ_SENT,S_REQ_SENT,S_ACK_SENT,S_REQ_SENT},
{S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_CLOSING,S_STOPPING,S_REQ_SENT,S_REQ_SENT,S_REQ_SENT,S_STOPPING},
{S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_CLOSED,S_STOPPED,S_REQ_SENT,S_REQ_SENT,S_ACK_SENT,S_REQ_SENT},
{S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_CLOSING,S_STOPPING,S_REQ_SENT,S_ACK_RCVD,S_ACK_SENT,S_OPENED},
{S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_CLOSING,S_STOPPING,S_REQ_SENT,S_REQ_SENT,S_ACK_SENT,S_OPENED},
{S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_CLOSED,S_STOPPED,S_STOPPED,S_STOPPED,S_STOPPED,S_STOPPING},
{S_UNDEF,S_UNDEF,S_CLOSED,S_STOPPED,S_CLOSING,S_STOPPING,S_REQ_SENT,S_ACK_RCVD,S_ACK_SENT,S_OPENED}
};

/*Control Protocol option*/
typedef struct __attribute__((packed)){
	uint8_t type;
	uint8_t length;
	uint8_t *payload;
} cp_opt_t;

/* Status of Control Protocol options response */
typedef struct opt_stack_t
{
	uint8_t status; /* Status of the set of CP opt response (ACK, NAK, REJ)*/
	uint8_t num_opts; /* Number of options in response */
	uint8_t content_flag;
	cp_opt_t *opts;
}opt_stack_t;

typedef struct cp_pkt_metadata_t
{
	cp_pkt_t *pkt; /* Pointer to received packet */
	uint8_t opts_status_flag;
} cp_pkt_metadata_t;

/* Control Protocol struct*/
typedef struct ppp_cp_t{
	uint8_t event;
	uint8_t l_upper_msg;
	uint8_t l_lower_msg;
	uint8_t up;
	uint8_t state;

	/* Select Configure or Terminate timer */
	uint8_t timer_select;

	uint32_t restart_time;
	uint32_t restart_counter;
	uint8_t counter_term;
	uint8_t counter_config;
	uint8_t counter_failure;

	struct ppp_dev_t *dev;

	/* For Configure Request */
	uint8_t cr_sent_identifier;
	uint32_t cr_sent_size;

	uint8_t cr_recv_identifier;
	uint32_t cr_recv_size;


	/* For terminate request */
	uint8_t tr_identifier;

	cp_pkt_metadata_t metadata; 


	/* Pointer to another struct with CP options*/
	void *cp_options;
	/* Function for converting cp_opts to payload  */
	uint32_t *_load_specific_cp_opts(void *cp_options, uint8_t *dst);
	/* Negotiate nak */
	void *negotiate_nak(void *cp_options, opt_stack_t *opt_stack);
	/* Hydrate cp opt */
	int *hydrate_cp_opt(uint8_t type, uint8_t *payload, size_t size, cp_opt_t *opt_buf);

	void *populate_opt_stack(void *cp_options, opt_stack_t *opt_stack);

} ppp_cp_t;

int ppp_cp_populate_options(opt_stack *o_stack, uint8_t *payload, size_t p_size);
int ppp_cp_opts_are_equal(cp_opt_t *o1, cp_opt_t *o2);
int ppp_cp_optstacks_are_equal(opt_stack_t *s1, opt_stack_t *s2);
