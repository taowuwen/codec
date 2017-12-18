#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char **argv)
{
	int n = 0;
	int mod_3 = 0;
	int mod_5 = 0;

	printf("\n");
	for (n = 1; n <= 100; n++) {
		mod_3 = n % 3;
		mod_5 = n % 5;

		if (mod_3 == 0 || mod_5 == 0) {
			if (mod_3 == 0 && mod_5 == 0) {
				printf(" Fizz-Buzz");
			} else if (mod_3 == 0) {
				printf(" Fizz");
			} else {
				printf(" Buzz");
			}
		} else {
			printf(" %d", n);
		}
	}

	printf("\n");

	return 0;
}
