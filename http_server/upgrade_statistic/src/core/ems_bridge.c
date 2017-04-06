
#include "ems_core.h"
#include "ems_client.h"
#include "app.h"
#include "ems_bridge.h"
#include "ems_cmd.h"

#define BR_CONNECT_GW_TIMEOUT	3000
#define BR_HEARTBEAT_TIME	60000

static ems_int br_start  (ems_bridge *br, ems_session *sess, ems_uint flg);
static ems_int br_stopped(ems_bridge *br, ems_session *sess, ems_uint flg);
static ems_int br_connect(ems_bridge *br, ems_session *sess, ems_uint flg);
static ems_int br_normal (ems_bridge *br, ems_session *sess, ems_uint flg);
static ems_int br_br     (ems_bridge *br, ems_session *sess, ems_uint flg);
static ems_int br_err    (ems_bridge *br, ems_session *sess, ems_uint flg);

typedef ems_int (*br_evt_func)(ems_bridge *br, ems_session *sess, ems_uint flg);
static br_evt_func br_evt_handler[] = 
{
	[st_start]   = br_start,
	[st_stopped] = br_stopped,
	[st_connect] = br_connect,
	[st_normal]  = br_normal,
	[st_hb]      = br_br, 
	[st_err]     = br_err,
	[st_max]     = NULL
};


static ems_int br_to_connect(ems_bridge *br, ems_session *sess, ems_timeout *to);
static ems_int br_to_normal (ems_bridge *br, ems_session *sess, ems_timeout *to);
static ems_int br_to_br     (ems_bridge *br, ems_session *sess, ems_timeout *to);
static ems_int br_to_err    (ems_bridge *br, ems_session *sess, ems_timeout *to);

typedef ems_int (*br_timeout_func)(ems_bridge *br, ems_session *sess, ems_timeout *to);
static br_timeout_func br_timeout_handler[] = 
{
	[st_start]   = NULL,
	[st_stopped] = NULL,
	[st_connect] = br_to_connect,
	[st_normal]  = br_to_normal,
	[st_hb]      = br_to_br, 
	[st_err]     = br_to_err,
	[st_max]     = NULL
};

static ems_int br_evt_run(ems_bridge *br, ems_session *sess,  ems_uint flg)
{
	ems_assert(br_evt_handler[br->st]);
	return br_evt_handler[br->st](br, sess, flg);
}

static ems_void br_evt_cb(ems_session *sess, ems_int err, ems_int flg)
{
	ems_bridge *br = (ems_bridge *)sess_cbarg(sess);

	ems_assert(br->st > st_min && br->st < st_max);

	ems_l_trace("[br] evt: error? %s, flg: 0x%x", err?"yes":"no", flg);

	if (err) {
		ems_l_trace("[br] evt err, sess: %d %s",
				ems_sock_fd(&sess->sock),
				ems_sock_addr(&sess->sock));
		{
			if (ems_flag_like(emscorer()->flg, FLG_NETWORK_BRIDGE))
				br_change_status(br, st_hb);
			else {
				ems_send_message(ty_bridge, ty_downlink, EMS_APP_START, NULL);
				br_change_status(br, st_stopped);
			}
			return;
		}
	}

	br_evt_run(br, sess, flg);
}

static ems_void br_timeout_cb(ems_session *sess, ems_timeout *to)
{
	ems_bridge *br = (ems_bridge *)sess_cbarg(sess);

	ems_assert(br->st > st_min && br->st < st_max);

	ems_assert(br_timeout_handler[br->st]);

	if (br_timeout_handler[br->st])
		br_timeout_handler[br->st](br, sess, to);
}

