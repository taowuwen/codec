
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <error.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


int main(int argc, char *argv[])
{
	int fd;
	ssize_t sz;
	int rtn;
	char buf[1024];

	if (argc < 2) 
		return  0;

	fd = open(argv[1], O_RDONLY | O_NONBLOCK | O_ASYNC | O_NDELAY);

	if (fd <= 0)
		return -1;

	sz = 0;
	do {
		while ((rtn = read(fd, buf, 1024))> 0) {
			sz += rtn;
		}

		if (rtn == 0) {
			printf("got eof ...\n");
			break;
		}

		switch(errno) {
		case EAGAIN:
		case EINTR:
			printf("recv it again");
			break;

		default:
			goto err_out;
		}
	} while (1);


err_out:
	close(fd);
	printf("total read: %ld bytes\n", sz);
	return 0;
}
