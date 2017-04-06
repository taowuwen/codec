
#include "ems_core.h"
#include "ems_client.h"
#include "ems_fw.h"
#include "app.h"
#include "ems_netcheck.h"
#include "ems_cmd.h"

static ems_int 
ems_cmd_c_set(ems_core *core, ems_session *sess, json_object *req)
{
	ems_int  port;
	ems_str  buf;
	ems_int  restart = EMS_NO;

	str_init(&buf);

	ems_json_get_int(req,    "port", port);
	ems_json_get_string(req, "addr", &buf);

	if (str_len(&buf) <= 0)
		return MSG_ST_INVALID_ARG;

	if (strcmp(cfg_get(emscfg(), CFG_ems_s_addr), str_text(&buf))) {
		cfg_set(emscfg(), CFG_ems_s_addr, str_text(&buf));
		restart = EMS_YES;
	}

	if (ems_atoi(cfg_get(emscfg(), CFG_ems_s_port)) != port ) {
		cfg_set(emscfg(), CFG_ems_s_port, ems_itoa(port));
		restart = EMS_YES;
	}

#ifdef DEBUG
	ems_json_get_string_def(req, "sn", &buf, NULL);
	if (str_len(&buf) > 0)
		cfg_set(emscfg(), CFG_ems_sn, str_text(&buf));
#endif
	if (restart) {
		cfg_write(emscfg());
		ems_send_message(ty_ctrl, ty_client, EMS_APP_STOP,  NULL);
		ems_send_message(ty_ctrl, ty_client, EMS_APP_START, NULL);
	}

	str_clear(&buf);

	return EMS_OK;
}

static ems_int 
ems_cmd_c_get(ems_core *core, ems_session *sess, json_object *req)
{
	json_object *rsp;
	ems_cfg     *cfg = emscfg();


	rsp = json_object_new_object();

	ems_assert(cfg_get(cfg, CFG_ems_s_addr) && cfg_get(cfg, CFG_ems_s_port));

	json_object_object_add(rsp, "addr", 
			json_object_new_string(cfg_get(cfg, CFG_ems_s_addr)));
	json_object_object_add(rsp, "port", 
			json_object_new_int(ems_atoi(cfg_get(cfg, CFG_ems_s_port))));
	
	sess_response_set(sess, rsp);

	return EMS_OK;
}


ems_int ems_cmd_c(ems_core *core, ems_session *sess, json_object *req)
{
	json_object *obj;

	obj = json_object_object_get(req, "method");

	if (!(obj && json_object_is_type(obj, json_type_string)))
		return MSG_ST_INVALID_ARG;

	if (!strcasecmp(json_object_get_string(obj), "set")) 
		return ems_cmd_c_set(core, sess, req);

	return ems_cmd_c_get(core, sess, req);
}

#if 0
static ems_int 
ems_cmd_ctrl_set(ems_core *core, ems_session *sess, json_object *req)
{
	ems_int auto_reg;
	ems_int run;
	ems_uint flg;

	ems_json_get_int_def(req, "auto", auto_reg, 1);
	ems_json_get_int(req,     "run",  run);

	if (auto_reg)
		ems_flag_set(core->flg, FLG_AUTO_REG);
	else
		ems_flag_unset(core->flg, FLG_AUTO_REG);

	if (ems_atoi(cfg_get(emscfg(), CFG_ems_c_auto)) != auto_reg) {
		cfg_set(emscfg(), CFG_ems_c_auto, ems_itoa(auto_reg));
	}

	flg = core->flg;

	if (run)
		ems_flag_set(flg, FLG_RUN);
	else 
		ems_flag_unset(flg, FLG_RUN);

	return (flg == core->flg)? EMS_OK: clnt_run(core, run);
}

