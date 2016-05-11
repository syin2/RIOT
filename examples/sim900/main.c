#include "sim900.h"
#define ENABLE_DEBUG (1)
#include "debug.h"

#include "shell.h"
#include "shell_commands.h"

#define ENABLE_SHELL (1)

int main(void)
{
    /*sim900_t dev;

	sim900_params_t params;
	params.uart = 1;
	sim900_setup(&dev, &params);
	*/

	//gnrc_pppdev_t pppdev;
	//gnrc_ppp_setup(&pppdev, (pppdev_t*) &dev);

	//kernel_pid_t pid = gnrc_pppdev_init(thread_stack, sizeof(thread_stack) ,THREAD_PRIORITY_MAIN-1, "gnrc_sim900", &pppdev);

	//(void) pid;

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

	msg_send_receive(&msg, &reply, netifs[0]);

	gnrc_ppp_dial_up(&msg, netifs[0]);

#if ENABLE_SHELL
	char line_buf[SHELL_DEFAULT_BUFSIZE];
	shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);
#else
	while(1){}
#endif

    return 0;
}

