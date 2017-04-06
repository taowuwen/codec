
#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Psapi.h>
#endif

#include "ems.h"


#ifdef WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

#include "ems_main.h"
#include "ems_msg.h"

ems_int  ems_sock_send(ems_sock *sock, ems_buffer *buf)
{
	ems_char *p;
	ems_int  left;
	ems_int  ret;
	ems_int  fd = ems_sock_fd(sock);

	if (ems_sock_fd(sock) <= 0)
		return EMS_ERR;

	p     = buf_rd(buf);
	left  = buf_len(buf);

	while (left > 0) {
		ret = send(fd, p, left, 0);
		if (ret <= 0) {
			switch(errno) {

			case EAGAIN:
			case EINTR:
				ems_buffer_seek_rd(buf, abs(p-buf_rd(buf)), EMS_BUFFER_SEEK_CUR);
				return -EAGAIN;

			default:
				ems_l_trace("session[%d]send error: %s", fd, ems_lasterrmsg());
				ems_sock_close(sock);
				return EMS_ERR;
			}
			break;
		}

		p    += ret;
		left -= ret;
	}

	ret = abs(p - buf_rd(buf));
	ems_buffer_seek_rd(buf, ret, EMS_BUFFER_SEEK_CUR);

	return  ret;
}

ems_int  ems_sock_read(ems_sock *sock, ems_buffer *buf)
{
	ems_int  ret;
	ems_int  fd, left;
	ems_char *wr = buf_wr(buf);

	if (ems_sock_fd(sock) <= 0)
		return EMS_ERR;

	ems_assert(buf_left(buf) > 0);
	fd   = ems_sock_fd(sock);
	left = buf_left(buf);

	while (left > 0) {
		ret = recv(fd, wr, left, 0);

		if (ret > 0) {
			wr   += ret;
			left -= ret;
		} else if (ret == 0) {
			ems_buffer_seek_wr(buf, abs(wr-buf_wr(buf)), EMS_BUFFER_SEEK_CUR);
			ems_l_trace("session[%d] down", fd);
			return 0;
		} else {
			switch(errno) {

			case EAGAIN:
			case EINTR:
				ems_buffer_seek_wr(buf, abs(wr-buf_wr(buf)), EMS_BUFFER_SEEK_CUR);
				return -EAGAIN;
				break;
			default:
				ems_sock_close(sock);
				ems_l_trace("session[%d]recv error %s", fd, ems_lasterrmsg());
				return ret;
			}

			break;
		}
	}

	ret = abs(wr - buf_wr(buf));
	ems_buffer_seek_wr(buf, ret, EMS_BUFFER_SEEK_CUR);

	return ret;
}