static ems_int 
ems_cmd_ctrl_get(ems_core *core, ems_session *sess, json_object *req)
{
	ems_int  tmp;
	json_object *rsp;

	rsp = json_object_new_object();

	tmp = EMS_YES;
	if (ems_flag_unlike(core->flg, FLG_AUTO_REG))
		tmp = EMS_NO;

	json_object_object_add(rsp, "auto", json_object_new_int(tmp));

	tmp = EMS_YES;
	if (ems_flag_unlike(core->flg, FLG_RUN))
		tmp = EMS_NO;

	json_object_object_add(rsp, "run", json_object_new_int(tmp));

	sess_response_set(sess, rsp);

	return EMS_OK;
}
#endif


ems_int ems_cmd_ctrl(ems_core *core, ems_session *sess, json_object *req)
{
	json_object *obj;

	obj = json_object_object_get(req, "method");

	if (!(obj && json_object_is_type(obj, json_type_string)))
		return MSG_ST_INVALID_ARG;

#if 0
	if (!strcasecmp(json_object_get_string(obj), "set")) 
		return ems_cmd_ctrl_set(core, sess, req);

	return ems_cmd_ctrl_get(core, sess, req);
#else
	return EMS_OK;
#endif
}


static ems_int 
ems_cmd_status_fill_system(ems_core *core, json_object *root)
{
	json_object *rsp, *net;

	ems_cfg     *cfg = emscfg();

	rsp = json_object_new_object();

	json_object_object_add(rsp, "sn",  json_object_new_string(cfg_get(cfg, CFG_ems_sn)));
	json_object_object_add(rsp, "mac", json_object_new_string(cfg_get(cfg, CFG_ems_mac)));

	net = json_object_new_object();
	{
		json_object_object_add(net, "st", 
			json_object_new_string(ems_flag_like(core->flg, FLG_NETWORK_READY)?"up":"down"));

		json_object_object_add(net, "bridge", 
			json_object_new_string(ems_flag_like(core->flg, FLG_NETWORK_BRIDGE)?"yes":"no"));
	}

	json_object_object_add(rsp, "network_status", net);
	json_object_object_add(root, "system", rsp);

	return EMS_OK;
}


ems_int ems_cmd_status(ems_core *core, ems_session *sess, json_object *req)
{
	ems_uint flg;
	json_object *rsp;

	ems_json_get_int_def(req, "flag", flg, 0x01);

#define flg_system 0x01<<0
#define flg_ems    0x01<<1

	rsp = json_object_new_object();
	if (ems_flag_like(flg, flg_system))
		ems_cmd_status_fill_system(core, rsp);

	if (ems_flag_like(flg, flg_ems))
		ems_app_process(ty_ctrl, ty_client, EMS_APP_EMS_STATUS, rsp);

	sess_response_set(sess, rsp);

	return EMS_OK;
}


static ems_int ems_cmd_bwlist_set(ems_core *core, ems_session *sess, json_object *req)
{
	ems_uint     flg;
	json_object *obj;

	ems_json_get_int(req, "flag", flg);

	if (ems_flag_like(flg, 0x01 << 0)) {
		obj = json_object_object_get(req, "url");
		if (obj && json_object_is_type(obj, json_type_array)) {
			cfg_set_json(emscfg(), CFG_user_url_white, obj);
		} 
	}

	if (ems_flag_like(flg, 0x01 << 1)) {
		obj = json_object_object_get(req, "whitemac");
		if (obj && json_object_is_type(obj, json_type_array)) {
			cfg_set_json(emscfg(), CFG_user_mac_white, obj);
		}
	}

	if (ems_flag_like(flg, 0x01 << 2)) {
		obj = json_object_object_get(req, "blackmac");
		if (obj && json_object_is_type(obj, json_type_array)) {
			cfg_set_json(emscfg(), CFG_user_mac_black, obj);
		}
	}

	ems_app_process(ty_ctrl, ty_fw, EMS_APP_RULES_UPDATE, NULL);
	cfg_write(emscfg());

	return EMS_OK;
}


