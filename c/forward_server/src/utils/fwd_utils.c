
#include "dt.h"
#include "fwd_utils.h"
#include "fwd.h"


dt_int fwd_cpucore()
{
	return (dt_int) sysconf(_SC_NPROCESSORS_ONLN);
}

dt_int fwd_pagesize()
{
	return (dt_int) sysconf(_SC_PAGESIZE);
}

dt_void fwd_printhex(dt_cchar *s, dt_int len)
{
	dt_int  n, ret;
	dt_char buf[64], *p;

	n = 16;
	p = buf;
	while (len > 0) {

		if ( n == 0 ) {
			log_trace("%s", buf);
			n = 16;
			p = buf;
			continue;
		}

		if (n == 8)
			ret = snprintf(p, 64, "  %02x", *s & 0xff);
		else
			ret = snprintf(p, 64, " %02x",  *s & 0xff);

		n--;
		p += ret;
		s++;
		len --;
	}

	log_trace("%s", buf);
}

dt_cchar *fwd_public_addr()
{
	struct sockaddr_in serv;
	struct sockaddr_in name;
	static dt_char     buffer[100];
	socklen_t          namelen;

	dt_cchar *google_dns_server = "8.8.8.8";
	dt_int    google_dns_port   = 53;
	dt_int    sock;

	sock = socket (AF_INET, SOCK_DGRAM, 0);

	memset( &serv, 0, sizeof(serv) );
	serv.sin_family      = AF_INET;
	serv.sin_addr.s_addr = inet_addr( google_dns_server );
	serv.sin_port        = htons(google_dns_port);

	if (connect(sock, (struct sockaddr*) &serv, sizeof(serv)) == -1) {
		log_info("connect failed: %s", fwd_lasterrstr());
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

#if 0
dt_cchar *fwd_public_addr_ipv6()
{
	struct sockaddr_in6 serv;
	struct sockaddr_in6 name;
	static dt_char     buffer[100];
	socklen_t          namelen;

	dt_cchar *google_dns_server = "8.8.8.8";
	dt_int    google_dns_port   = 53;
	dt_int    sock;

	sock = socket (AF_INET6, SOCK_DGRAM, 0);

	memset( &serv, 0, sizeof(serv) );
	serv.sin_family      = AF_INET6;
	serv.sin_addr.s_addr = inet_addr(google_dns_server);
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
#endif

dt_int fwd_time_diff(struct timeval *t1, struct timeval *t2)
{
	return
		(t1->tv_sec - t2->tv_sec) * 1000 +
		(t1->tv_usec - t2->tv_usec) / 1000;
}
