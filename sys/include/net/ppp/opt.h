#ifndef PPP_OPT_H
#define PPP_OPT_H

#include <errno.h>
#include "net/ppp/pkt.h"

#ifdef __cplusplus
extern "C" {
#endif


/*Control Protocol option*/
typedef struct __attribute__((packed)){
	uint8_t type;
	uint8_t length;
} cp_opt_hdr_t;

typedef struct opt_metadada_t
{
	int num;
	void *head;
	void *current;

	int _co;
} opt_metadata_t;

static inline int _get_num_opt(void *head_opt, uint16_t opts_length)
{
	if (opts_length == 0)
		return 0;
	if (opts_length <= 4)
		return -EBADMSG;
	int num=0;
	uint16_t cursor=1;
	uint8_t *p = ((uint8_t*) head_opt)+1;

	while(cursor < opts_length)
	{
		if(*((uint8_t*) p) < 4)
			return -EBADMSG;
		cursor += *p;
		p += *p;
		num += 1;
	}
	
	return num;
}
static inline int ppp_opts_init(opt_metadata_t *opt_metadata, cp_pkt_t *pkt)
{
	opt_metadata->head = opt_metadata->current = pkt->payload;
	opt_metadata->_co = 0;
	uint16_t pkt_length = ppp_pkt_get_length(pkt);
	int num = _get_num_opt(pkt->payload, pkt_length-sizeof(cp_hdr_t));
	if (num == -EBADMSG)
		return -1;
	
	opt_metadata->num = num;
	return 0;
}
static inline void *ppp_opts_get_head(opt_metadata_t *opt_metadata)
{
	return opt_metadata->head;
}
static inline void ppp_opts_reset(opt_metadata_t *opt_metadata)
{
	opt_metadata->current = opt_metadata->head;
}
void *ppp_opts_next(opt_metadata_t *opt_metadata)
{
	void *current = opt_metadata->current;
	uint8_t opt_size = *((uint8_t*)current+1);

	if(opt_metadata->_co < opt_metadata->num-1)
	{
		opt_metadata->current = (void*) (((uint8_t*) current)+opt_size);
		opt_metadata->_co += 1;
		return opt_metadata->current; 
	}
	return NULL;
}
static inline int ppp_opts_get_num(opt_metadata_t *opt_metadata)
{
	return opt_metadata->num;
}

static inline uint8_t ppp_opt_get_type(void *opt)
{
	return (uint8_t) *((uint8_t *) opt);
}
	
static inline uint8_t ppp_opt_get_length(void *opt)
{
	return (uint8_t) *(((uint8_t*) opt)+1);
}
static inline void * ppp_opt_get_payload(void *opt)
{
	return (void*) (((uint8_t*) opt)+2);
}

#ifdef __cplusplus
}
#endif
#endif
