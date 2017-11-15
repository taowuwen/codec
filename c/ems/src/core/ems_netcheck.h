
#ifndef EMS_CLIENT_NET_CHECK_____
#define EMS_CLIENT_NET_CHECK_____

#define DNS_INTERCEPT_PORT	9112

typedef struct  _netcheck_s  netcheck;

struct _netcheck_s 
{
	ems_status   st;

	ems_uint     flg;
	ems_uint     ntrans;
	ems_int      ident;
	ems_int      id;
	ems_int      retry;
	ems_session *sess;
	ems_session *sess_dns; /* for udp only */
};

ems_int ping_change_status(netcheck *ping, ems_status st);

#endif
