#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/time.h>
#include <sys/resource.h>


int connection_handler(int connection_fd)
{
	int nbytes;
	char buffer[256];


	while (1) {
		nbytes = read(connection_fd, buffer, 256);
		if (nbytes <= 0) {
			fprintf(stderr, "read failed: ret:%d, %s\n", nbytes, strerror(errno));

			printf("sleep 20s");
			sleep(20);
			break;
		}

		buffer[nbytes] = 0;
		

		printf("MESSAGE FROM CLIENT: %s\n", buffer);
		nbytes = snprintf(buffer, 256, "hello from the server");

		nbytes = write(connection_fd, buffer, nbytes);
		if (nbytes <= 0) {
			fprintf(stderr, "write failed: ret:%d, %s\n", nbytes, strerror(errno));
			break;
		}
	}

	close(connection_fd);
	return 0;
}

int main(void)
{
	struct sockaddr_un address;
	int socket_fd, connection_fd;
	socklen_t address_length;
	pid_t child;

	socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if(socket_fd < 0)
	{
		printf("socket() failed\n");
		return 1;
	} 

	unlink("./demo_socket");

	/* start with a clean address structure */
	memset(&address, 0, sizeof(struct sockaddr_un));

	address.sun_family = AF_UNIX;
	snprintf(address.sun_path, sizeof(address.sun_path), "./demo_socket");

	if(bind(socket_fd, (struct sockaddr *) &address, sizeof(struct sockaddr_un)) != 0)
	{
		printf("bind() failed\n");
		return 1;
	}

	if(listen(socket_fd, 5) != 0)
	{
		printf("listen() failed\n");
		return 1;
	}

	while((connection_fd = accept(socket_fd, (struct sockaddr *) &address, &address_length)) > -1)
	{
		child = fork();
		if(child == 0)
		{
			struct rlimit rl;
			int i;

			char *argv[10] = {0};
			char *cmd = "/bin/sleep";

			if (getrlimit(RLIMIT_NOFILE, &rl))
				rl.rlim_max = 1024;

			if (rl.rlim_max == RLIM_INFINITY)
				rl.rlim_max = 1024;

			for (i = 3; i < rl.rlim_max; i++)
				close(i);


			argv[0] = cmd;
			argv[1] = "15s";
			argv[2] = NULL;

			if (execv(cmd, argv) != 0) {
				fprintf(stderr, "exec: %s failed: %s\n", cmd, strerror(errno));
			}
			
		//	close(socket_fd);
			/* now inside newly created connection handling process */
		//	return connection_handler(connection_fd);
		}

		/* still inside server process */
		close(connection_fd);
		break;
	}

	close(socket_fd);
	unlink("./demo_socket");
	return 0;
}
