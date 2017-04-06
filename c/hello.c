#include <stdio.h>
#include <mcheck.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char *argv[])
{
	char *p = NULL;

	mtrace();
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
