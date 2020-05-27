#include <stdio.h>
#include <mcheck.h>
#include <stdlib.h>
#include <string.h>


char a[10] = {0};
char b[12] = {0};

int main(int argc, char *argv[])
{
	char *p = NULL;

	mtrace();
	a[2] = b[14];
	p = (char *)malloc(100);
	if (p) {
		strcpy(p, "hello, world\n");
		printf("%s", p);
	}

	p = (char *)malloc(100);
	strcpy(p, "hello, world\n");
	printf("%s", p);

	return 0;
}
