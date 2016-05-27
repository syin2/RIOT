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
	if(argc < 3) {
		printf("usage: %s <if> [apn|tunnel|status|dial_up]\n", argv[0]);
		return 1;
	}

	if(strcmp(argv[1],"dial_up") == 0) 
	{
		if (argc < 3)
		{
			printf("usage: %s dial_up <if>\n", argv[0]);
			return 1;
		}
		msg_t msg;
		printf("Dialing up PPP\n");
		gnrc_ppp_dial_up(&msg, atoi(argv[2]));
	}
	else if(strcmp(argv[1],"apn") == 0)
	{
		if(argc < 4)
		{
			printf("usage: %s apn <if> <apn_address> [<apn_username> [<apn_pass>]]", argv[0]);
			return 1;
		}
		gnrc_netapi_set(atoi(argv[2]), NETOPT_APN_NAME, 0, argv[3], strlen(argv[3]));
		printf("setting apn to %s\n", argv[3]);
		if(argc > 4)
		{
			gnrc_netapi_set(atoi(argv[2]), NETOPT_APN_USER, 0, argv[4], strlen(argv[4]));;
			printf("setting apn username to %s\n", argv[4]);
		}
		if(argc > 5)
		{
			gnrc_netapi_set(atoi(argv[2]), NETOPT_APN_PASS, 0, argv[5], strlen(argv[5]));;
			printf("setting apn password to %s\n", argv[5]);
		}
	}
	else if (strcmp(argv[1], "tunnel") == 0)
	{
		if(argc < 4)
		{
			printf("usage: %s tunnel <if> <tunnel_ipv4_address> <udp port>", argv[0]);
			return 1;
		}
		ipv4_addr_t tunnel_addr;
		if(ipv4_addr_from_str(&tunnel_addr, argv[3]) == NULL)
		{
			printf("malformed tunnel IP address.\n");
			return 1;
		}
		uint32_t port = atoi(argv[4]);
		if(port > 0xFFFF || port == 0)
		{
			printf("invalid port number");
		}

		uint16_t p16 = port;
		gnrc_netapi_set(atoi(argv[2]), NETOPT_TUNNEL_IPV4_ADDRESS, 0, (void*) &tunnel_addr , sizeof(ipv4_addr_t));
		gnrc_netapi_set(atoi(argv[2]), NETOPT_TUNNEL_UDP_PORT, 0, (void*) &p16 , sizeof(uint16_t));
		printf("Setting IP address to %s and UDP port to %s\n", argv[3], argv[4]);
	}
	else
	{
		if(argc < 3) {
			printf("usage: %s  [apn|tunnel|status|dial_up]\n", argv[0]);
			return 1;
		}
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

#if ENABLE_SHELL
	char line_buf[SHELL_DEFAULT_BUFSIZE];
	shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);
#else
	while(1){

	}
#endif

    return 0;
}

