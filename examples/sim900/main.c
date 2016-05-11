#include "sim900.h"
#define ENABLE_DEBUG (1)
#include "debug.h"

char thread_stack[2*THREAD_STACKSIZE_MAIN];	

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

	xtimer_usleep(1000000);
	DEBUG("Here we go!\n");

#if ENABLE_SHELL
	char line_buf[SHELL_DEFAULT_BUFSIZE];
	shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);
#else
	while(1){}
#endif

    return 0;
}

