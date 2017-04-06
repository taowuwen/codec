#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int my_pow(int s, int n)
{
	int i, n1, t;

	if (s == 0)
		return 0;

	n1 = abs(n);

	if (n == 0)
		return 1;

	t = s;
	for (i = 1; i < n1; i++)
		t = t * s;

	/* we are not handle for n < -1*/
	return t;
}


void print_daffodil(int n)
{
	int hundred;
	int ten;
	int one;
	int daffodil = 0;

	hundred = n / 100;
	ten     = (n - hundred * 100) / 10;
	one     = n % 10;

	daffodil = my_pow(hundred, 3) + my_pow(ten , 3) + my_pow(one, 3);

	if (daffodil == n)
		printf("%d ", n);
}


int main(int argc, char **argv)
{
	int i;

	for (i = 100; i < 1000; i++) {
		print_daffodil(i);
	}

	return 0;
}

