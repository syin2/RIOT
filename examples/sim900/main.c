#include "sim900.h"
#define ENABLE_DEBUG (1)
#include "debug.h"

#include "shell.h"
#include "shell_commands.h"

#define ENABLE_SHELL (1)

int main(void)
{
	DEBUG("Here we go!\n");
	kernel_pid_t netifs[GNRC_NETIF_NUMOF];

	DEBUG("Number of interfaces: %i\n", gnrc_netif_get(netifs));

	char apn[] ="mmsbouygtel.com";
	gnrc_netapi_set(netifs[1], NETOPT_APN_NAME, 0, apn, sizeof(apn)-1);

	msg_t msg;
	gnrc_ppp_dial_up(&msg, netifs[1]);

#if ENABLE_SHELL
	char line_buf[SHELL_DEFAULT_BUFSIZE];
	shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);
#else
	while(1){}
#endif

    return 0;
}

