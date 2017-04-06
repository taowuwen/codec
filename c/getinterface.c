#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ctype.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


const char *ems_public_addr(const char *addr)
{
	struct sockaddr_in serv;
	struct sockaddr_in name;
	static char     buffer[100];
	socklen_t          namelen;

	int    google_dns_port   = 53;
	int    sock;

	sock = socket (AF_INET, SOCK_DGRAM, 0);

	memset( &serv, 0, sizeof(serv) );
	serv.sin_family      = AF_INET;
	serv.sin_addr.s_addr = inet_addr( addr );
	serv.sin_port        = htons(google_dns_port);

	if (connect(sock, (struct sockaddr*) &serv, sizeof(serv)) == -1) {
		close(sock);
		return NULL;
	}

	namelen = sizeof(name);
	getsockname(sock, (struct sockaddr*) &name, &namelen);

	memset(buffer, 0, sizeof(buffer));
	inet_ntop(AF_INET, &name.sin_addr, buffer, 100);

	close(sock);

	return buffer;
}


int main(int argc, char *argv[])
{
	const char *domain;
	struct hostent	*remote;
	struct in_addr   addr;
	char *paddr;
	int   i;

	if (argc <= 1) {
		fprintf(stderr, "usage: %s address\n", argv[0]);
		return 1;
	}

	domain = argv[1];

	remote = gethostbyname(domain);
	if (!remote)
		fprintf(stderr, "Warnning gethostbyname(%s) failed: %s\n", domain, strerror(errno));

	if (!remote && !isalpha(domain[0])) {

		addr.s_addr = inet_addr(domain);
		if (addr.s_addr != INADDR_NONE) {
			remote = gethostbyaddr((char *) &addr, 4, AF_INET);
			if (!remote)
				fprintf(stderr, "Warnning gethostbyaddr(%s) failed: %s\n", domain, strerror(errno));
		}
	}

	if (!remote) {
		fprintf(stderr, "failed to parse args: %s\n", domain);
		return 1;
	}

	fprintf(stdout, "gethostbyname: %s\n"
			"\tname: %s\n"
			"\taliases: \n",
			domain, remote->h_name);

	for (i = 0; ; i++) {
		paddr = remote->h_aliases[i];
		if (!paddr)
			break;
		fprintf(stdout, "\t\t%s\n", paddr);
	}


	fprintf(stdout, "\taddrtype: %d\n", remote->h_addrtype);
	fprintf(stdout, "\tlength: %d\n", remote->h_length);

	fprintf(stdout, "\taddress list:\n");


	for (i = 0; ; i++) {
		paddr = remote->h_addr_list[i];
		if (!paddr)
			break;

		memcpy(&addr, paddr, remote->h_length);
		fprintf(stdout, "\t\t%s\n", inet_ntoa(addr));

		fprintf(stdout, "\t\t\tpublic address route: %s\n", ems_public_addr(inet_ntoa(addr)));
	}

	return 0;
}
