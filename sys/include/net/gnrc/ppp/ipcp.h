#ifndef PPP_IPCP_H_
#define PPP_IPCP_H_

#include "net/gnrc/ppp/fsm.h"
#include "net/ipv4/addr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IPCP_RESTART_TIMER (3000000U)

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

typedef struct ppp_ipv4_t
{
	ppp_protocol_t prot;
	ipcp_t *ipcp;
	pppdev_t *pppdev;
} ppp_ipv4_t;

struct gnrc_pppdev_t;
struct ppp_fsm_t;

int ipcp_init(struct gnrc_pppdev_t *ppp_dev, struct ppp_fsm_t *ipcp);
int ppp_ipv4_init(gnrc_pppdev_t *ppp_dev, ppp_ipv4_t *ipv4, ipcp_t *ipcp, pppdev_t *pppdev);
int ppp_ipv4_handler(ppp_protocol_t *prot, uint8_t event, gnrc_pktsnip_t *pkt);

#ifdef __cplusplus
}
#endif

#endif /* PPP_IPCP_H_ */
/** @} */
