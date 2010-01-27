
/*
#include <sys/types.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <signal.h>
#include <unistd.h>
*/

#include <stdio.h>

#include <sys/resource.h>

long Socket_SetDescriptorLimitToMax(void)
{
	struct rlimit rlp;

	// if were root, we could also do:
	// system("sysctl -w kern.maxfiles=1000001");
	// system("sysctl -w kern.maxfilesperproc=1000000");
	
	/*
	int optval = 1;
     int
     setsockopt(int socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	SO_REUSEADDR
	*/

	if (getrlimit(RLIMIT_NOFILE, &rlp) != 0)
	{
		return -1;
	}

	printf("rlp.rlim_cur = %i\n", (int)rlp.rlim_cur);
	printf("rlp.rlim_max = %i\n", (int)rlp.rlim_max);

	rlp.rlim_cur = rlp.rlim_max;

	if (setrlimit(RLIMIT_NOFILE, &rlp) != 0)
	{
		return -2;
	}

	if (getrlimit(RLIMIT_NOFILE, &rlp) != 0)
	{
		return -3;
	}

	printf(" max listeners = %i\n", (int)rlp.rlim_cur);

	return (long)(rlp.rlim_cur);
}
