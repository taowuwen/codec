
#ifndef EMS_ONLINE_MGMT_HEADER___
#define EMS_ONLINE_MGMT_HEADER___


typedef struct _tunnel_session_s  tunnel_session;
typedef struct _ems_tunnel_s      ems_tunnel;
typedef struct _ems_tunnel_pkg_s  ems_tunnel_pkg;

struct _ems_tunnel_s {

	ems_session   *sess; /* for tunnel */

	ems_hash       hash_sess;
	ems_queue      list_sess;

	ems_status     st;
	ems_uint       flg;

	struct {
		ems_str  addr;
		ems_int  port;
		ems_int  hb;
		ems_int  enable;
		ems_uint id;
	};
};


struct _ems_tunnel_pkg_s {
	ems_short  flgty; /* flag and type flgty=( flags << 12 | type)  */

#define  PH_RST 	0x8000
#define  PH_RSV0	0x4000
#define  PH_RSV1	0x2000
#define  PH_RSV2	0x1000
	ems_short  len;   /* pkg length, max length 2k + header length*/
	ems_uint   id;    /* pkg id */
	ems_char   val[0]; /* values */
};

struct _tunnel_session_s {
	ems_hash_fd   hash;
	ems_session  *sess;
	ems_queue     entry;
	ems_uint      id;
	ems_tunnel   *tunnel;

	ems_status    st;
};

#define SIZE_TUNNEL_HL	8

#define MSG_TYPE_TUNNEL	0x0001
#define MSG_TYPE_WEB	0x0002

#define MSG_WEB_DOWN	(MSG_TYPE_WEB | PH_RST)
#define MSG_TUNNEL_DOWN	(MSG_TYPE_TUNNEL | PH_RST)


ems_int tunnel_change_status(ems_tunnel *tunnel, ems_status);
ems_int tunnel_packmsg(ems_tunnel *tunnel, ems_short ty, ems_uint id, ems_short len, ems_uchar *val);

ems_int tunnel_session_connect(ems_session *sess);

tunnel_session *tunnel_session_new(ems_tunnel *tunnel, ems_uint id);
tunnel_session *tunnel_session_find(ems_hash *hash, ems_uint id);
ems_int tunnel_unpackmsg(ems_tunnel_pkg *pkg, tunnel_session *tsess);
ems_int tunnel_session_change_status(tunnel_session *tsess, ems_status st);

#define tsess_change_status	tunnel_session_change_status




#endif
