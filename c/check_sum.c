
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECKSUM_FORMAT "XXXXXXXXXXXXXXXXXXXX"
#define LEN_CHECKSUM strlen(CHECKSUM_FORMAT)

#define YES	1
#define NO	0

char crc8(char *src, int len)
{
	char ch;

	ch = 0;

	while (*src && len-- > 0) {
		ch ^= *src++;
	}

	return ch;
}


int valid_ac(char *src)
{
	char ch;
	if (!src)
		return NO;

	while (*src) {
		ch = *src++;
		if ( (ch >= '0' && ch <= '9') ||
		     (ch >= 'A' && ch <= 'Z')) {
			continue;
		} else
			return NO;
	}

	/*
	 * should check last 4 bytes
	 * [0-9A-F]
	 * */

	return YES;
}


int gen_check_sum(char *src, char *dst)
{
	char ch[2];
	int ret;

	if (!valid_ac(src))
		return NO;

	ch[0] = crc8(src, 8);
	ch[1] = crc8(src+8, 8);

	sprintf(dst, "%.2X%.2X", ch[0], ch[1]);

	return 0;
}



int valid_check_sum(char *src)
{
	char buf[32], ch;
	char cs[5] = {0};

	if (!src || strlen(src) != LEN_CHECKSUM) {
		return NO;
	}

	snprintf(buf, 32, "%s", src);

	if (!valid_ac(src))
		return NO;

	gen_check_sum(src, &buf[16]);

	if (strcmp(src, buf)) {
		printf("check sum : %s is not valid: should be: %s\n", src, buf);
		return NO;
	}

	return YES;
}

static void test()
{
	int len = 20;
	int total = 16;

	printf("~(total -1) = 0x%x\n", ~(total -1));
	printf(" (len + total -1) & ~(total -1) = 0x%x\n", (len + total -1) & ~(total -1));


	{
		char buf[256] = {0};
		long i, k, j;

		snprintf(buf, sizeof(buf), "1234567890;987654321;12341232");

		sscanf(buf, "%ld;%ld;%ld", &i, &k, &j);
		printf("i=%ld, k=%ld, j=%ld\n", i, k, j);
	}

}


int main(int argc, char *argv[])
{
	char buf[1024];
	char *s, *e;

	char chsum[5][8];
	int i;

	test();

	if (argc < 3) {
		printf("usage: %s check/gen active_code\n", argv[0]);
		return 0;
	}

	if (strcmp(argv[1], "check") == 0) {
		if (!valid_check_sum(argv[2])) {
			printf("checksum : %s invalid\n",argv[2]);
		} else {
			printf("checksum: %s---- VALID\n", argv[2]);
		}
	} else {
		snprintf(buf, 1024, "%s", argv[2]);
		gen_check_sum(argv[2], &buf[16]);

		printf("checksum: %s\n", buf);
	}

	return 0;
}
