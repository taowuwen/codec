#include <stdio.h>
#include <stdlib.h>


int main(int argc, char **argv)
{
	int i, j;

	i = ~0;

	printf("%d, %u, 0x%x\n", i, i, i);

	i <<= 4;

	printf("%d, %u, 0x%x\n", i, i, i);

	i = ~i;

	printf("%d, %u, 0x%x\n", i, i, i);

	return 0;
}
