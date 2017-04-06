
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


unsigned long long gettotal(char *cmd)
{
	char buf[1024];
	unsigned long long total;
	FILE  *fp;

	fp = popen(cmd, "r");
	if (!fp)
		return 0;

	total = 0;
	while (fgets(buf, 1024, fp))
		total += strtol(buf, NULL, 10);

	pclose(fp);

	return total;
}

int main()
{
	unsigned long long total, used;

	total = gettotal("grep -E \"MemTotal|SwapTotal\" /proc/meminfo | awk '{print $2}'");

	used  = total - 
		gettotal("grep -E \"MemFree|Buffers|Cached|SwapCached|SwapFree\" /proc/meminfo | awk '{print $2}'");

	printf("mem usage: %f  (%lld/%lld)\n", (double)used * 100 / total,  used, total);

	return 0;
}



