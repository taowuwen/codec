#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>


int main(int argc, char **argv)
{
	char *my_argv[10];
	int i = 0;

	if (argc < 2)
		exit(1);

	setuid(0);

	for (i = 0; i < 9 && i < argc -1; i++) {
		my_argv[i] = argv[i + 1];
	}

	my_argv[i] = NULL;

	execv(my_argv[0], my_argv);
}
