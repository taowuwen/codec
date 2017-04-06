
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


int ems_bin2str(const char *s, int len, char *d, int d_l)
{
	char *p = d;
	int   ret = 0;
	int   l = 0;
	char  ch;

	l = len;

	while (len > 0 && d_l > 0) {
		ch = *s++;

		printf("ch=%02x\n", (ch & 0xff));

		ret = snprintf(p, d_l, "%02x", (ch & 0xff));

		len--;

		p   += ret;
		d_l -= ret;
	}

	return (l - len);
}



int main()
{
	char mymac[] = {0x00, 0x0a, 0xf5, 0x9d, 0x18, 0x5c};
	char buf[12] = {0};

	ems_bin2str(mymac, 6, buf, 12);

	printf("buf: %s\n", buf);
	return 0;
}