static ems_int ems_cmd_bwlist_get(ems_core *core, ems_session *sess, json_object *req)
{
	ems_uint flg;
	json_object *rsp;

	ems_json_get_int_def(req, "flag", flg, 0x07);

	rsp = json_object_new_object();

	if (ems_flag_like(flg, 0x01 << 0)) {
		json_object_object_add(rsp, "url",      cfg_get_json(emscfg(), CFG_user_url_white));
	}

	if (ems_flag_like(flg, 0x01 << 1)) {
		json_object_object_add(rsp, "whitemac", cfg_get_json(emscfg(), CFG_user_mac_white));
	}


	if (ems_flag_like(flg, 0x01 << 2)) {
		json_object_object_add(rsp, "blackmac", cfg_get_json(emscfg(), CFG_user_mac_black));
	}

	sess_response_set(sess, rsp);

	return EMS_OK;

}

ems_int ems_cmd_bwlist(ems_core *core, ems_session *sess, json_object *req)
{
	json_object *obj;

	obj = json_object_object_get(req, "method");

	if (!(obj && json_object_is_type(obj, json_type_string)))
		return MSG_ST_INVALID_ARG;

	if (!strcasecmp(json_object_get_string(obj), "set")) 
		return ems_cmd_bwlist_set(core, sess, req);

	return ems_cmd_bwlist_get(core, sess, req);
}

#if 0

static ems_int 
ems_cmd_qos_set(ems_core *core, ems_session *sess, json_object *req)
{
	ems_uint  enable;

	ems_json_get_int(req, "enable", enable);

	ems_l_trace("qos set into enable? %s", enable?"yes":"no");

	cfg_set(emscfg(), CFG_app_qos_enable, ems_itoa(enable));

	ems_app_process(ty_ctrl, ty_qos, EMS_APP_RULES_UPDATE, req);

	cfg_write(emscfg());

	return EMS_OK;
}

static ems_int 
ems_cmd_qos_get(ems_core *core, ems_session *sess, json_object *req)
{
	json_object *rsp;
	ems_cchar   *val;

	rsp = json_object_new_object();

	val = cfg_get(emscfg(), CFG_app_qos_enable);
	if (!val) {
		cfg_set(emscfg(), CFG_app_qos_enable, "1");
		val = cfg_get(emscfg(), CFG_app_qos_enable);
	}

	json_object_object_add(rsp, "enable",  json_object_new_int(ems_atoi(val)));

	sess_response_set(sess, rsp);

	return EMS_OK;
}
#endif

ems_int ems_cmd_qos(ems_core *core, ems_session *sess, json_object *req)
{
#if 0
	json_object *obj;

	obj = json_object_object_get(req, "method");

	if (!(obj && json_object_is_type(obj, json_type_string)))
		return MSG_ST_INVALID_ARG;

	if (!strcasecmp(json_object_get_string(obj), "set")) 
		return ems_cmd_qos_set(core, sess, req);

	return ems_cmd_qos_get(core, sess, req);
#else
	return EMS_ERR;
#endif
}


static ems_int
ems_cmd_portal_set(ems_core *core, ems_session *sess, json_object *req)
{
	ems_app_process(ty_ctrl, ty_portal, EMS_APP_RULES_UPDATE, req);
	cfg_write(emscfg());

	str_clear(&core->portal);
	core->portal_redirect_port = 0;

	return EMS_OK;
}

static ems_int
ems_cmd_portal_get(ems_core *core, ems_session *sess, json_object *req)
{
	json_object *rsp;
	ems_cchar   *val;
	ems_cfg     *cfg = emscfg();

	rsp = json_object_new_object();

	val = cfg_get(cfg, CFG_app_portal_auto);
	if (val)
		json_object_object_add(rsp, "auto", json_object_new_int(ems_atoi(val)));
	else
		json_object_object_add(rsp, "auto", json_object_new_int(1));

	val = cfg_get(cfg, CFG_app_portal_addr);
	if (val)
		json_object_object_add(rsp, "addr", json_object_new_string(val));

	val = cfg_get(cfg, CFG_app_portal_port);
	if (val)
		json_object_object_add(rsp, "port", json_object_new_int(ems_atoi(val)));

	val = cfg_get(cfg, CFG_app_portal_reg);
	if (val)
		json_object_object_add(rsp, "reg_period", json_object_new_int(ems_atoi(val)));

	val = cfg_get(cfg, CFG_app_portal_hb);
	if (val)
		json_object_object_add(rsp, "hb_period", json_object_new_int(ems_atoi(val)));

	val = cfg_get(cfg, CFG_app_portal_redirect);
	if (val)
		json_object_object_add(rsp, "redirect_port", json_object_new_int(ems_atoi(val)));

	sess_response_set(sess, rsp);

	return EMS_OK;
}

