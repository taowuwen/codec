
#ifndef FWD_SOCK_ADDR_HEADER___
#define FWD_SOCK_ADDR_HEADER___

dt_int fwd_sock_init(fwd_sock *s);
dt_void fwd_sock_clear(fwd_sock *s);

dt_int fwd_sock_setaddr(fwd_sock *s, dt_cchar *addr);
dt_int fwd_sock_setport(fwd_sock *s, dt_int port);
dt_int fwd_sock_setfd(fwd_sock *s, dt_int fd);

dt_cchar *fwd_sock_addr(fwd_sock *);
dt_int    fwd_sock_port(fwd_sock *);
dt_int    fwd_sock_fd(fwd_sock *);

dt_int    fwd_sock_close(fwd_sock *);

#define fwd_sock_reset	fwd_sock_init


dt_int  fwd_sock_send(fwd_sock *, fwd_buffer *buf);
dt_int  fwd_sock_read(fwd_sock *, fwd_buffer *buf);


#endif
