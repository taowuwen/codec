
#ifndef EMS_CLIENT_HEADER_____123
#define EMS_CLIENT_HEADER_____123

#include "ems_core.h"
#include "ems_conf.h"

#define FLG_AUTO_REG		0x00000001
#define FLG_RUN			0x00000002
#define FLG_INITED		0x00000004
#define FLG_CLIENT_ONLINE	0x80000000
#define FLG_NETWORK_BRIDGE_RUN	0x40000000
#define FLG_NETWORK_READY	0x20000000
#define FLG_NETWORK_BRIDGE	0x10000000
#define FLG_NETWORK_WIRELESS	0x08000000
#define FLG_RADIUS_AUTO		0x04000000
#define FLG_PORTAL_AUTO		0x02000000
#define FLG_SESSION_IS_WEB	0x01000000
#define FLG_NETWORK_LAN_BACK	0x00800000
#define FLG_SUBDOMAIN_ENABLE	0x00400000
#define FLG_NET_DNS_RUN		0x00200000
#define FLG_FIRST_CONFIG	0x00100000
#define FLG_FILE_FULL		0x00080000


typedef struct _ems_core_s   ems_core;

struct _ems_core_s
{
	ems_cfg       cfg;
	ems_event     evt;
	ems_uint      flg;

	ems_queue     app_entry;

	ems_buffer    buf; // for buffer control, for speed up
	ems_str       gw;
	ems_str       ifname;
	ems_str       ac_mac;
	ems_str       portal;
	ems_int       portal_redirect_port;
	ems_str       ssid;
	ems_str       devty;
	ems_str       sn;

	/* for msg queues */
	ems_queue     msg_entry;
	ems_mtx       msg_mtx;
};

ems_int ems_core_init(ems_core *core);
ems_int ems_core_uninit(ems_core *core);
ems_int ems_core_main(ems_core *core, ems_int argc, ems_char **argv);

ems_int core_pack_rsp(ems_session *sess, ems_uint tag, ems_int st);
ems_int core_pack_req(ems_session *sess, ems_uint tag);
ems_core  *emscorer();
ems_cfg   *emscfg();

ems_cchar *ems_popen_get(ems_cchar *cmd);
ems_cchar *ems_strcat(ems_cchar *s1, ems_cchar *s2);
ems_char  *ems_usermac(ems_cchar *ip);
ems_int ems_systemcmd(ems_cchar *cmd);
ems_int core_wireless_info();
ems_int ems_flush_system_info();


/*
   for speed up
 */
ems_buffer *core_buffer();


#endif