static  ems_int br_connect_gw(ems_session *sess)
{
	ems_int    fd, ret;
	socklen_t  len;
	struct sockaddr_in addr;
	ems_sock   *sock = &sess->sock;

	ems_assert(sess);
	
	memset(&addr, 0, sizeof(addr));
	if (ems_gethostbyname(ems_sock_addr(sock), &addr) != OK) {
		ems_l_trace("gethostbyename failed %s : %s", 
				ems_sock_addr(sock), ems_lasterrmsg());
		return EMS_ERR;
	}

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd <= 0)
		return EMS_ERR;

	ems_l_trace("[br] sess(%d) try to connect to: %s(%s): %d...",
			fd, 
			ems_sock_addr(sock), 
			inet_ntoa(addr.sin_addr), 
			ems_sock_port(sock));

	addr.sin_family = AF_INET;
	addr.sin_port   = htons(ems_sock_port(sock));

	ems_setnonblocking(fd, YES);
	len = sizeof(struct sockaddr_in);
	ret = connect(fd, (struct sockaddr *)&addr, len);

	switch(ret) {
	case 0:
		ems_flag_set(sess->flg, EMS_FLG_ONLINE);
		break;

	case -1:
		ret = ems_lasterr();
		ems_flag_unset(sess->flg, EMS_FLG_ONLINE);

		if (ret != EINPROGRESS) {
			close(fd);
			ems_l_trace("[br] connect to: %s:%d: failed: %s",
					ems_sock_addr(sock), 
					ems_sock_port(sock), 
					ems_geterrmsg(ret));
			return EMS_ERR;

		}
	default:
		break;
	}

	ems_sock_setfd(sock, fd);
	return EMS_OK;
}

static ems_int br_detect_uplink(ems_bridge *br)
{
	ems_session *sess = NULL;
	ems_cfg     *cfg = emscfg();

	if (!br->sess) {
		br->sess = ems_session_new();
		if (!br->sess)
			return br_change_status(br, st_stopped);
	}

	sess = br->sess;
	sess_cbarg_set(sess, br);

	ems_sock_setaddr(&sess->sock, cfg_get(cfg, CFG_wan_gw));
	ems_sock_setport(&sess->sock, EMS_PORT);

	if (br_connect_gw(sess) != EMS_OK)
		return br_change_status(br, st_stopped);

	sess_event_set(sess,          EMS_EVT_WRITE,         br_evt_cb);
	sess_timeout_set_sorted(sess, BR_CONNECT_GW_TIMEOUT, br_timeout_cb);

	if (ems_flag_like(sess->flg, EMS_FLG_ONLINE)) {
		core_pack_req(sess,  CMD_BRIDGE_REQ);
		return br_change_status(br, st_normal);
	}

	return br_change_status(br, st_connect);
}

