#ifndef PPP_LCP_H_
#define PPP_LCP_H_

#ifdef __cplusplus
extern "C" {
#endif

#define LCP_OPT_MRU (1)
#define LCP_OPT_ACCM (2)

#define LCP_MAX_MRU (2000)
#define LCP_DEFAULT_MRU (1500)

#define LCP_MRU_EN (1)
#define LCP_RESTART_TIMER (3000000U)

#define ID_LCP (1)


typedef enum{
	LCP_MRU,
	LCP_ACCM,
	LCP_NUMOPTS
} lcp_options_t;

struct gnrc_pppdev_t;
struct ppp_fsm_t;

int lcp_init(struct gnrc_pppdev_t *ppp_dev, struct ppp_fsm_t *lcp);

#ifdef __cplusplus
}
#endif

#endif /* PPP_LCP_H_ */
/** @} */
