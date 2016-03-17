
#ifndef PPP_PKT_H_
#define PPP_PKT_H_

#include <inttypes.h>

#include "net/gnrc.h"
#include "net/gnrc/netdev2.h"
#include "byteorder.h"

#ifdef __cplusplus
extern "C" {
#endif
#define PPP_PAYLOAD_SIZE (200)
#define CPOPT_MAX_OPT (25)

#define OPT_HAS_ACK (1)
#define OPT_HAS_NAK (2)
#define OPT_HAS_REJ (4)

#define CP_CREQ_ACK (0)
#define CP_CREQ_NAK (1)
#define CP_CREQ_REJ (2)

#define PPP_CONF_REQ (1)
#define PPP_CONF_ACK (2)
#define PPP_CONF_NAK (3)
#define PPP_CONF_REJ (4)
#define PPP_CP_TERM_REQUEST (5)
#define PPP_CP_TERM_ACK (6)
#define PPP_CP_SER (7)

/*  PPP pkt header struct */
typedef struct __attribute__((packed)){
	uint8_t code;
	uint8_t id;
	network_uint16_t length;
} cp_hdr_t;

/*Control Protocol option*/
typedef struct __attribute__((packed)){
	uint8_t type;
	uint8_t length;
	uint8_t *payload;
} cp_opt_hdr_t;

typedef struct opt_metadada_t
{
	cp_opt_hdr_t *opt;
	uint8_t status;
} opt_metadata_t;

/* A PPP packet*/
typedef struct __attribute__((packed))
{
	cp_hdr_t *hdr;
	uint8_t *payload;
} cp_pkt_t;


typedef struct cp_pkt_metadata_t
{
	cp_pkt_t *pkt; /* Pointer to received packet */
	uint8_t opts_status_content; /* In case of CP options*/
	opt_metadata_t tagged_opts[CPOPT_MAX_OPT];
	uint8_t num_tagged_opts;
} cp_pkt_metadata_t;


/* Status of Control Protocol options response */
typedef struct opt_stack_t
{
	uint8_t status; /* Status of the set of CP opt response (ACK, NAK, REJ)*/
	uint8_t num_opts; /* Number of options in response */
	uint8_t content_flag;
	cp_opt_hdr_t *opts;
}opt_stack_t;


int ppp_pkt_init(uint8_t *data, size_t length, cp_pkt_t *cp_pkt);
/* Function for option tagging */
/* Init metadata, tag options if necessary */
void ppp_pkt_gen_metadata(cp_pkt_metadata_t *metadata, cp_pkt_t *pkt);
/* Tag each options with corresponding status, add info to metadata*/
void _ppp_pkt_metadata_tag_cr_opts(cp_pkt_metadata_t);

int _ppp_cr_populate_options(uint8_t *payload, size_t p_size);
int ppp_cr_opts_are_equal(cp_opt_hdr_t *o1, cp_opt_hdr_t *o2);

uint8_t ppp_pkt_get_code(cp_pkt_t *cp_pkt);
void ppp_pkt_set_code(cp_pkt_t *cp_pkt, uint8_t code);
uint8_t ppp_pkt_get_id(cp_pkt_t *cp_pkt);
void ppp_pkt_set_id(cp_pkt_t *cp_pkt, uint8_t id);
uint16_t ppp_pkt_get_length(cp_pkt_t *cp_pkt);
void ppp_pkt_set_length(cp_pkt_t *cp_pkt, uint16_t length);
int ppp_pkt_is_configure(cp_pkt_t *pkt);




#ifdef __cplusplus
}
#endif

#endif /* PPP_PKT_H_ */
/** @} */
