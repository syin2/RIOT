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

#define LCP_MRU_EN (1)

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


/* Implementation of LCP fsm actions */
static void lcp_tlu(ppp_cp_t *lcp);
static void lcp_tld(ppp_cp_t *lcp);
static void lcp_tls(ppp_cp_t *lcp);
static void lcp_tlf(ppp_cp_t *lcp);
static void lcp_irc(ppp_cp_t *lcp);
static void lcp_zrc(ppp_cp_t *lcp);
static void lcp_scr(ppp_cp_t *lcp);
static void lcp_sca(ppp_cp_t *lcp);
static void lcp_scn(ppp_cp_t *lcp);
static void lcp_str(ppp_cp_t *lcp);
static void lcp_sta(ppp_cp_t *lcp);
static void lcp_scj(ppp_cp_t *lcp);
static void lcp_ser(ppp_cp_t *lcp);

/*Event generators*/
void lcp_handle_conf(cp_ppp_t *lcp, cp_pkt_t *pkt);
void lcp_handle_code(cp_ppp_t *lcp, cp_pkt_t *pkt);

#ifdef __cplusplus
}
#endif

#endif /* PPP_LCP_H_ */
/** @} */
