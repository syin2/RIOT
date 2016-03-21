#ifndef PPP_OPT_H
#define PPP_OPT_H

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
	cp_opt_hdr_t *opt;
	uint8_t status;
} opt_metadata_t;

#ifdef __cplusplus
}
#endif
#endif
