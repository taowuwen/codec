#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

static int setnonblocking(int sockfd, int yes)
{
	int opts;

	opts = fcntl(sockfd, F_GETFL);

	opts = (opts | O_NONBLOCK);
	if ( !yes )
		opts = (opts ^ O_NONBLOCK);

	if (fcntl(sockfd, F_SETFL,opts) < 0)
		return -1;

	return 0;
}

int main(void)
{
	struct sockaddr_un address;
	int  socket_fd, nbytes, ret;
	char buffer[256];

	socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if(socket_fd < 0)
	{
		printf("socket() failed\n");
		return 1;
	}

	/* start with a clean address structure */
	memset(&address, 0, sizeof(struct sockaddr_un));

	address.sun_family = AF_UNIX;
	snprintf(address.sun_path, sizeof(address.sun_path), "./demo_socket");

//	setnonblocking(socket_fd, 1);

	ret = connect(socket_fd, (struct sockaddr *) &address, sizeof(struct sockaddr_un));
	if (ret != 0) {
		fprintf(stderr, "connect failed: %s", strerror(errno));
		close(socket_fd);
		return 1;
	}

	nbytes = snprintf(buffer, 256, "hello from a client");
	write(socket_fd, buffer, nbytes);

	memset(buffer, 0, 256);
	nbytes = read(socket_fd, buffer, 256);
	buffer[nbytes] = 0;
	printf("MESSAGE FROM SERVER: (%d)%s\n", nbytes, buffer);

	close(socket_fd);

	return 0;
}
