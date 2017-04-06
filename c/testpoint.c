#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char *argv[])
{

	int b = 11;
	int a = 10;

	int *pa = &a;

	printf("%p = %d, a = %d\n", pa, *pa, a);

	*pa = b;
	printf("%p = %d, a = %d\n", pa, *pa, a);

	return 0;
}
