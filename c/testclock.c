
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[])
{

	struct timespec tm;

	clock_gettime(CLOCK_BOOTTIME, &tm);

	printf("current boot time: %ld: %ld\n", tm.tv_sec, tm.tv_nsec);

	printf("boot time: %ldd, %ldh, %ldm, %lds\n", 
			tm.tv_sec/(24 * 3600),
			(tm.tv_sec%(24 * 3600))/3600,
			(tm.tv_sec % 3600) / 60,
			(tm.tv_sec % 60));

	return 0;
}