static ems_int br_start(ems_bridge *br, ems_session *sess, ems_uint flg)
{
	ems_cfg *cfg = emscfg();

	ems_l_trace("\033[00;32m\n"
		"\tlan: %s proto: %s (%s/%s) gw: %s\n"
		"\twan: %s proto: %s (%s/%s) gw: %s\n"
		"\twireless: %s ssid: %s, encryption: %s key: %s"
		"\033[00m",
			core_gw_ifname(),
			cfg_get(cfg, CFG_lan_proto),
			core_gw_addr(),
			cfg_get(cfg, CFG_lan_mask),
			cfg_get(cfg, CFG_lan_gw),

			cfg_get(cfg, CFG_wan_ifname),
			cfg_get(cfg, CFG_wan_proto),
			cfg_get(cfg, CFG_wan_addr),
			cfg_get(cfg, CFG_wan_mask),
			cfg_get(cfg, CFG_wan_gw),

			cfg_get(cfg, CFG_wireless_ifname),
			cfg_get(cfg, CFG_wireless_ssid),
			cfg_get(cfg, CFG_wireless_encrypt),
			cfg_get(cfg, CFG_wireless_key)
			);

	/* check network ready or not */
	if (!ems_flag_like(emscorer()->flg, FLG_NETWORK_READY)) 
	{
		ems_assert(0 && "should not be here");
		ems_l_trace("network not ready");
		return br_change_status(br, st_stopped);
	}

	ems_assert((cfg_get(cfg, CFG_wan_gw) || cfg_get(cfg, CFG_lan_gw)) && "never show up this");

	/*
	   check if we are in brigde now
	   lan gw not null and wan addr null
	 */
	if (!cfg_get(cfg, CFG_wan_addr) && cfg_get(cfg, CFG_lan_gw)) 
	{
		ems_l_trace("we are in brigde mode right now");

		ems_flag_set(emscorer()->flg, FLG_NETWORK_BRIDGE);
		ems_send_message(ty_bridge, ty_net,  EMS_APP_STOP, NULL);

		/* if client is running, stop it and other modules */
		//ems_send_message(ty_bridge, ty_client, EMS_APP_STOP, NULL);

		/* for send heartbeat */
		if (!br->sess)
			br->sess = ems_session_new();

		if (br->sess) {
			sess_cbarg_set(br->sess, br);
			return br_change_status(br, st_hb);
		}

		return br_change_status(br, st_stopped);
	}

	if (cfg_get(cfg, CFG_wan_addr) && cfg_get(cfg, CFG_wan_gw)) {

		ems_cchar *pro = cfg_get(cfg, CFG_wan_proto);
		ems_cchar *ifname = cfg_get(cfg, CFG_wan_ifname);
		  // set flag to connect method wired

		ems_l_trace("try to connect to gw: %s from : %s", 
				cfg_get(cfg, CFG_wan_gw), cfg_get(cfg, CFG_wan_addr));

		if (pro && !strcmp(pro, "pppoe")) {
			ems_l_trace("the uplink is pppoe server, we are fat ap");

			ems_send_message(ty_bridge, ty_downlink, EMS_APP_START, NULL);
			return br_change_status(br, st_stopped);
		}

		if (ifname && !strncmp(ifname, "apcli", 5)) {
			ems_l_trace("we are in wireless repeat mode, not support wireless brigde");
			ems_send_message(ty_bridge, ty_downlink, EMS_APP_START, NULL);
			return br_change_status(br, st_stopped);
		}

		/*
		   check if lan address same with gw address
		   the same? fix it
		 */
		{
			ems_cchar *lan_addr, *wan_gw;
			lan_addr = cfg_get(cfg, CFG_lan_addr);
			wan_gw   = cfg_get(cfg, CFG_wan_gw);

			if (lan_addr && wan_gw && !strcmp(lan_addr, wan_gw)) {
				ems_char buf[512];

				snprintf(buf, sizeof(buf), "ip addr del dev %s %s/%s",
						cfg_get(cfg, CFG_lan_ifname),
						lan_addr,
						cfg_get(cfg, CFG_lan_mask));
				ems_systemcmd(buf);

				ems_flag_set(emscorer()->flg, FLG_NETWORK_LAN_BACK);
			}
		}

		return br_detect_uplink(br);
	}

	ems_assert(0 && "we did not handle for now");
	ems_send_message(ty_bridge, ty_downlink, EMS_APP_START, NULL);
	return br_change_status(br, st_stopped);
}

static ems_int br_stopped(ems_bridge *br, ems_session *sess, ems_uint flg)
{
	ems_l_trace("[br] in status stoppped");

	if (br->sess) {
		ems_session_shutdown_and_destroy(br->sess);
		br->sess = NULL;
	}

	if (ems_flag_like(emscorer()->flg, FLG_NETWORK_LAN_BACK)) {
		ems_char buf[512];
		ems_cfg *cfg = emscfg();

		ems_flag_unset(emscorer()->flg, FLG_NETWORK_LAN_BACK);
		snprintf(buf, sizeof(buf), "ip addr add dev %s %s/%s",
				cfg_get(cfg, CFG_lan_ifname),
				cfg_get(cfg, CFG_lan_addr),
				cfg_get(cfg, CFG_lan_mask));
		ems_systemcmd(buf);
	}

	return EMS_OK;
}

