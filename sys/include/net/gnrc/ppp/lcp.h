#ifndef PPP_LCP_H_
#define PPP_LCP_H_

#include <inttypes.h>

#include "net/gnrc.h"
#include "net/gnrc/ppp/cp.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LCP_OPT_MRU (1)
#define LCP_OPT_AUTH (2)
#define LCP_OPT_QUALITY (4)
#define LCP_OPT_MAGIC (8)
#define LCP_OPT_PFC (16)
#define LCP_OPT_ACFC (32)

#define LCP_MAX_MRU (2000)
#define LCP_DEFAULT_MRU (1500)

typedef struct lcp_opt_t{
	uint16_t flags;
	uint16_t mru;
	uint16_t auth;
	uint16_t *auth_data;
	uint16_t quality;
	uint32_t magic;
	int b_pfc;
	int b_acfc;
} lcp_opt_t;

void lcp_negotiate_nak(void *lcp_opt, opt_metadata_t *recv_opts, uint8_t recv_num);


#ifdef __cplusplus
}
#endif

#endif /* PPP_LCP_H_ */
/** @} */
