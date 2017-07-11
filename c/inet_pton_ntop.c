#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>


int main(int argc, char *argv[])
{
	int  i;
	char buf[1024], *addr;
	char dst[128];
	int  af;

	for (i = 1; i < argc; i++) {

		addr = argv[i];

		af = AF_INET;

		if (1 != inet_pton(af, addr, buf)) {
			af = AF_INET6;

			if (1 != inet_pton(af, addr, buf)) {

				fprintf(stderr, "Warning: invalid address: %s\n", argv[i]);
				continue;
			}
		}

		if (!inet_ntop(af, buf, dst, 128)) {
			fprintf(stderr, "Warning: inet_ntop failed for : %s\n", argv[i]);
			continue;
		}

		fprintf(stdout, "%s -> %s\n", argv[i], dst);

	}

#if 0
	const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
	int inet_pton(int af, const char *src, void *dst);
#endif

	return 0;
}

