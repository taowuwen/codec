
#include <stdio.h>


void granted()
{
	printf("\nGranted!\n");
	fflush(stdout);
}

void do_it()
{
	char password[10];

	printf("Input your password: ");
	gets(password);


	if (strcmp(password, "password1")) {
		printf("\nLogin fail!!\n");
	} else {
		granted();
	}
}

int main(int argc, char *argv[])
{
	do_it();
	return 0;
}
