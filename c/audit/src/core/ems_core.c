
#include "ems_core.h"
#include "ems_client.h"
#include "class.h"
#include "audit.h"

#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/stat.h>

static jmp_buf	jmpaddr;
typedef struct _msg_queue_s  msgqueue;

struct _msg_queue_s {
	ems_uint     s;
	ems_uint     d;
	ems_uint     evt;
	json_object *obj;
	ems_queue    entry;
};

static ems_void ems_mq_destroy(msgqueue *mq);



ems_cchar *ems_popen_get(ems_cchar *cmd)
{
	static ems_char buf[512];
	FILE  *fp;

	memset(buf, 0, sizeof(buf));
	fp = popen(cmd, "r");
	if (!fp)
		return NULL;

	fgets(buf, sizeof(buf), fp);

	ems_trim(buf);

	pclose(fp);

	ems_l_trace("\033[00;32m cmd: %s, result: %s\033[00m", cmd, buf);
	return buf;
}

ems_int ems_systemcmd(ems_cchar *cmd)
{
	ems_assert(cmd);
	if (!cmd)
		return EMS_ERR;

	ems_l_trace("\033[00;31m >>>>>>>: %s \033[00m", cmd);

	return system(cmd);
}

static ems_void ems_sighandler(ems_int sig)
{
	fprintf(stdout, "signal got %d\n", sig);
	longjmp(jmpaddr,1);
}


json_object *ems_json_tokener_parse(ems_cchar *str)
{
	enum json_tokener_error err;
	struct json_object *root;

	ems_assert(str != NULL);
	if (!str)
		return NULL;

	root = json_tokener_parse_verbose(str, &err);

	if (err != json_tokener_success) {
		ems_l_warn("<< \033[01;32m JSON ERROR: %s:%s \033[00m>>>>",
			str, json_tokener_error_desc(err));
		return NULL;
	}

	return root;
}

ems_int ems_core_init(ems_core *core)
{
	ems_assert(core);

	memset(core, 0, sizeof(ems_core));

	ems_buffer_init(&core->buf, EMS_BUFFER_8k * 8);
	str_init(&core->gw);
	str_init(&core->ifname);
	str_init(&core->ac_mac);
	str_init(&core->portal);
	str_init(&core->ssid);
	str_init(&core->devty);
	str_init(&core->sn);
	core->portal_redirect_port = 0;

	ems_queue_init(&core->msg_entry);
	ems_mtx_init(core->msg_mtx);

	ems_event_init(&core->evt, EVT_DRIVER_EPOLL);
	core->flg = 0;
	ems_queue_init(&core->app_entry);

#ifndef DEBUG
	cfg_init(&core->cfg, "/etc/audit.cfg");
#else
	cfg_init(&core->cfg, "./audit.cfg");
#endif

	return EMS_OK;
}

ems_int ems_core_uninit(ems_core *core)
{
	ems_event_done(&core->evt);
	cfg_uninit(&core->cfg);
	ems_buffer_uninit(&core->buf);

	str_uninit(&core->gw);
	str_uninit(&core->ifname);
	str_uninit(&core->ac_mac);
	str_uninit(&core->portal);
	str_uninit(&core->ssid);
	str_uninit(&core->devty);
	str_uninit(&core->sn);

	ems_queue_clear(&core->msg_entry, msgqueue, entry, ems_mq_destroy);
	ems_queue_init(&core->msg_entry);
	ems_mtx_destroy(core->msg_mtx);

	return EMS_OK;
}

ems_int core_pack_req(ems_session *sess, ems_uint tag)
{
	ems_cchar *ctx = NULL;
	ems_int   len, rtn;

	json_object *root = (json_object *)sess_request(sess);

	ctx = NULL;
	len = 0;
	if (root) {
		ctx = json_object_to_json_string(root);
		len = ems_strlen(ctx);
	}

	tag = (EMS_MODULE_AUDIT << 16) | tag;

	ems_l_trace("\033[01;33m <req tag: 0x%x, ctx: %s> \033[00m",
			tag , ctx?ctx:"null");

	rtn = ems_pack_req(tag, ctx, len, &sess->buf);

	if (root) {
		json_object_put(root);
		sess_request_set(sess, NULL);
	}

	return rtn;

}

ems_int core_pack_rsp(ems_session *sess, ems_uint tag, ems_int st)
{
	ems_cchar *ctx = NULL;
	ems_int   len, rtn;

	json_object *root = (json_object *)sess_response(sess);

	ctx = NULL;
	len = 0;
	if (root) {
		ctx = json_object_to_json_string(root);
		len = ems_strlen(ctx);
	}

	ems_l_trace("\033[01;34m<rsp tag: 0x%x, st: %d ctx(0x%x): %s> \033[00m",
			tag | 0x80000000, st, len, 
				(len < 1024)?(ctx?ctx:"no ctx"):"******TOO LONG *****");

	rtn = ems_pack_rsp(tag, st, ctx, len, &sess->buf);

	if (root) {
		json_object_put(root);
		sess_response_set(sess, NULL);
	}

	return rtn;
}

