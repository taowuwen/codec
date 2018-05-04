#include <stdio.h>
#include <stdlib.h>
#include <math.h>


int main(int argc, char **argv)
{
	unsigned n = 0;
	unsigned array[64] = {0};
	unsigned num;
	unsigned sys;

	printf("input a number: ");
	scanf("%u", &num);

	printf("input the system that you want to convert to: ");
	scanf("%u", &sys);

	if (sys <= 1 || sys >= 32) {
		printf("we need system between 1 to 31\n");
		return -1;
	}

	while (num > 0) {
		array[n++] = num % sys;
		num = num / sys;
	}

	while (n-- > 0) {
		printf("%u", array[n]);
	}
	printf("\n");

	return 0;
}
