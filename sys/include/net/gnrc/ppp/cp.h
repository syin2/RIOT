
#ifndef GNRC_PPP_CP_H_
#define GNRC_PPP_CP_H_
#include "net/ppp/pkt.h"
#include "net/ppp/opt.h"

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



#define MAX_CP_OPTIONS (20)
#define OPT_PAYLOAD_SIZE (20)



#define RC_SEL_CONF (0)
#define RC_SEL_TERM (1)

#define OPT_PAYLOAD_BUF_SIZE (100)

#define CP_OPT_MAX (20)


typedef struct cp_pkt_metadata_t
{
	cp_pkt_t *pkt; /* Pointer to received packet */
	uint8_t opts_status_content; /* In case of CP options*/
	uint8_t tagged_opts[CPOPT_MAX_OPT];
	opt_metadata_t opts;
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

	uint8_t cr_sent_opts[OPT_PAYLOAD_BUF_SIZE];
	uint16_t cr_sent_size;

	/* For terminate request */
	uint8_t tr_identifier;

	cp_pkt_metadata_t metadata; 


	/* Pointer to another struct with CP options*/
	void *cp_options;
	void handle_conf();
	void handle_code();
	/*Add sm handlers here*)
	uint16_t enabled_options:
	/* Function for converting cp_opts to payload  */
	uint32_t (*_load_specific_cp_opts)(void *cp_options, uint8_t *dst);
	/* Negotiate nak */
	void (*negotiate_nak)(void *cp_options, cp_pkt_metadata_t *metadata);
	int (*get_option_status)(void *opt);



} ppp_cp_t;

void handle_cp_pkt(ppp_cp_t *cp, cp_pkt_t *pkt);
/* Function for option tagging */
/* Init metadata, tag options if necessary */
void ppp_pkt_gen_metadata(cp_pkt_metadata_t *metadata, cp_pkt_t *pkt, int (*get_opt_status)(void*));
/* Tag each options with corresponding status, add info to metadata*/
void _ppp_pkt_metadata_tag_cr_opts(cp_pkt_metadata_t);
int ppp_cr_opts_are_equal(cp_opt_hdr_t *o1, cp_opt_hdr_t *o2);

#ifdef __cplusplus
}
#endif
#endif /* GNRC_PPP_CP_H_ */