static ems_int br_connect(ems_bridge *br, ems_session *sess, ems_uint flg)
{
	ems_int    err;
	socklen_t len;

	len = sizeof(err);
	err = 0;
	getsockopt(ems_sock_fd(&sess->sock), SOL_SOCKET, SO_ERROR, (ems_char *)&err, &len);

	if (err ) {
		errno = err;
		ems_l_trace("[br]sess(%d) connect to %s:%d failed, %s",
				ems_sock_fd(&sess->sock),
				ems_sock_addr(&sess->sock),
				ems_sock_port(&sess->sock),
				ems_geterrmsg(err));
		return br_change_status(br, st_stopped);
	}

	ems_l_trace("[br]sess(%d) established with %s:%d",
			ems_sock_fd(&sess->sock),
			ems_sock_addr(&sess->sock),
			ems_sock_port(&sess->sock));

	ems_flag_set(sess->flg, EMS_FLG_ONLINE);

	if (ems_flag_like(emscorer()->flg, FLG_NETWORK_BRIDGE))
		core_pack_req(sess, CMD_BRIDGE_HB);
	else 
		core_pack_req(sess, CMD_BRIDGE_REQ);

	sess_event_set(sess,   EMS_EVT_READ | EMS_EVT_WRITE,  br_evt_cb);
	sess_timeout_set_sorted(sess, BR_CONNECT_GW_TIMEOUT,  br_timeout_cb);

	return br_change_status(br, st_normal);
}

static ems_int br_send_msg(ems_bridge *br, ems_session *sess, ems_uint flg)
{
	ems_int ret;

	ems_assert(ems_flag_like(flg, EMS_EVT_WRITE));

	ret = sess_send(sess, &sess->buf);
	if (ret <= 0) {
		switch(ret) {

		case -EAGAIN:
			break;

		default:
			return ret;
		}
	}

	if (buf_len(&sess->buf) <= 0) {
		sess_event_set(sess, EMS_EVT_READ, br_evt_cb);
		ems_buffer_refresh(&sess->buf);
	}

	return EMS_OK;
}

typedef ems_int (*br_rsp_cb)(ems_bridge *, ems_session *, ems_response *, json_object *);
static ems_int 
br_process_rsp(ems_bridge *br, ems_session *sess, ems_response *rsp, br_rsp_cb h)
{
	struct json_object *root;
	ems_int      rtn = EMS_OK;

	ems_assert(br && sess && rsp);

	root = NULL;
	if (rsp->len > SIZE_RESPONSE) {
		ems_int     len;
		ems_char    *p, ch;

		ems_assert(buf_len(&sess->buf_in) >= rsp->len);

		p = (ems_char *)(buf_rd(&sess->buf_in) + SIZE_RESPONSE);
		getword(p, len);

		ch = p[len]; // backup
		p[len] = '\0';
		root = ems_json_tokener_parse(p);
		p[len] = ch; // restore
	}
#ifdef DEUBG
	{
		ems_cchar *ctx = root?json_object_to_json_string(root):"no ctx";

		ems_l_trace("\033[01;34m[br]<rsp tag: 0x%x, st: %d ctx: %s> \033[00m",
			rsp->tag.val, rsp->st, strlen(ctx)> 0x200?"**too long**":ctx);
	}
#endif

	if (h)
		rtn = h(br, sess, rsp, root);
	else
		rtn = EMS_ERR;

	if (root)
		json_object_put(root);

	ems_buffer_seek_rd(&sess->buf_in, rsp->len, EMS_BUFFER_SEEK_CUR);
	ems_buffer_refresh(&sess->buf_in);
	return rtn;
}


static ems_int br_preprocess(ems_bridge *br, ems_session *sess, br_rsp_cb h)
{
	ems_response rsp;

	ems_assert(br && sess && h);

	if (buf_len(&sess->buf_in) < SIZE_RESPONSE) 
		return EMS_CONTINUE;

	ems_buffer_prefetch(&sess->buf_in, (ems_char *)rsp.val, SIZE_RESPONSE);

	rsp.tag.val = ntohl(rsp.tag.val);
	rsp.len     = ntohl(rsp.len);
	rsp.st      = ntohl(rsp.st);

	if (rsp.len >= buf_size(&sess->buf_in)) {
		if (ems_buffer_increase(&sess->buf_in, rsp.len) != EMS_OK)
			return EMS_BUFFER_INSUFFICIENT;
	}

	if (buf_len(&sess->buf_in) < rsp.len)
		return EMS_CONTINUE;

	return br_process_rsp(br, sess, &rsp, h);
}


