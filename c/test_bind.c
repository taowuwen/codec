

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/select.h>
#include <unistd.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/file.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <arpa/inet.h>

static int make_sock(struct sockaddr_in *addr, int type, int dienow)
{
	int family = addr->sin_family;
	int fd, rc, opt = 1;

	if ((fd = socket(family, type, 0)) == -1)
	{
		int port, errsav;
		char *s;

		/* No error if the kernel just doesn't support this IP flavour */
		if (errno == EPROTONOSUPPORT ||
			errno == EAFNOSUPPORT ||
			errno == EINVAL)
			return -1;

err:
		errsav = errno;

		if (fd != -1)
			close (fd);

		errno = errsav;

		printf("err: %s\n", strerror(errno));

		return -1;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
		goto err;

	if ((rc = bind(fd, (struct sockaddr *)addr, sizeof(struct sockaddr_in))) == -1)
		goto err;

	if (type == SOCK_STREAM)
	{
		if (listen(fd, 5) == -1)
		goto err;
	}

	return fd;
}



int main(int argc, char **argv)
{
	struct sockaddr_in addr;
	int fd, tcpfd;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(10000);
	addr.sin_addr.s_addr = htonl(0);


	printf("bind udp:\n");
	fd    = make_sock(&addr, SOCK_DGRAM, 0);

	printf("bind udp: ready: fd: %d\n", fd);

	printf("bind tcp:\n");
	tcpfd = make_sock(&addr, SOCK_STREAM, 0);

	printf("bind tcp: ready: %d\n", tcpfd);


	printf("bind both at tcp and udp port: udp: %d tcp fd: %d \n", fd, tcpfd);

	sleep(10);

	close(fd);
	close(tcpfd);

	return 0;
}

