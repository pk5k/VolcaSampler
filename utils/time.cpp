#include <stdlib.h>
#include <sys/time.h>
#include <stdint.h> 
#include "time.h"

uint64_t system_current_time_millis()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	return (uint64_t)(tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ; // convert tv_sec & tv_usec to millisecond
}