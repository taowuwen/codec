#include <stdio.h>
#include <sys/utsname.h>


int main(int argc, char *argv[])
{
	struct utsname un;
	uname(&un);

	printf("sysname:  %s\n", un.sysname);
	printf("nodename: %s\n", un.nodename);
	printf("release:  %s\n", un.release);
	printf("version:  %s\n", un.version);
	printf("machine:  %s\n", un.machine);

	return 0;
}