ems_int ems_cmd_portal(ems_core *core, ems_session *sess, json_object *req)
{
	json_object *obj;

	obj = json_object_object_get(req, "method");

	if (!(obj && json_object_is_type(obj, json_type_string)))
		return MSG_ST_INVALID_ARG;

	if (!strcasecmp(json_object_get_string(obj), "set")) 
		return ems_cmd_portal_set(core, sess, req);

	return ems_cmd_portal_get(core, sess, req);
}

static ems_int
ems_cmd_radius_set(ems_core *core, ems_session *sess, json_object *req)
{
	ems_app_process(ty_ctrl, ty_radius, EMS_APP_RULES_UPDATE, req);
	cfg_write(emscfg());
	return EMS_OK;
}

static ems_int
ems_cmd_radius_get(ems_core *core, ems_session *sess, json_object *req)
{
	json_object *rsp;
	ems_cchar   *val;
	ems_cfg     *cfg = emscfg();

	rsp = json_object_new_object();

	val = cfg_get(cfg, CFG_app_radius_auto);
	if (val)
		json_object_object_add(rsp, "auto", json_object_new_int(ems_atoi(val)));
	else
		json_object_object_add(rsp, "auto", json_object_new_int(1));

	val = cfg_get(cfg, CFG_app_radius_addr);
	if (val)
		json_object_object_add(rsp, "addr", json_object_new_string(val));

	val = cfg_get(cfg, CFG_app_radius_shared_key);
	if (val)
		json_object_object_add(rsp, "secret", json_object_new_string(val));

	val = cfg_get(cfg, CFG_app_radius_port_auth);
	if (val)
		json_object_object_add(rsp, "authport", json_object_new_int(ems_atoi(val)));

	val = cfg_get(cfg, CFG_app_radius_port_acct);
	if (val)
		json_object_object_add(rsp, "acctport", json_object_new_int(ems_atoi(val)));

	val = cfg_get(cfg, CFG_app_radius_report_period);
	if (val)
		json_object_object_add(rsp, "rp_period", json_object_new_int(ems_atoi(val)));

	val = cfg_get(cfg, CFG_app_radius_retry_times);
	if (val)
		json_object_object_add(rsp, "retry_times", json_object_new_int(ems_atoi(val)));
	else
		json_object_object_add(rsp, "retry_times", json_object_new_int(3));

	val = cfg_get(cfg, CFG_app_radius_retry_timeout);
	if (val)
		json_object_object_add(rsp, "retry_timeout", json_object_new_int(ems_atoi(val)));
	else
		json_object_object_add(rsp, "retry_timeout", json_object_new_int(5));

	sess_response_set(sess, rsp);

	return EMS_OK;
}

ems_int ems_cmd_radius(ems_core *core, ems_session *sess, json_object *req)
{
	json_object *obj;

	obj = json_object_object_get(req, "method");

	if (!(obj && json_object_is_type(obj, json_type_string)))
		return MSG_ST_INVALID_ARG;

	if (!strcasecmp(json_object_get_string(obj), "set"))
		return ems_cmd_radius_set(core, sess, req);

	return ems_cmd_radius_get(core, sess, req);
}

