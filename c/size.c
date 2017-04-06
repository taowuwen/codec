#include <stdio.h>
#include <stdlib.h>


int main(int argc, char **argv)
{
	printf("sizeof(long double) = %ld\n", sizeof(long double));
	printf("sizeof(double) = %ld\n", sizeof(double));
	printf("sizeof(long) = %ld\n", sizeof(long));

	printf("sizeof(long long) = %ld\n", sizeof(unsigned long long));

	return 0;
}

