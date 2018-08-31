#include <stdio.h>
#include <stdlib.h>


int main(int argc, char **argv)
{
	char a[] = "Virtual C++";
	char *b  = "Virtual C++";

	printf("%d, %d\n", sizeof(a), sizeof(b));
	printf("%d, %d\n", sizeof(*a), sizeof(*b));
	return 0;
}
