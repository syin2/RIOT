#ifndef PPP_IPCP_H_
#define PPP_IPCP_H_

#ifdef __cplusplus
extern "C" {
#endif

#define IPCP_RESTART_TIMER (3000000U)

typedef enum{
	IPCP_NUMOPTS
} ipcp_options_t;

struct ppp_dev_t;
struct ppp_cp_t;

int ipcp_init(struct ppp_dev_t *ppp_dev, struct ppp_cp_t *ipcp);

#ifdef __cplusplus
}
#endif

#endif /* PPP_IPCP_H_ */
/** @} */
