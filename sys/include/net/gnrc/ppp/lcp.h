#ifndef PPP_LCP_H_
#define PPP_LCP_H_

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
#define LCP_RESTART_TIMER (3000000U)

#define ID_LCP (1)


typedef enum{
	LCP_MRU,
	LCP_NUMOPTS
} lcp_options_t;

struct ppp_dev_t;
struct ppp_cp_t;

int lcp_init(struct ppp_dev_t *ppp_dev, struct ppp_cp_t *lcp);

#ifdef __cplusplus
}
#endif

#endif /* PPP_LCP_H_ */
/** @} */
