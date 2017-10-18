#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
	int len;
	char *ptr = "hello, world";

	len = strlen(ptr);

	printf("===> %.*s\n",len - 1, ptr);
	printf("===> %s\n", ptr);

	return 0;
}