ems_cchar *ems_strcat(ems_cchar *s1, ems_cchar *s2)
{
	static ems_char buf[512] = {0};

	snprintf(buf, 512, "%s%s 2&>/dev/null", s1, s2);
	return buf;
}

ems_int ems_flush_system_info()
{
	ems_cfg   *cfg = emscfg();
	ems_char   buf[256] = {0};
	ems_cchar *ifname = NULL;
	ems_cchar *cmd = "uci get -P /tmp/state ";

	cfg_set(cfg, CFG_lan_ifname, ems_popen_get(ems_strcat(cmd, CFG_lan_ifname)));
	cfg_set(cfg, CFG_lan_proto,  ems_popen_get(ems_strcat(cmd, CFG_lan_proto)));
	cfg_set(cfg, CFG_lan_addr,   ems_popen_get(ems_strcat(cmd, CFG_lan_addr)));
	cfg_set(cfg, CFG_lan_mask,   ems_popen_get(ems_strcat(cmd, CFG_lan_mask)));
	cfg_set(cfg, CFG_lan_gw,     ems_popen_get(ems_strcat(cmd, CFG_lan_gw)));

	ifname = cfg_get(cfg, CFG_lan_ifname);
	if (!cfg_get(cfg, CFG_lan_addr)) {
		snprintf(buf, sizeof(buf), 
			"ifconfig %s | grep inet | awk '{print $2}' | cut -d: -f2-", ifname);
		cfg_set(cfg, CFG_lan_addr, ems_popen_get(buf));
	}

	if (!cfg_get(cfg, CFG_lan_mask)) {
		snprintf(buf, sizeof(buf), 
			"ifconfig %s | grep inet | awk '{print $4}' | cut -d: -f2-", ifname);
		cfg_set(cfg, CFG_lan_mask, ems_popen_get(buf));
	}

	if (!cfg_get(cfg, CFG_lan_gw)) {
		snprintf(buf, sizeof(buf), 
			"ip route | grep 'default*.*%s' | awk '{print $3}'", ifname);
		cfg_set(cfg, CFG_lan_gw, ems_popen_get(buf));
	}

	cfg_set(cfg, CFG_wan_ifname, ems_popen_get(ems_strcat(cmd, CFG_wan_ifname)));
#if 0
	cfg_set(cfg, CFG_wan_addr,   ems_popen_get(ems_strcat(cmd, CFG_wan_addr)));
	cfg_set(cfg, CFG_wan_mask,   ems_popen_get(ems_strcat(cmd, CFG_wan_mask)));
	cfg_set(cfg, CFG_wan_gw,     ems_popen_get(ems_strcat(cmd, CFG_wan_gw)));
#endif
	cfg_set(cfg, CFG_wan_proto,  ems_popen_get(ems_strcat(cmd, CFG_wan_proto)));

	ifname = cfg_get(cfg, CFG_wan_ifname);
#if 0
	if (!cfg_get(cfg, CFG_wan_addr)) 
#endif
	{
		snprintf(buf, sizeof(buf), 
			"ifconfig %s | grep inet | awk '{print $2}' | cut -d: -f2-", ifname);
		cfg_set(cfg, CFG_wan_addr, ems_popen_get(buf));
	}

#if 0
	if (!cfg_get(cfg, CFG_wan_mask)) 
#endif
	{
		snprintf(buf, sizeof(buf), 
			"ifconfig %s | grep inet | awk '{print $4}' | cut -d: -f2-", ifname);
		cfg_set(cfg, CFG_wan_mask, ems_popen_get(buf));
	}

#if 0
	if (!cfg_get(cfg, CFG_wan_gw)) 
#endif
	{
		snprintf(buf, sizeof(buf), 
			"ip route | grep 'default*.*%s' | awk '{print $3}'", ifname);
		cfg_set(cfg, CFG_wan_gw, ems_popen_get(buf));
	}

	core_wireless_info();

	return EMS_OK;
}

ems_int core_wireless_info()
{
	ems_cfg   *cfg = emscfg();
	ems_cchar *cmd = "uci get -P /tmp/state ";

	cfg_set(cfg, CFG_wireless_ifname,  ems_popen_get(ems_strcat(cmd, CFG_wireless_ifname)));
	cfg_set(cfg, CFG_wireless_ssid,    ems_popen_get(ems_strcat(cmd, CFG_wireless_ssid)));
	cfg_set(cfg, CFG_wireless_encrypt, ems_popen_get(ems_strcat(cmd, CFG_wireless_encrypt)));
	cfg_set(cfg, CFG_wireless_key,     ems_popen_get(ems_strcat(cmd, CFG_wireless_key)));
	{
		ems_cchar *val = cfg_get(cfg, CFG_wireless_encrypt);

		if (val && strstr(val, "wep")) {
			ems_char   buf[256] = {0};
			snprintf(buf, sizeof(buf), "%s %s1", cmd, CFG_wireless_key);
			cfg_set(cfg, CFG_wireless_key, ems_popen_get(buf));

			snprintf(buf, sizeof(buf), "%s", cfg_get(cfg, CFG_wireless_key));
			ems_l_trace("ori key: %s", buf);

			if (!strncmp("s:", buf, 2))
				cfg_set(cfg, CFG_wireless_key, buf + 2);
		}
	}

	return EMS_OK;
}