static ems_int 
br_recv_handle(ems_bridge *br, ems_session *sess, br_rsp_cb h)
{
	ems_int ret, again;

	again = EMS_YES;
recv_again:
	ret = sess_recv(sess, &sess->buf_in);
	if (ret <= 0) {
		switch (ret) {
		case -EAGAIN:
			again = EMS_NO;
			break;
		default:
			if (buf_len(&sess->buf_in) > 0) 
				br_preprocess(br, sess, h);
			return EMS_ERR;
		}
	}

	do {
		ret = br_preprocess(br, sess, h);

		switch (ret) {
		case EMS_BUFFER_INSUFFICIENT:
		case EMS_ERR:
			return EMS_ERR;

		case EMS_OK:
		case EMS_CONTINUE:
		default:
			break;
		}
	} while (ret != EMS_CONTINUE);

	if (again)
		goto recv_again;

	return EMS_OK;
}

/*
   command list below


   a. shutdown dhcp
   b. brctrl addinf $lan $wan

   c. iwpriv ra0 set AuthMode=OPEN/SHARED/WEPAUTO/WPAPSK, WPA2PSK, WPANONE
   d. iwpriv ra0 set EncrypType=NONE/WEP/TKIP/AES
   e. iwpriv ra0 set SSID="new ssid"

   if EncrypType == WEP ; then
	   iwpriv ra0 set DefaultKeyID=1
	   iwpriv ra0 set Key1="AP's wep key"
   else
   	iwpriv ra0 set WPAPSK="wpa preahsred key"

	iwpriv ra0 set AuthMode=SHARED
	iwpriv ra0 set EncrypType=WEP
	iwpriv ra0 set DefaultKeyID=1
	iwpriv ra0 set Key1="AP's wep key"
	iwpriv ra0 set SSID="AP's SSID"
			
   c. dhcp for $lan (for bridge, not really necessary
   d. stop all the other modules.

 */


typedef struct _wifi_cmd_s {
	ems_char  *nick;
	ems_char  *AuthMode;
	ems_char  *EncrypType;
	ems_char  *IEEE8021X;
	ems_char  *pre_SSID;
	ems_char  *key;
	ems_char  *keyid;
} br_wifi_cmd;

static br_wifi_cmd _gcmds[] = 
{
	/* nick          authmode       encrypttype 	ieee8021x     pressid   key    keyid */
	/* for none or open */
	{"none",	 "OPEN", 	"NONE", 	NULL,		NULL, 	NULL, 	NULL},
	{"open",	 "OPEN", 	"NONE", 	NULL, 		NULL, 	NULL, 	NULL},

	/* for wep* */
	{"wep-open",     "WEPAUTO",     "WEP",         "IEEE8021X=0",   NULL,  "Key1",  "DefaultKeyID=1"},
	{"wep-shared",   "WEPAUTO",     "WEP",         "IEEE8021X=0",   NULL,  "Key1",  "DefaultKeyID=1"},
	{"wep",          "WEPAUTO",     "WEP",         "IEEE8021X=0",   NULL,  "Key1",  "DefaultKeyID=1"},
	{"WEP",          "WEPAUTO",     "WEP",         "IEEE8021X=0",   NULL,  "Key1",  "DefaultKeyID=1"},

	/* for others */
	{"psk",          "WPAPSK",      "AES",         "IEEE8021X=0",  "SSID",  "WPAPSK", "DefaultKeyID=2"},
	{"psk+ccmp",     "WPAPSK",      "AES",         "IEEE8021X=0",  "SSID",  "WPAPSK", "DefaultKeyID=2"},
	{"psk+tkip",     "WPAPSK",      "TKIP",        "IEEE8021X=0",  "SSID",  "WPAPSK", "DefaultKeyID=2"},
	{"psk+tkip+ccmp","WPAPSK",      "TKIPAES",     "IEEE8021X=0",  "SSID",  "WPAPSK", "DefaultKeyID=2"},

	{"psk2",         "WPA2PSK",     "AES",         "IEEE8021X=0",  "SSID",  "WPAPSK", "DefaultKeyID=2"},
	{"psk2+ccmp",    "WPA2PSK",     "AES",         "IEEE8021X=0",  "SSID",  "WPAPSK", "DefaultKeyID=2"},
	{"psk2+tkip",    "WPA2PSK",     "TKIP",        "IEEE8021X=0",  "SSID",  "WPAPSK", "DefaultKeyID=2"},
	{"psk2+tkip+ccmp","WPA2PSK",    "TKIPAES",     "IEEE8021X=0",  "SSID",  "WPAPSK", "DefaultKeyID=2"},

	/* for mix and psk + psk2*/
	{"psk+psk2",     "WPAPSKWPA2PSK", "AES",       "IEEE8021X=0",  "SSID",  "WPAPSK", "DefaultKeyID=2"}
};

