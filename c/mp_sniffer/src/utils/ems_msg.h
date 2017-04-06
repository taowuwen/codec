
#ifndef EMS_MSG_HEADER___
#define EMS_MSG_HEADER___

ems_int ems_sock_send(ems_sock *, ems_buffer *buf);
ems_int ems_sock_read(ems_sock *, ems_buffer *buf);
ems_int ems_pack_msg(ems_uint tag, ems_cchar *ctx, ems_int len, ems_buffer *buf);
ems_int ems_sock_be_server(ems_sock *sock);;
ems_int ems_setsock_rw_timeout(ems_int sockfd, ems_int msecs);


#endif