#if 0
static ems_char *ems_apmac()
{
	ems_cchar *cmd = "hexdump -s 0x28 -n 6 -C /dev/mtd2 | head -1 | awk '{printf(\"%02s:%02s:%02s:%02s:%02s:%02s\", $2, $3, $4, $5, $6, $7)}'";
	static ems_char buf[32];

	snprintf(buf, sizeof(buf), "%s", ems_popen_get(cmd));

	return buf;
}
#endif

ems_int core_cfg_init(ems_core *core)
{
	ems_cfg *cfg = &core->cfg;

	cfg_read(cfg);
#if 0
	if (!cfg_get(cfg, CFG_ems_c_addr))
		cfg_set(cfg, CFG_ems_c_addr, EMS_ADDR);

	if (!cfg_get(cfg, CFG_ems_c_port))
		cfg_set(cfg, CFG_ems_c_port, ems_itoa(EMS_PORT));
#endif

	if (cfg_get(cfg, CFG_log_level))
		ems_logger_set_level(logger(), ems_atoi(cfg_get(cfg, CFG_log_level)));

	return EMS_OK;
}

ems_int core_getargs(ems_core *core, ems_int argc, ems_char **argv)
{
	ems_int c;

	while ((c = getopt(argc, argv, "c:")) != -1) {

		switch(c) {

		case 'c':
			str_set(&core->cfg.fl, optarg);
			break;
		default:
			ems_l_error("usage: %s -c config file\n", argv[0]);
			return EMS_ERR;
		}
	}

	return core_cfg_init(core);
}

static msgqueue  *ems_mq_new()
{
	msgqueue *mq = (msgqueue *)ems_malloc(sizeof(msgqueue));

	if (mq) {
		memset(mq, 0, sizeof(msgqueue));
		ems_queue_init(&mq->entry);
	}

	return mq;
}

static ems_void ems_mq_destroy(msgqueue *mq)
{
	if (mq) {
		if (mq->obj) {
			json_object_put(mq->obj);
			mq->obj = NULL;
		}

		ems_free(mq);
	}
}

#ifdef DEBUG
extern ems_cchar *ems_app_desc(ems_uint ty);
extern ems_cchar *ems_evt_desc(ems_uint evt);
#endif

ems_int audit_send_json(ems_uint s, ems_uint d, ems_uint evt, json_object *obj)
{
	ems_core *core = emscorer();
	msgqueue *mq = ems_mq_new();

	if (!mq)
		return EMS_ERR;

	mq->s   = s;
	mq->d   = d;
	mq->evt = evt;
	mq->obj = NULL;
	if (obj) {
		mq->obj = ems_json_tokener_parse(json_object_to_json_string(obj));
		if (!mq->obj) {
			ems_mq_destroy(mq);
			return EMS_ERR;
		}
	}

	ems_queue_insert_tail(&core->msg_entry, &mq->entry);

	return EMS_OK;
}

static ems_void core_evt_cb(ems_event *evt)
{
	ems_queue *p;
	msgqueue  *mq;
	ems_core  *core = emscorer();
#ifdef DEBUG
	static ems_uint cnt = 0;

	ems_l_trace("evt cb, cnt: %u", ++cnt);
#endif

	while (!ems_queue_empty(&core->msg_entry)) {

		p = ems_queue_head(&core->msg_entry);
		ems_queue_remove(p);

		mq = ems_container_of(p, msgqueue, entry);

		audit_sendmsg(mq->s, mq->d, mq->evt, (ems_uchar *)mq->obj);
		ems_mq_destroy(mq);
	}
}

ems_int ems_core_main(ems_core *core, ems_int argc, ems_char **argv)
{
	srandom(time(NULL));

	signal(SIGINT,  ems_sighandler);
	signal(SIGABRT, ems_sighandler);
	signal(SIGKILL, ems_sighandler);
	signal(SIGSTOP, ems_sighandler);
	signal(SIGTERM, ems_sighandler);

	do {
		if (core_getargs(core, argc, argv) != EMS_OK)
			break;

		audit_modules_init();

		if (audit_sendmsg(0, mod_ctrl, A_AUDIT_START, NULL) == EMS_OK) 
		{
			if (!setjmp(jmpaddr)) {
				ems_event_run(&core->evt, core_evt_cb);
			} else
				ems_l_trace("aaa got signal, do exit");
		}

		audit_sendmsg(0, mod_ctrl, A_AUDIT_STOP, NULL);
		audit_modules_uninit();

	} while (0);

	return EMS_OK;
}