static br_wifi_cmd *wifi_mod_find(ems_cchar *enc)
{
	br_wifi_cmd *cmd;
	ems_int      total, i;


	total = sizeof(_gcmds) / sizeof(br_wifi_cmd);

	for (i = 0; i < total; i++) {
		cmd = &_gcmds[i];
		if (!strcmp(enc, cmd->nick))
			return cmd;
	}

	return NULL;
}


static ems_int br_set_wifi_info(ems_cchar *inf, br_wifi_cmd *wifi, ems_cchar *ssid, ems_str *key)
{
	ems_buffer  *cmd = core_buffer();

	ems_assert(inf && wifi && ssid);
	if (inf && wifi && ssid) {
		ems_char  *buf = buf_wr(cmd);
		ems_int    len = buf_left(cmd);

		snprintf(buf, len, "iwpriv %s set AuthMode=%s", inf, wifi->AuthMode);
		ems_systemcmd(buf);

		snprintf(buf, len, "iwpriv %s set EncrypType=%s", inf, wifi->EncrypType);
		ems_systemcmd(buf);

		if (wifi->IEEE8021X) {
			snprintf(buf, len, "iwpriv %s set %s", inf, wifi->IEEE8021X);
			ems_systemcmd(buf);
		}

		ems_assert(ssid != NULL);
		if (wifi->pre_SSID) {
			snprintf(buf, len, "iwpriv %s set %s=%s", inf, wifi->pre_SSID, ssid);
			ems_systemcmd(buf);
		}

		if (wifi->key) {
			snprintf(buf, len, "iwpriv %s set %s=%s", inf, wifi->key, str_text(key));
			ems_systemcmd(buf);
		}

		if (wifi->keyid) {
			snprintf(buf, len, "iwpriv %s set %s", inf, wifi->keyid);
			ems_systemcmd(buf);
		}

		snprintf(buf, len, "iwpriv %s set WscConfMode=0", inf);
		ems_systemcmd(buf);

		snprintf(buf, len, "iwpriv %s set SSID=%s", inf, ssid);
		ems_systemcmd(buf);

	}

	return EMS_OK;
}

