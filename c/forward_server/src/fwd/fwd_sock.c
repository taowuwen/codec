
#include "fwd_main.h"
#include "fwd_buffer.h"
#include "fwd_sock.h"

dt_int fwd_sock_init(fwd_sock *s)
{
	memset(s, 0, sizeof(fwd_sock));
	str_init(&s->addr);
	return FWD_OK;
}

dt_void fwd_sock_clear(fwd_sock *s)
{
	dt_assert(s != NULL);

	str_clear(&s->addr);
	fwd_sock_init(s);
}

dt_int fwd_sock_setaddr(fwd_sock *s, dt_cchar *addr)
{
	dt_assert(s != NULL);
	str_set(&s->addr, addr);
	return FWD_OK;
}

dt_int fwd_sock_setport(fwd_sock *s, dt_int port)
{
	dt_assert(s != NULL);

	s->port = port;

	return port;
}

dt_int fwd_sock_setfd(fwd_sock *s, dt_int fd)
{
	dt_assert(s != NULL);

	s->fd = fd;

	return fd;
}

dt_cchar *fwd_sock_addr(fwd_sock *s)
{
	return str_text(&s->addr);
}

dt_int    fwd_sock_port(fwd_sock *s)
{
	return s->port;
}

dt_int    fwd_sock_fd(fwd_sock *s)
{
	return s->fd;
}

dt_int    fwd_sock_close(fwd_sock *s)
{
	if (s->fd > 0) {
		close(s->fd);
		s->fd = 0;
	}

	return FWD_OK;
}

dt_int  fwd_sock_send(fwd_sock *sock, fwd_buffer *buf)
{
	dt_char *p;
	dt_int  len;
	dt_int ret;
	dt_int fd = fwd_sock_fd(sock);

	if (fwd_sock_fd(sock) <= 0)
		return FWD_ERR;

	p    = buf_rd(buf);
	len  = buf_len(buf);

	while (len > 0) {
		ret = send(fd, p, len, 0);

		if (ret <= 0) {
			switch(errno) {

			case EAGAIN:
			case EINTR:
				break;
			default:
				log_trace("[%d]send msg failed: %s", fd, fwd_lasterrstr());
				fwd_sock_close(sock);
				return FWD_ERR;
			}
			break;
		}

		p   += ret;
		len -= ret;
	}

	len = buf_len(buf) - len;
	fwd_buffer_seek_rd(buf, len, FWD_BUFFER_SEEK_CUR);

	return  len;
}

dt_int  fwd_sock_read(fwd_sock *sock, fwd_buffer *buf)
{
	dt_int  ret;
	dt_int  fd;
	dt_char *wr = buf_wr(buf);

	if (fwd_sock_fd(sock) <= 0)
		return FWD_ERR;

	fd  = fwd_sock_fd(sock);
	dt_assert(buf_left(buf) > 0);

	while (buf_left(buf) > 0) {
		ret = recv(fd, buf_wr(buf), buf_left(buf), 0);

		if (ret > 0)
			fwd_buffer_seek_wr(buf, ret, FWD_BUFFER_SEEK_CUR);
		else if (ret == 0) {
			log_trace("[%d]recv msg failed: %s", fd, fwd_lasterrstr());
			return 0;
		} else {
			switch(errno) {

			case EAGAIN:
			case EINTR:
				if (buf_wr(buf) -wr > 0)
					goto out;
				return -EAGAIN;
				break;
			default:
				fwd_sock_close(sock);
				log_trace("[%d]recv msg failed: %s", fd, fwd_lasterrstr());
				return ret;
			}

			break;
		}
	}

out:
	return (buf_wr(buf) - wr);
}