ems_void ems_printhex(ems_cchar *s, ems_int len)
{
	ems_int  n, ret;
	ems_char buf[64], *p;

	n = 16;
	p = buf;
	while (len > 0) {

		if ( n == 0 ) {
			ems_l_trace("%s", buf);
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

	ems_l_trace("%s", buf);
}


ems_int ems_pack_req(ems_uint tag, ems_cchar *ctx, ems_int len, ems_buffer *buf)
{
	ems_uint     total;
	ems_request *req;
	ems_char    *p;

	if (len + SIZE_REQUEST > buf_left(buf)) {
		ems_assert(len < 2 * EMS_BUFFER_16k * 64);
		if (ems_buffer_increase(buf, len + SIZE_REQUEST) == EMS_ERR)
			return EMS_ERR;
	}

	req = (ems_request *)buf_wr(buf);
	p   = (ems_char *)  (buf_wr(buf) + SIZE_REQUEST);

	if (ctx)
		putmem(p, ctx, len);

	total = abs(p - buf_wr(buf));
	ems_buffer_seek_wr(buf, total, EMS_BUFFER_SEEK_CUR);
	
	req->tag.val = ntohl(tag);
	req->len     = ntohl(total);

	return EMS_OK;
}

ems_int ems_pack_rsp(ems_uint tag, ems_int st, ems_cchar *ctx, ems_int len, ems_buffer *buf)
{
	ems_int       total;
	ems_response *rsp;
	ems_char     *p;

	if ((len + SIZE_RESPONSE) > buf_left(buf)) {
		ems_assert(len < 2 * EMS_BUFFER_16k * 64);
		if (ems_buffer_increase(buf, len + SIZE_RESPONSE) == EMS_ERR)
			return EMS_ERR;
	}

	rsp = (ems_response *)buf_wr(buf);
	p   = (ems_char *)   (buf_wr(buf) + SIZE_RESPONSE);

	if (ctx)
		putmem(p, ctx, len);

	total = abs(p - buf_wr(buf));
	ems_buffer_seek_wr(buf, total, EMS_BUFFER_SEEK_CUR);
	
	rsp->tag.val = ntohl(tag | 0x80000000);
	rsp->len = ntohl(total);
	rsp->st  = ntohl(st);

	return EMS_OK;
}


ems_int ems_gethostbyname(ems_cchar *domain, struct sockaddr_in *dst)
{
	struct hostent	*remote;
	struct in_addr   addr;

	ems_assert(domain && dst);

	remote = gethostbyname(domain);
	if (!remote && !isalpha(domain[0])) {
		addr.s_addr = inet_addr(domain);
		if (addr.s_addr != INADDR_NONE)
			remote = gethostbyaddr((char *) &addr, 4, AF_INET);
	}

	if (!remote)
		return ERR;

	dst->sin_addr.s_addr = *(u_long *) remote->h_addr_list[0];
	memcpy(&dst->sin_addr, remote->h_addr, remote->h_length);

	return OK;
}

#ifdef WIN32
ems_int ems_setnonblocking(ems_int sockfd, ems_int yes)
{
	ems_ulong opts = 1;

	if (!yes) 
		opts = 0;

	if (SOCKET_ERROR == ioctlsocket(sockfd, FIONBIO, (ems_ulong *)&opts))
		return ERR;

	return OK;
}
#else
ems_int ems_setnonblocking(ems_int sockfd, ems_int yes)
{
	int opts;

	opts = fcntl(sockfd, F_GETFL);

	opts = (opts | O_NONBLOCK);
	if ( !yes )
		opts = (opts ^ O_NONBLOCK);

	if (fcntl(sockfd, F_SETFL,opts) < 0)
		return ERR;

	return OK;
}
#endif


/*
 * which gonna cause send or recv return in msecs/1000 seconds.
 * check  whether errno == EAGAIN and try again.
 * WSAETIMEDOUT on windows gonna return : check
 * http://msdn.microsoft.com/en-us/library/windows/desktop/ms740476(v=vs.85).aspx
 * */
ems_int ems_setsock_rw_timeout(ems_int sockfd, ems_int msecs)
{
#ifdef WIN32
	ems_int to = msecs;
	setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (ems_char*)&to, sizeof(to));
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (ems_char*)&to, sizeof(to));
#else
	struct timeval to = {msecs/1000, 0};

	setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &to, sizeof(to));
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to));
#endif

	return OK;
}


ems_cchar *ems_public_addr()
{
	struct sockaddr_in serv;
	struct sockaddr_in name;
	static ems_char     buffer[100];
	socklen_t          namelen;

	ems_cchar *google_dns_server = "8.8.8.8";
	ems_int    google_dns_port   = 53;
	ems_int    sock;

	sock = socket (AF_INET, SOCK_DGRAM, 0);

	memset( &serv, 0, sizeof(serv) );
	serv.sin_family      = AF_INET;
	serv.sin_addr.s_addr = inet_addr( google_dns_server );
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

static ems_int
ems_sock_setopt_bind(ems_int fd)
{
	ems_int on;
	struct linger so_linger;

	on = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (ems_cchar *)&on, sizeof(on));

	so_linger.l_onoff  = 1;
	so_linger.l_linger = 30;
	setsockopt(fd, SOL_SOCKET, SO_LINGER, (ems_cchar *)&so_linger, sizeof(so_linger));

	return EMS_OK;
}


ems_int ems_sock_be_server(ems_sock *sock)
{
	ems_int  ret;
	ems_int  fd;
	struct sockaddr_in addr;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd <= 0)
		return EMS_ERR;

	ems_sock_setopt_bind(fd);

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ems_sock_addr(sock));
	addr.sin_port   = htons((unsigned short)ems_sock_port(sock));

	ret = bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr));

	if (ret == -1) {
		ems_l_warn("[bind]bind error:%d, %s", ret, ems_lasterrmsg());
		close(fd);
		return EMS_ERR;
	}
	
	if (listen(fd, SOMAXCONN)) {
		ems_l_warn("[bind]listen error: %s", ems_lasterrmsg());
		close(fd);
		return EMS_ERR;
	}

	ems_sock_setfd(sock, fd);

	ems_l_trace("[bind] socket: %d bind @address %s:%d ready", 
						ems_sock_fd(sock),
						ems_sock_addr(sock),
						ems_sock_port(sock)); 
	return EMS_OK;
}