ems_int ems_cmd_bridge_req(ems_core *core, ems_session *sess, json_object *req)
{
	ems_int      rtn;
	json_object *rsp = NULL;
	ems_cchar   *val;
	ems_cfg     *cfg = emscfg();

	rsp = json_object_new_object();

	do {
		rtn = EMS_ERR;

		core_wireless_info();

		val = cfg_get(cfg, CFG_wireless_ssid);
		if (!val) break;
		json_object_object_add(rsp, "ssid", json_object_new_string(val));

		if (ems_atoi(cfg_get(cfg, CFG_wireless_enable_encrypt))) {
			val = cfg_get(cfg, CFG_wireless_encrypt);
			if (!val) break;

			json_object_object_add(rsp, "encryption", json_object_new_string(val));

			val = cfg_get(cfg, CFG_wireless_key);
			if (val) 
				json_object_object_add(rsp, "key", json_object_new_string(val));
		} else
			json_object_object_add(rsp, "encryption", json_object_new_string("none"));

		rtn = EMS_OK;
	} while (0);

	if (rtn != EMS_OK) {
		json_object_put(rsp);
		rsp = NULL;
	} else {
		sess_response_set(sess, rsp);
	}

	return rtn;
}

ems_int ems_cmd_bridge_hb(ems_core *core, ems_session *sess, json_object *req)
{
	json_object *obj;
	obj = json_object_new_object();

	json_object_object_add(obj, "ip",  
			json_object_new_string(ems_sock_addr(&sess->sock)));
	json_object_object_add(obj, "mac", 
			json_object_new_string(ems_usermac(ems_sock_addr(&sess->sock))));

	ems_app_process(ty_ctrl, ty_downlink, EMS_EVT_DOWNLINK_IN, obj);

	json_object_put(obj);

	return EMS_OK;
}

ems_int ems_cmd_test_radius(ems_core *core, ems_session *sess, json_object *req)
{
	json_object *obj;
	obj = json_object_object_get(req, "method");

	if (!(obj && json_object_is_type(obj, json_type_string)))
		return MSG_ST_INVALID_ARG;

	if (!strcasecmp(json_object_get_string(obj), "set")) 
		return ems_send_message(ty_portal, ty_radius, 0x0001, req);

	return ems_send_message(ty_portal, ty_radius, 0x0002, req);
}

static ems_int
ems_cmd_network_up(ems_core *core, ems_session *sess, json_object *req)
{
	if (ems_flag_like(core->flg, FLG_NETWORK_BRIDGE))
		return EMS_OK;

	if (ems_flag_like(core->flg, FLG_NETWORK_READY))
		return EMS_OK;

	ems_app_process(ty_ctrl, ty_net,    EMS_APP_STOP, NULL);
	ems_app_process(ty_ctrl, ty_portal, EMS_APP_STOP, NULL);

	ems_app_process(ty_ctrl,  ty_net,    EMS_APP_START, NULL);
	ems_send_message(ty_ctrl, ty_portal, EMS_APP_START, NULL);

	return EMS_OK;
}

ems_int ems_cmd_network(ems_core *core, ems_session *sess, json_object *req)
{
	json_object *obj;
	obj = json_object_object_get(req, "method");

	if (!(obj && json_object_is_type(obj, json_type_string)))
		return MSG_ST_INVALID_ARG;

	if (!strcasecmp(json_object_get_string(obj), "up")) 
		return ems_cmd_network_up(core, sess, req);

	return EMS_OK;
}

static ems_int
ems_cmd_config_set(ems_core *core, ems_session *sess, json_object *req)
{
	ems_int cfg;

	ems_json_get_int(req, "firstconfig", cfg);

	if (cfg)
		ems_flag_set(core->flg, FLG_FIRST_CONFIG);
	else
		ems_flag_unset(core->flg, FLG_FIRST_CONFIG);

	return EMS_OK;
}

ems_int 
ems_cmd_config(ems_core *core, ems_session *sess, json_object *req)
{
	json_object *obj;
	obj = json_object_object_get(req, "method");

	if (!(obj && json_object_is_type(obj, json_type_string)))
		return MSG_ST_INVALID_ARG;

	if (!strcasecmp(json_object_get_string(obj), "set")) 
		return ems_cmd_config_set(core, sess, req);

	return EMS_OK;
}

