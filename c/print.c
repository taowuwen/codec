#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
	int len;
	char ptr_str[] = "hello, world";

	printf("strlen(ptr_str): %d\n", strlen(ptr_str));

	printf("===> %.*s\n",sizeof(ptr_str) - 2, ptr_str);
	printf("===> %s\n", ptr_str);

	return 0;
}
