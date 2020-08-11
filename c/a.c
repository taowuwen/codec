#include <stdio.h>
#include <stdlib.h>
#include <math.h>


int main(int argc, char **argv) 
{

	{
		char a[] = "xyz";
		char b[] = {'x', 'y', 'z'};
		printf("sizeof(a) = %d, sizeof(b) = %d\n", sizeof(a), sizeof(b));
		printf("a = %s\n", a);
		printf("b = %s\n", b);
	}
	return 0;
}