ems_int ems_cmd_fw(ems_core *core, ems_session *sess, json_object *req)
{
	ems_l_trace("reloading.....");

	ems_app_process(ty_ctrl, ty_net, EMS_APP_STOP,  NULL);
	ems_flush_system_info();
	ems_app_process(ty_ctrl, ty_net, EMS_APP_START, NULL);

	str_clear(&core->gw);
	str_clear(&core->ifname);
	str_clear(&core->ac_mac);
	str_clear(&core->ssid);

	if (!ems_atoi(cfg_get(emscfg(), CFG_wireless_enable_encrypt))) {
		ems_l_trace("reset wireless encrypt method");
		ems_setwifi_nopassword();
	}

	ems_app_process(ty_ctrl, ty_fw,     EMS_APP_EVT_FW_RELOAD, NULL);
	ems_app_process(ty_ctrl, ty_radius, EMS_APP_EVT_FW_RELOAD, NULL);
	ems_l_trace("done !!!");

	return EMS_OK;
}

ems_int ems_cmd_user(ems_core *core, ems_session *sess, json_object *req)
{
	json_object *obj;
	obj = json_object_object_get(req, "method");

	if (!(obj && json_object_is_type(obj, json_type_string)))
		return MSG_ST_INVALID_ARG;

	if (!strcasecmp(json_object_get_string(obj), "kickout"))  {
		obj = json_object_object_get(req, "userip");

		if (!(obj && json_object_is_type(obj, json_type_string)))
			return MSG_ST_INVALID_ARG;

		json_object_object_add(req, "usermac", 
			json_object_new_string(ems_usermac(json_object_get_string(obj)))
		);

		json_object_object_add(req, "username", 
			json_object_new_string(
				ems_app_radius_username(json_object_get_string(obj))
			)
		);

		return ems_send_message(ty_portal,  ty_radius,  EMS_APP_CMD_RADIUS_LOGOUT, req);
	}

	sess_response_set(sess, ems_app_radius_userlist());

	return EMS_OK;
}

ems_int ems_cmd_wireless(ems_core *core, ems_session *sess, json_object *req)
{
	json_object *rsp = NULL;
	json_object *obj;
	obj = json_object_object_get(req, "method");

	if (!(obj && json_object_is_type(obj, json_type_string)))
		return MSG_ST_INVALID_ARG;

	if (!strcasecmp(json_object_get_string(obj), "get"))  {
		rsp = json_object_new_object();
		json_object_object_add(rsp, "enable_encrypt", 
				json_object_new_int(
					ems_atoi(cfg_get(emscfg(), CFG_wireless_enable_encrypt))
				)
		);

		sess_response_set(sess, rsp);
		return EMS_OK;
	}

	return EMS_ERR;
}

#ifdef EMS_LOGGER_FILE
extern ems_int ems_cmd_log(ems_core *core, ems_session *sess, json_object *req)
{
	json_object *rsp = NULL;
	json_object *obj;
	ems_int      l;
	obj = json_object_object_get(req, "method");

	if (!(obj && json_object_is_type(obj, json_type_string)))
		return MSG_ST_INVALID_ARG;

	if (!strcasecmp(json_object_get_string(obj), "set"))  {
		ems_json_get_int_def(req, "loglevel", l, ems_logger_level(logger()));
		if (l != ems_logger_level(logger())) {
			ems_logger_set_level(logger(), l);
		}

		return EMS_OK;
	}

	if (!strcasecmp(json_object_get_string(obj), "get"))  {
		rsp = json_object_new_object();
		json_object_object_add(rsp, "loglevel", 
				json_object_new_int(ems_logger_level(logger()))
		);

		sess_response_set(sess, rsp);
		return EMS_OK;
	}

	return EMS_ERR;
}
#endif

extern ems_int ems_cmd_staticstic(ems_core *core, ems_session *sess, json_object *req)
{
	json_object *rsp;

	rsp = json_object_new_object();

	ems_app_process(ty_ctrl, ty_statistic, CMD_STATICSTIC, rsp);

	sess_response_set(sess, rsp);

	return EMS_OK;
}

extern ems_int ems_cmd_totalinfo(ems_core *core, ems_session *sess, json_object *req)
{
	json_object *rsp;

	rsp = json_object_new_object();

	ems_app_process(ty_ctrl, ty_statistic, CMD_TOTAL_INFO, rsp);

	sess_response_set(sess, rsp);

	return EMS_OK;
}
