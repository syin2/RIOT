#include <stdint.h>

#include <platform/random.h>
#include "random.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

uint32_t otPlatRandomGet(void)
{
	return random_uint32();
}
