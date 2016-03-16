
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

#define CP_OPT_MAX (20)


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

typedef struct opt_metadada_t
{
	cp_opt_t *opt;
	uint8_t status;
	cp_opt_t *next;
} opt_metadata_t

typedef struct cp_pkt_metadata_t
{
	cp_pkt_t *pkt; /* Pointer to received packet */
	uint8_t opts_status_flag; /* In case of CP options*/
	opt_metadata_t tagged_opts[CPOPT_MAX_OPT];
	cp_pkt_t sent_ack;

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
