#include <stdio.h>
#include <stdlib.h>
#include <math.h>


int main(int argc, char **argv) 
{
	int i, n;
	int number = 0;
	double s, sum = 0;

	printf("Please input a number for n: \n");
	scanf("%d", &n);
	printf("The number is n=%d\n", n);

	for (i = 21; i < n; i+=21) {
		if (i % 21 == 0) {
			sum += (double)i;
			number++;
		}

		/*
		if (i%3 == 0 && i % 7 == 0) {
		}
		*/
	}

	printf("sum = %lf, sqrt(%lf) = %f\n", sum, sqrt(sum)); 
	printf("Number = %d\n", number);
	return 0;
}
