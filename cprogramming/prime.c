#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void print_prime(int n)
{
	int half = 0, i;

	half = n / 2;

	for (i = 2; i < half; i++) {
		if (n % i == 0)
			return;
	}

	printf("%d ", n);
}


int main(int argc, char **argv)
{
	int i;

	for (i = 10; i < 100; i++) {
		print_prime(i);
	}

	printf("\n");

	return 0;
}

