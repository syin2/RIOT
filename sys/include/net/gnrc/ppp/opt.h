#ifndef PPP_OPT_H
#define PPP_OPT_H

#include <errno.h>
#include <string.h>
#include "net/gnrc/pkt.h"
#include "net/gnrc/pktbuf.h"

#if ENABLE_DEBUG
/* For PRIu16 etc. */
#include <inttypes.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t ppp_option_t;

/*Control Protocol option*/
typedef struct __attribute__((packed)){
	uint8_t type;
	uint8_t length;
} cp_opt_hdr_t;


static inline uint8_t ppp_opt_get_type(ppp_option_t *opt)
{
	return (uint8_t) *((uint8_t *) opt);
}

static inline void ppp_opt_set_type(ppp_option_t *opt, uint8_t type)
{
	*((uint8_t *) opt) = type;
}

static inline uint8_t ppp_opt_get_length(ppp_option_t *opt)
{
	return (uint8_t) *(((uint8_t*) opt)+1);
}

static inline void ppp_opt_set_length(ppp_option_t *opt, uint8_t length)
{
	*(((uint8_t*) opt)+1) = length;
}

static inline void * ppp_opt_get_payload(ppp_option_t *opt)
{
	return (void*) (((uint8_t*) opt)+2);
}

static inline void ppp_opt_set_payload(ppp_option_t *opt, void *data, size_t size)
{
	memcpy(((uint8_t*) opt)+2,data,size);
}
static inline ppp_option_t *ppp_opt_get_next(ppp_option_t *curr_opt, ppp_option_t *head, size_t opt_size)
{
	ppp_option_t *ret = NULL;
	ret = (ppp_option_t*)(((uint8_t*) curr_opt)+ppp_opt_get_length(curr_opt));
	if(ret - head >= opt_size)
		return NULL;
	return ret;
}

static inline int ppp_conf_opts_valid(gnrc_pktsnip_t *opts_snip, uint8_t expected_length)
{
	uint8_t opts_length = expected_length;
	if (opts_length < 4)
		return -EBADMSG;

	uint16_t cursor=1;
	uint8_t *p = ((uint8_t*) opts_snip->data)+1;

	while(cursor < opts_length)
	{
		if(*((uint8_t*) p) < 2)
			return EBADMSG;
		cursor += *((uint8_t*)p);
		if(cursor-1 > opts_length)
			return -EBADMSG;
		p += *p;
	}

	if (cursor-1 != opts_length)	
		return false;

	return true;
}

static inline int ppp_opt_is_subset(ppp_option_t *opt, ppp_option_t *optset, size_t size)
{
	uint8_t opt_type = (uint8_t) *((uint8_t*) opt);
	uint8_t *curr_opt = (uint8_t*) optset;
	uint8_t cursor=0;
	while(cursor<size)
	{
		if(opt_type == *curr_opt)
			return true;
		cursor+=ppp_opt_get_length(curr_opt);
		curr_opt += (int) ppp_opt_get_length(curr_opt);
	}
	return false;
}

static inline int ppp_opt_fill(void *opt_buf, uint8_t type, void *payload, size_t pay_size)
{
	ppp_option_t *opt = opt_buf;
	ppp_opt_set_type(opt, type);
	ppp_opt_set_length(opt, 2+pay_size);
	ppp_opt_set_payload(opt, payload, pay_size);
	return 2+pay_size;
}

#ifdef __cplusplus
}
#endif
#endif
