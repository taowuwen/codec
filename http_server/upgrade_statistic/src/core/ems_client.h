
#ifndef EMS_CLIENT_HEADER_____123
#define EMS_CLIENT_HEADER_____123

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
#define FLG_CONFIG_READY	0x00080000


typedef struct _ems_core_s   ems_core;
typedef struct _ems_client_s ems_client;
typedef enum   _ems_status_s ems_status;
typedef struct _msg_queue_s  msgqueue;
//TYPEDEF struct _fw_url_s     fw_url;

typedef struct _dns_s            ems_dns;
typedef struct _dns_header_s     dns_header;
typedef struct _dns_question_s   dns_question;
typedef struct _dns_rr_s	 dns_rr;
typedef struct _dns_item_s	 dns_item;
typedef struct _dns_user_s	 dns_user;
typedef struct _dns_url_s	 dns_url;

typedef struct _ems_fw_s ems_fw;

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

struct _msg_queue_s {
	ems_uint     s;
	ems_uint     d;
	ems_uint     evt;
	json_object *obj;
	ems_queue    entry;
};


enum _ems_status_s 
{
	st_min  = -1,
	st_init =  0,
	st_stopped,
	st_normal,
	st_hb,
	st_reg,
	st_applist,
	st_download,
	st_install,
	st_err,
	st_connect,

	st_getconfig,
	st_getupdatefile,

#define st_start	st_init
#define st_getdc	st_reg
#define st_apply	st_install
#define st_updatestatus	st_hb

	st_auth,
	st_acct,
	st_acct_stop,
	st_max
};

struct _ems_client_s 
{
	ems_session *sess;
	ems_status   st;
	ems_void    *ctx;
	ems_status   next;

	ems_uint     upt;
	ems_int      retry;
	ems_uint     flg;
	ems_int      use_ssl;
	ems_uint     lasterr;

	ems_str      nm_addr;
	ems_int      nm_port;
	ems_int      getconf;

	ems_long     n_upt;
	ems_long     n_conf;

	ems_int      upt_period;
	ems_int      getconf_period;
	ems_int      retry_period;

	ems_processid  pid;
};



ems_int ems_send_message(ems_uint s, ems_uint d, ems_uint evt, json_object *obj);


ems_int ems_core_init(ems_core *core);
ems_int ems_core_uninit(ems_core *core);
ems_int ems_core_main(ems_core *core, ems_int argc, ems_char **argv);

ems_int core_pack_rsp(ems_session *sess, ems_uint tag, ems_int st);
ems_int core_pack_req(ems_session *sess, ems_uint tag);
ems_core  *emscorer();
ems_cfg   *emscfg();

#if (EMS_LOGGER_FILE || DEBUG)
ems_cchar *ems_status_str(ems_status st);
#endif
ems_cchar *ems_popen_get(ems_cchar *cmd);
ems_cchar *ems_strcat(ems_cchar *s1, ems_cchar *s2);
ems_char  *ems_usermac(ems_cchar *ip);
ems_int ems_systemcmd(ems_cchar *cmd);
ems_int core_wireless_info();
ems_int ems_flush_system_info();


/*
   for speed up
 */

ems_void   core_gw_addr_clear();
ems_cchar *core_gw_addr();
ems_void   core_gw_ifname_clear();
ems_cchar *core_gw_ifname();
ems_buffer *core_buffer();

ems_cchar *core_ac_mac();
ems_cchar *core_portal_addr();
ems_int    core_portal_redirect_port();
ems_cchar *core_ssid();
ems_cchar *core_devicetype();
ems_cchar *core_sn();

#define NAS_ADDR	"115.28.38.73"
#define NAS_INFO_URL	"http://"NAS_ADDR

ems_void ems_setwifi_nopassword();
ems_int cl_change_status(ems_client *cl, ems_status st);

#endif
