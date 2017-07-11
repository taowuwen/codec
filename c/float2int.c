#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
	int   i = 10;
	float f;

	memcpy((void *)&f, (void *)&i, sizeof(i));
	{
		char *ch = (char *)&f;

		ch += (sizeof(float) - 1);

		*ch |= 0xc0;
	}

	printf("%f\n", f);

	f = (float)(25 / 10);

	printf("%f\n", f);
	printf("%lld\n", f);

	return 0;
}

