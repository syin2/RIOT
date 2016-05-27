#include "sim900.h"
#define ENABLE_DEBUG (1)
#include "debug.h"

#include "shell.h"
#include "shell_commands.h"

#include "msg.h"
#include "net/ipv4/hdr.h"

#define ENABLE_SHELL (1)
#define MAIN_QUEUE_SIZE     (8)

static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];


int ppp_cmd(int argc, char **argv)
{
	if(argc < 2) {
		printf("usage: %s [set|get|status|dial_up]\n", argv[0]);
		return 1;
	}

	if(strcmp(argv[1],"dial_up") == 0) 
	{
		if (argc < 3)
		{
			printf("usage: %s, dial_up <if>\n", argv[0]);
			return 1;
		}
		msg_t msg;
		printf("Dialing up PPP\n");
		gnrc_ppp_dial_up(&msg, atoi(argv[2]));
	}
	return 0;
}

static const shell_command_t shell_commands[] = {
    { "ppp", "gnrc_ppp CLI.", ppp_cmd},
    { NULL, NULL, NULL }
};
int main(void)
{
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
	kernel_pid_t netifs[GNRC_NETIF_NUMOF];

	DEBUG("Number of interfaces: %i\n", gnrc_netif_get(netifs));

	char apn[] ="mmsbouygtel.com";
	ipv4_addr_t tunnel_addr;
	uint16_t port = 9876;
	tunnel_addr.u8[0] = 51;
	tunnel_addr.u8[1] = 254;
	tunnel_addr.u8[2] = 204;
	tunnel_addr.u8[3] = 66;

	char apn_user[] = "test";
	char apn_pass[] = "test";
	gnrc_netapi_set(netifs[1], NETOPT_APN_NAME, 0, apn, sizeof(apn)-1);
	gnrc_netapi_set(netifs[1], NETOPT_TUNNEL_IPV4_ADDRESS, 0, (void*) &tunnel_addr , sizeof(ipv4_addr_t));
	gnrc_netapi_set(netifs[1], NETOPT_TUNNEL_UDP_PORT, 0, (void*) &port , sizeof(uint16_t));
	gnrc_netapi_set(netifs[1], NETOPT_APN_USER, 0, apn_user, sizeof(apn_user)-1);
	gnrc_netapi_set(netifs[1], NETOPT_APN_PASS, 0, apn_pass, sizeof(apn_pass)-1);

#if ENABLE_SHELL
	char line_buf[SHELL_DEFAULT_BUFSIZE];
	shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);
#else
	while(1){

	}
#endif

    return 0;
}

