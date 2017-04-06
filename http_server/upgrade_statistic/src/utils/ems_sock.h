
#ifndef EMS_SOCK_ADDR_HEADER___
#define EMS_SOCK_ADDR_HEADER___

typedef struct _ems_sock_s      ems_sock;
struct _ems_sock_s {
	ems_str   addr;
	ems_int   port;
	ems_int   fd;
};

ems_int ems_sock_init(ems_sock *s);
ems_void ems_sock_clear(ems_sock *s);

ems_int ems_sock_setaddr(ems_sock *s, ems_cchar *addr);
ems_int ems_sock_setport(ems_sock *s, ems_int port);
ems_int ems_sock_setfd(ems_sock *s, ems_int fd);

ems_cchar *ems_sock_addr(ems_sock *);
ems_int    ems_sock_port(ems_sock *);
ems_int    ems_sock_fd(ems_sock *);

ems_int    ems_sock_close(ems_sock *);

#define ems_sock_reset	ems_sock_clear

#endif
