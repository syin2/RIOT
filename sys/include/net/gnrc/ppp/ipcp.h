#ifndef PPP_IPCP_H_
#define PPP_IPCP_H_

#ifdef __cplusplus
extern "C" {
#endif

#define IPCP_RESTART_TIMER (3000000U)
#define ID_IPCP (2)

typedef enum{
	IPCP_IPADDRESS,
	IPCP_NUMOPTS
} ipcp_options_t;

struct gnrc_pppdev_t;
struct ppp_cp_t;

int ipcp_init(struct gnrc_pppdev_t *ppp_dev, struct ppp_cp_t *ipcp);

#ifdef __cplusplus
}
#endif

#endif /* PPP_IPCP_H_ */
/** @} */
