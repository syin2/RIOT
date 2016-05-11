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
	gnrc_netapi_opt_t apn;
	apn.opt = NETOPT_APN_NAME;
	apn.data = "mmsbouygtel.com";
	apn.data_len = sizeof("mmsbouygtel.com")-1;
	msg_t msg, reply;
	msg.type = GNRC_NETAPI_MSG_TYPE_SET;
	msg.content.ptr = (void*) &apn;

	msg_send_receive(&msg, &reply, netifs[1]);

	gnrc_ppp_dial_up(&msg, netifs[1]);

#if ENABLE_SHELL
	char line_buf[SHELL_DEFAULT_BUFSIZE];
	shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);
#else
	while(1){}
#endif

    return 0;
}

