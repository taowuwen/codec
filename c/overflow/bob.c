#include <string.h>
#include <stdio.h>

int test(char *src)
{
	char buffer[24];
	strcpy(buffer, src);
	return 1;
}

int main(int argc, char *argv[]) {
	printf("Hello, buffer overflow by taowuwen@gmail.com 3170703004!\n");
	test(argv[1]);
	return 1;
}
