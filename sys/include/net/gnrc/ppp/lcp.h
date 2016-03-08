#ifndef PPP_LCP_H_
#define PPP_LCP_H_

#include <inttypes.h>

#include "net/gnrc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LCP_OPT_MRU (1)
#define LCP_OPT_AUTH (2)
#define LCP_OPT_QUALITY (4)
#define LCP_OPT_MAGIC (8)
#define LCP_OPT_PFC (16)
#define LCP_OPT_ACFC (32)

typedef struct{
	uint16_t identifier;
	uint8_t opts;
	uint16_t mru;
	uint16_t auth;
	uint16_t *auth_data;
	uint16_t quality;
	uint32_t magic;
	int b_pfc;
	int b_acfc;

} lcp_conf_t;

typedef enum {
	LCPSTATE_DEAD,
	LCPSTATE_ESTABLISHED,
	LCPSTATE_AUTH,
	LCPSTATE_NCP,
	LCPSTATE_OPEN,
	LCPSTATE_TERMINATION
} lcp_state_t; 

static int lcp_recv(ppp_dev_t *dev, gnrc_pktsnip_t *pkt);

#ifdef __cplusplus
}
#endif

#endif /* PPP_LCP_H_ */
/** @} */
