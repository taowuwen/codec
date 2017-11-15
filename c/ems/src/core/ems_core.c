
#include "ems_core.h"
#include "ems_client.h"
#include "app.h"
#include "ems_netcheck.h"
#include "ems_bridge.h"
#include "ems_fw.h"

#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>

static jmp_buf	jmpaddr;

ems_cchar *ems_popen_get(ems_cchar *fmt, ...)
{
	static ems_char res[512];
	FILE   *fp;
	va_list args;
	ems_buffer *buf = core_buffer();

	ems_char *p = buf_wr(buf);
	ems_int   l = buf_left(buf);

	va_start(args, fmt);
	vsnprintf(p, l, fmt, args);
	va_end(args);

	memset(res, 0, sizeof(res));
	fp = popen(p, "r");
	if (!fp)
		return NULL;

	fgets(res, sizeof(res), fp);

	ems_trim(res);

	pclose(fp);

	ems_l_trace("\033[00;32m cmd: %s, result: %s\033[00m", p, res);
	return res;
}

ems_int ems_systemcmd(ems_cchar *fmt, ...)
{
	va_list args;
	ems_buffer *buf = core_buffer();

	ems_char *p = buf_wr(buf);
	ems_int   l = buf_left(buf);

	va_start(args, fmt);
	vsnprintf(p, l, fmt, args);
	va_end(args);

	ems_l_trace("\033[00;31m >>>>>>>: %s \033[00m", p);

	if (system(p) != EMS_OK)
		return errno;

	return EMS_OK;
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

static ems_int ems_core_modules_load(ems_core *core)
{
	ems_int   rtn;
	app_module **mod;


	mod =
	core->_core_module = ems_app_module_array();
	if (!core->_core_module)
		return EMS_ERR;

	mod = core->_core_module;
	do {
		rtn = EMS_ERR;

		if (ems_app_module_load(mod, ty_net,    NULL) != EMS_OK) break;
		if (ems_app_module_load(mod, ty_client, NULL) != EMS_OK) break;
		if (ems_app_module_load(mod, ty_nic,    NULL) != EMS_OK) break;
		if (ems_app_module_load(mod, ty_g_fw,   NULL) != EMS_OK) break;
		if (ems_app_module_load(mod, ty_ctrl,   NULL) != EMS_OK) break;
		if (ems_app_module_load(mod, ty_tunnel, NULL) != EMS_OK) break;

		/* no need support bridge and downlink from now on */
		//if (ems_app_module_load(mod, ty_bridge)   != EMS_OK) break;
		//if (ems_app_module_load(mod, ty_downlink)   != EMS_OK) break;

		rtn = EMS_OK;
	} while (0);

	return rtn;
}

static ems_int ems_core_modules_unload(ems_core *core)
{
	ems_app_module_array_destroy(core->_core_module);
	core->_core_module = NULL;

	return EMS_OK;
}

static ems_void ems_core_clear_network_info(ems_core *core)
{
	str_uninit(&core->net.lan.ifname);
	str_uninit(&core->net.lan.mac);
	str_uninit(&core->net.lan.ip);
	str_uninit(&core->net.lan.mask);

	str_uninit(&core->net.wan.ifname);
	str_uninit(&core->net.wan.mac);
	str_uninit(&core->net.wan.ip);
	str_uninit(&core->net.wan.mask);
	str_uninit(&core->net.wan.gw);
}


ems_int ems_core_init(ems_core *core)
{
	ems_assert(core);

	memset(core, 0, sizeof(ems_core));

	ems_buffer_init(&core->buf, EMS_BUFFER_8k);
	{ /* for net */
		
		str_init(&core->net.lan.ifname);
		str_init(&core->net.lan.mac);
		str_init(&core->net.lan.ip);
		str_init(&core->net.lan.mask);

		str_init(&core->net.wan.ifname);
		str_init(&core->net.wan.mac);
		str_init(&core->net.wan.ip);
		str_init(&core->net.wan.mask);
		str_init(&core->net.wan.gw);
	}
	{ /* for device info  */
		str_init(&core->dev.mac);
		str_init(&core->dev.ty);
		str_init(&core->dev.sn);
		str_init(&core->dev.ver);
	}

	core->_core_module = NULL;

	ems_queue_init(&core->msg_entry);

	ems_event_init(&core->evt, EVT_DRIVER_EPOLL);
	core->flg = 0;
	ems_queue_init(&core->app_entry);

#ifndef DEBUG
	cfg_init(&core->cfg, "/usr/ems/conf/ems.cfg");
#else
	cfg_init(&core->cfg, "/tmp/ems/conf/ems.cfg");
#endif

	ems_core_modules_load(core);

	return EMS_OK;
}

ems_int ems_core_uninit(ems_core *core)
{
	ems_core_modules_unload(core);

	ems_event_done(&core->evt);
	cfg_uninit(&core->cfg);
	ems_queue_clear(&core->app_entry, ems_app, entry, ems_app_destroy);
	ems_buffer_uninit(&core->buf);

	ems_core_clear_network_info(core);

	{ /* for device info  */
		str_uninit(&core->dev.mac);
		str_uninit(&core->dev.ty);
		str_uninit(&core->dev.sn);
		str_uninit(&core->dev.ver);
	}

	ems_queue_clear(&core->msg_entry, msgqueue, entry, ems_mq_destroy);
	ems_queue_init(&core->msg_entry);

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

	tag = (EMS_MODULE_AC << 16) | tag;

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

static ems_cchar *_cmd_prefix = "uci get -p /tmp/state ";

ems_cchar *ems_uci_get(ems_cchar *cfg)
{
	return ems_popen_get(ems_strcat(_cmd_prefix, cfg));
}

static ems_int ems_net_flush_ip(ems_cchar *ifname, ems_str *str, ems_cchar *cfg)
{
	if (cfg)
		str_set(str, ems_popen_get(ems_strcat(_cmd_prefix, cfg)));

	if (str_len(str) <= 0) {
		str_set(str, ems_popen_get(
			"ip addr show dev %s | awk '/inet/ { gsub(\"/.*\", \"\", $2); print $2}'", ifname));
	}

	return str_len(str) > 0?EMS_OK:EMS_ERR;
}

static ems_int ems_net_flush_mask(ems_cchar *ifname, ems_str *str, ems_cchar *cfg)
{
	if (cfg)
		str_set(str, ems_popen_get(ems_strcat(_cmd_prefix, cfg)));

	if (str_len(str) <= 0) {
		str_set(str, ems_popen_get(
			"ifconfig %s | sed -n '/inet/ {s/^.*Mask://g; p}'", ifname));
	}

	return str_len(str) > 0?EMS_OK:EMS_ERR;
}

static ems_int ems_net_flush_mac(ems_cchar *ifname, ems_str *str, ems_cchar *cfg)
{
	if (cfg)
		str_set(str, ems_popen_get(ems_strcat(_cmd_prefix, cfg)));

	if (str_len(str) <= 0) {
		str_set(str, ems_popen_get(
			"ip addr show dev %s | awk '/link/ {print $2}'", ifname));
	}

	return str_len(str) > 0?EMS_OK:EMS_ERR;
}

static ems_void ems_core_flush_lan(ems_core *core)
{
	ems_cchar *ifname = NULL;

	str_set(&core->net.lan.ifname, ems_popen_get(ems_strcat(_cmd_prefix, "network.lan.ifname")));
	if (str_len(&core->net.lan.ifname) <= 0) {
		ems_l_warn("[core] get lan ifname failed, reset ifname br-lan");
		str_set(&core->net.lan.ifname, "br-lan");
	}

	ifname = str_text(&core->net.lan.ifname);

	ems_net_flush_ip(ifname,   &core->net.lan.ip,   "network.lan.ipaddr");
	ems_net_flush_mask(ifname, &core->net.lan.mask, "network.lan.netmask");
	ems_net_flush_mac(ifname,  &core->net.lan.mac,  "network.lan.macaddr");
}

static ems_void ems_core_flush_wan(ems_core *core)
{
	ems_cchar *ifname = NULL;

	str_set(&core->net.wan.ifname, ems_popen_get(ems_strcat(_cmd_prefix, "network.wan.ifname")));
	if (str_len(&core->net.wan.ifname) <= 0) {
		ems_l_warn("[core] get wan ifname failed, reset ifname = eth0.2");
		str_set(&core->net.wan.ifname, "eth0.2");
	}

	ifname = str_text(&core->net.wan.ifname);

	ems_net_flush_ip(ifname,   &core->net.wan.ip,  "network.wan.ipaddr");
	ems_net_flush_mask(ifname, &core->net.wan.mask,"network.wan.netmask");
	ems_net_flush_mac(ifname,  &core->net.wan.mac, "network.wan.macaddr");
	str_set(&core->net.wan.gw, ems_popen_get("ip route | awk '/default/ {print $3; exit}'"));
}

ems_int ems_flush_system_info(ems_core *core)
{
	ems_l_trace("[core] flush system network info");
	ems_core_flush_lan(core);
	ems_core_flush_wan(core);

	ems_l_info("network [lan %s] %s/%s %s", 
			str_text(&core->net.lan.ifname),
			str_text(&core->net.lan.ip),
			str_text(&core->net.lan.mask),
			str_text(&core->net.lan.mac));

	ems_l_info("network [wan %s] %s/%s %s via %s", 
			str_text(&core->net.wan.ifname),
			str_text(&core->net.wan.ip),
			str_text(&core->net.wan.mask),
			str_text(&core->net.wan.mac),
			str_text(&core->net.wan.gw));

	return EMS_OK;
}

ems_cchar *ssid_suffix(ems_cchar *ssid)
{
	ems_cchar *p = NULL;

	if (ssid)
		p = strchr(ssid, '_');

	return (p != NULL?p:"_yingke");
}

static ems_cchar *ems_apmac()
{
	return ems_popen_get(
		"hexdump -s 0x28 -n 6 -C /dev/mtd2 | awk '{printf(\"%%02s:%%02s:%%02s:%%02s:%%02s:%%02s\", $2, $3, $4, $5, $6, $7); exit}'");
}

static ems_int ems_valid_sn(ems_char *sn)
{
	ems_char   c;

	while (*sn) {
		c = *sn++;

		if (!isalnum(c))
			return EMS_NO;
	}

	return EMS_YES;
}

static ems_char *ems_apsn()
{
	static ems_char sn[32] = {0};
	ems_char        buf[64] = {0};

	snprintf(buf, sizeof(buf), "%s", 
		ems_popen_get("hexdump -s 0x100 -n 16 -C /dev/mtd2 | awk '{print $2$3$4$5$6$7$8$9$10$11$12$13$14$15$16$17; exit}'"));
	memset(sn, 0, sizeof(sn));
	ems_str2bin(buf, sn, 32);

	if (!ems_valid_sn(sn)) {
		snprintf(sn, sizeof(sn), "%s", 
			ems_popen_get("hexdump -s 0x28 -n 6 -C /dev/mtd2 | awk '{print $2$3$4$5$6$7; exit}'"));
	}

	return sn;
}

static ems_char *ems_devicetype()
{
	static ems_char buf[32];
	snprintf(buf, 32, "%s", ems_popen_get("cat /proc/cpuinfo |grep machine | cut -d: -f2-"));
	return buf;
}

static ems_cchar *ems_sys_version()
{
	return ems_popen_get("cat /etc/lkwifi2000_ver");
///usr/lib/lua/luci/version.lua | grep luciversion | awk -F\\\" '{print $2}'");
}


ems_int core_cfg_init(ems_core *core)
{
	ems_cfg *cfg = &core->cfg;

	cfg_read(cfg);

	if (!cfg_get(cfg, CFG_ems_s_addr))
		cfg_set(cfg, CFG_ems_s_addr, "udc.yingke.com");

	if (!cfg_get(cfg, CFG_ems_s_port))
		cfg_set(cfg, CFG_ems_s_port, "80");

	if (cfg_get(cfg, CFG_log_level))
		ems_logger_set_level(logger(), ems_atoi(cfg_get(cfg, CFG_log_level)));
	else
		cfg_set(cfg, CFG_log_level, ems_itoa(ems_logger_level(logger())));

	str_set(&core->dev.sn,  ems_apsn());
	str_set(&core->dev.mac, ems_apmac());
	str_set(&core->dev.ver, ems_sys_version());
	str_set(&core->dev.ty,  ems_devicetype());

	cfg_write(cfg);

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

ems_void ems_mq_destroy(msgqueue *mq)
{
	if (mq) {
		if (mq->obj) {
			json_object_put(mq->obj);
			mq->obj = NULL;
		}

		ems_free(mq);
	}
}

ems_int _ems_send_message(ems_queue *list, ems_uint s, ems_uint d, ems_uint evt, json_object *obj)
{
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

	ems_queue_insert_tail(list, &mq->entry);

	return EMS_OK;
}

static ems_void core_evt_cb(ems_event *evt)
{
	ems_queue *p;
	msgqueue  *mq;
	ems_core  *core = emscorer();

	while (!ems_queue_empty(&core->msg_entry)) {

		p = ems_queue_head(&core->msg_entry);
		ems_queue_remove(p);

		mq = ems_container_of(p, msgqueue, entry);

		ems_app_process(mq->s, mq->d, mq->evt, mq->obj);
		ems_mq_destroy(mq);
	}

	ems_app_process(ty_ctrl, ty_nic, EMS_APP_PROCESS_MSGQUEUE, NULL);
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

		ems_flush_system_info(core);

		if (ems_atoi(ems_popen_get("uci get -P /tmp/state ykwifi.base.first_config"))) {
			ems_flag_set(core->flg, FLG_FIRST_CONFIG);
		}

		if (ems_atoi(cfg_get(emscfg(), CFG_client_subdomain_enable)))
			ems_flag_set(core->flg, FLG_SUBDOMAIN_ENABLE);

		ems_app_process(ty_ctrl, ty_net,    EMS_APP_START, NULL);
		ems_app_process(ty_ctrl, ty_ctrl,   EMS_APP_START, NULL);
		ems_app_process(ty_ctrl, ty_nic,    EMS_APP_START, NULL);
		ems_app_process(ty_ctrl, ty_client, EMS_APP_START, NULL);
		ems_app_process(ty_ctrl, ty_tunnel, EMS_APP_START, NULL);


		if (!setjmp(jmpaddr)) {
			ems_event_run(&core->evt, core_evt_cb);
		} else
			ems_l_trace("aaa got signal, do exit");

		ems_app_modules_run(core_moduler(), EMS_NO);
	} while (0);

	return EMS_OK;
}
