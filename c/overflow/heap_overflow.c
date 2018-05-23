
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char **argv)
{
	char *buf;
	char *cmd;

	buf = (char *)malloc(10);
	cmd = (char *)malloc(32);

	printf("buf = %p\n", buf);
	printf("cmd = %p\n", cmd);
	printf("buf - cmd = %ld\n", cmd - buf);

	printf("input a string: ");
	gets(buf);
	printf("got : %s", buf);
	system(cmd);
	return 0;
}
