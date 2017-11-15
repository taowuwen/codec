
#include "ems_common.h"
#include "ems_buffer.h"
#include "ems_sock.h"

ems_int ems_sock_init(ems_sock *s)
{
	memset(s, 0, sizeof(ems_sock));
	str_init(&s->addr);
	return EMS_OK;
}

ems_void ems_sock_clear(ems_sock *s)
{
	ems_assert(s != NULL);

	str_clear(&s->addr);
	ems_sock_init(s);
}

ems_int ems_sock_setaddr(ems_sock *s, ems_cchar *addr)
{
	ems_assert(s != NULL);
	str_set(&s->addr, addr);
	return EMS_OK;
}

ems_int ems_sock_setport(ems_sock *s, ems_int port)
{
	ems_assert(s != NULL);

	s->port = port;

	return port;
}

ems_int ems_sock_setfd(ems_sock *s, ems_int fd)
{
	ems_assert(s != NULL);

	s->fd = fd;

	return fd;
}

ems_cchar *ems_sock_addr(ems_sock *s)
{
	return str_text(&s->addr);
}

ems_int ems_sock_port(ems_sock *s)
{
	return s->port;
}

ems_int ems_sock_fd(ems_sock *s)
{
	return s->fd;
}

ems_int ems_sock_close(ems_sock *s)
{
	if (s->fd > 0) {
		close(s->fd);
		s->fd = 0;
	}

	return EMS_OK;
}

