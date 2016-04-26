#ifndef PPP_IPCP_H_
#define PPP_IPCP_H_

#include "net/gnrc/ppp/fsm.h"
#include "net/ipv4/addr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IPCP_RESTART_TIMER (3000000U)
#define ID_IPCP (2)

typedef enum{
	IPCP_IPADDRESS,
	IPCP_NUMOPTS
} ipcp_options_t;

typedef struct ipcp_t
{
	ppp_fsm_t fsm;
	ipv4_addr_t local_ip;
	ipv4_addr_t ip;
	cp_conf_t ipcp_opts[IPCP_NUMOPTS];
} ipcp_t;

struct gnrc_pppdev_t;
struct ppp_fsm_t;

int ipcp_init(struct gnrc_pppdev_t *ppp_dev, struct ppp_fsm_t *ipcp);

#ifdef __cplusplus
}
#endif

#endif /* PPP_IPCP_H_ */
/** @} */