static ems_int 
br_chrole_to_ap(ems_bridge *br, ems_str *ssid, ems_str *encrypt, ems_str *key)
{
	ems_int     rtn;
	ems_cfg    *cfg = emscfg();
	ems_buffer *cmd = core_buffer();
	br_wifi_cmd *wifi = NULL;

	do {
		rtn = EMS_ERR;

		wifi = wifi_mod_find(str_text(encrypt));

		if (!wifi) {
			ems_l_trace("did not find any wifi mod supply for : %s", str_text(encrypt));
			break;
		}

		if (wifi->key && str_len(key) <= 0) {
			ems_l_trace("key missing");
			break;
		}

		br_set_wifi_info(cfg_get(cfg, CFG_wireless_ifname), wifi, str_text(ssid), key);
		//br_set_wifi_info(cfg_get(cfg, CFG_wireless_ifname), wifi, cfg_get(cfg, CFG_wireless_ssid), key);
		{
			ems_cchar *lan_inf, *wan_inf;
			ems_char  *buf = buf_wr(cmd);
			ems_int    len = buf_left(cmd);

			ems_systemcmd("/etc/init.d/dnsmasq stop");
			ems_systemcmd("/etc/init.d/uhttpd stop");

			lan_inf = core_gw_ifname();
			wan_inf = cfg_get(cfg, CFG_wan_ifname);

			ems_assert(lan_inf && wan_inf);

			snprintf(buf, len, "brctl addif %s %s", lan_inf, wan_inf);
			ems_systemcmd(buf);

			snprintf(buf, len, 
				"ip addr del $(ip addr show dev %s | grep inet | awk '{print $2}') dev %s", 
				wan_inf, wan_inf);
			ems_systemcmd(buf);

			snprintf(buf, len, "ip route del default via %s dev %s", cfg_get(cfg, CFG_wan_gw), wan_inf);
			ems_systemcmd(buf);

			snprintf(buf, len, 
				"ip addr del $(ip addr show dev %s | grep inet | awk '{print $2}') dev %s", 
				lan_inf, lan_inf);
			ems_systemcmd(buf);

			snprintf(buf, len, "/sbin/udhcpc -i %s -xhostname:%s", lan_inf, cfg_get(cfg, CFG_wireless_ssid));
			ems_systemcmd(buf);
		}

		rtn = EMS_OK;
	} while (0);

	return rtn;
}

static ems_int 
br_rsp_req(ems_bridge *br, ems_session *sess, json_object *root)
{
	ems_int rtn;
	ems_str ssid;
	ems_str encrypt;
	ems_str key;

	ems_l_trace("uplink's return: %s", json_object_to_json_string(root));

	str_init(&ssid);
	str_init(&encrypt);
	str_init(&key);

	do {
		rtn = EMS_ERR;
		ems_json_get_string_def(root, "ssid", &ssid, NULL);
		ems_json_get_string_def(root, "encryption", &encrypt, NULL);
		ems_json_get_string_def(root, "key", &key, NULL);

		if (str_len(&ssid) <= 0 || str_len(&encrypt) <= 0) 
		{
			ems_l_trace("invalid arg");
			break;
		}

		if (br_chrole_to_ap(br, &ssid, &encrypt, &key) == EMS_OK) {
			ems_l_trace("we are in brigde mode now");
			ems_flag_set(emscorer()->flg, FLG_NETWORK_BRIDGE);
			ems_send_message(ty_bridge, ty_net,  EMS_APP_STOP, NULL);

			/* if client is running, stop it and other modules */
			//ems_send_message(ty_bridge, ty_client, EMS_APP_STOP, NULL);
			rtn = EMS_OK;
		}

	} while (0);

	str_uninit(&ssid);
	str_uninit(&encrypt);
	str_uninit(&key);

	if (rtn == EMS_OK)
		return br_change_status(br, st_hb);
	else
		ems_send_message(ty_bridge, ty_downlink, EMS_APP_START, NULL);

	return rtn;
}

static ems_int br_rsp_hb(ems_bridge *br, ems_session *sess, json_object *root)
{
	return br_change_status(br, st_hb);
}

static ems_int 
br_msg_normal_rsp(ems_bridge *br, ems_session *sess, ems_response *rsp, json_object *root)
{
	sess_timeout_cancel(sess);

	if (rsp->st != EMS_OK)
		return EMS_ERR;

	switch(rsp->tag.msg & 0x0000ffff) {

	case CMD_BRIDGE_REQ:
		return br_rsp_req(br, sess, root);

	case CMD_BRIDGE_HB:
		return br_rsp_hb(br, sess, root);

	default:
		break;
	}

	return EMS_ERR;
}

