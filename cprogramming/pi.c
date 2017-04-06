#include <stdio.h>
#include <stdlib.h>


int main(int argc, char *argv[])
{
	long n, i, punc;
	float pi;

	n = 10000000000;
	punc = -1;
	for (i = 1; i < n; i += 2) {
		punc *= -1;

//		printf("%s1/%d", punc > 0?"+":"-", i); 

		pi += punc * 1.0/i;
	}

	printf("\n pi = %.12f\n", pi);

	pi *= 4.0;

	printf("pi == %.12f\n", pi);

	return 0;
}