static ems_int br_normal(ems_bridge *br, ems_session *sess, ems_uint flg)
{
	if (ems_flag_like(flg, EMS_EVT_READ)) {
		if (br_recv_handle(br, sess, br_msg_normal_rsp) != EMS_OK) 
			return br_change_status(br, st_stopped);
	}

	if (ems_flag_like(flg, EMS_EVT_WRITE)) {
		if (br_send_msg(br, sess, flg)  != EMS_OK)
			return br_change_status(br, st_stopped);
	}

	return EMS_OK;
}

static ems_int br_br(ems_bridge *br, ems_session *sess, ems_uint flg)
{
	sess = br->sess;
	if (!sess) 
		return br_change_status(br, st_stopped);

	ems_l_trace("[br]session(%d) [%s: %d] down", 
		ems_sock_fd(&sess->sock),
		ems_sock_addr(&sess->sock),
		ems_sock_port(&sess->sock));

	sess_event_cancel(sess);
	ems_sock_close(&sess->sock);
	sess_timeout_set_sorted(sess, BR_HEARTBEAT_TIME, br_timeout_cb);

	return EMS_OK;
}

static ems_int br_err (ems_bridge *br, ems_session *sess, ems_uint flg)
{
	return br_change_status(br, st_stopped);
}

static ems_int br_to_connect(ems_bridge *br, ems_session *sess, ems_timeout *to)
{
	ems_l_trace("timeout to connect uplink");
	ems_send_message(ty_bridge, ty_downlink, EMS_APP_START, NULL);
	return br_change_status(br, st_stopped);
}

static ems_int br_to_normal (ems_bridge *br, ems_session *sess, ems_timeout *to)
{
	if (ems_flag_like(emscorer()->flg, FLG_NETWORK_BRIDGE))
		return br_change_status(br, st_hb);

	return br_change_status(br, st_stopped);
}

static ems_int br_to_br(ems_bridge *br, ems_session *sess, ems_timeout *to)
{
	ems_cchar *gw = NULL;
	ems_l_trace("[br] time to say hello");

	gw = ems_sock_addr(&sess->sock);
	if (!gw) {
		gw = cfg_get(emscfg(), CFG_lan_gw);
		if (gw) {
			ems_sock_setaddr(&sess->sock, gw);
			ems_sock_setport(&sess->sock, EMS_PORT);
		} else
			return br_change_status(br, st_hb);
	}

	if (br_connect_gw(sess) != EMS_OK)
		return br_change_status(br, st_stopped);

	sess_event_set(sess,          EMS_EVT_WRITE,         br_evt_cb);
	sess_timeout_set_sorted(sess, BR_CONNECT_GW_TIMEOUT, br_timeout_cb);

	if (ems_flag_like(sess->flg, EMS_FLG_ONLINE)) {
		core_pack_req(sess,  CMD_BRIDGE_HB);
		return br_change_status(br, st_normal);
	}

	return br_change_status(br, st_connect);
}

static ems_int br_to_err(ems_bridge *br, ems_session *sess, ems_timeout *to)
{
	return br_change_status(br, st_stopped);
}

ems_void ems_setwifi_nopassword()
{
	ems_cfg   *cfg = emscfg();
	ems_cchar *val = cfg_get(emscfg(), CFG_wireless_encrypt);

	if (val && !strcmp(val, "none"))
		return;

	br_set_wifi_info(cfg_get(cfg, CFG_wireless_ifname), 
			 wifi_mod_find("none"), 
			 cfg_get(cfg, CFG_wireless_ssid), 
			 NULL);
}

ems_int br_change_status(ems_bridge *br, ems_status st)
{
	if (br->st == st)
		return EMS_OK;

	ems_l_trace("[br] chnage status: %s +++> %s",
			ems_status_str(br->st), ems_status_str(st));

	br->st = st;

	switch(st) {

	case st_start:
	case st_stopped:
	case st_hb:
		return br_evt_run(br, NULL, 0);
		break;

	default:
		break;
	}

	return EMS_OK;
}

