
#include "ems_core.h"
#include "ems_client.h"
#include "ems_fw.h"
#include "app.h"
#include "ems_netcheck.h"

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

	if (restart) {
		cfg_write(emscfg());
		if (ems_app_run(ty_client)) {
			ems_send_message(ty_ctrl, ty_client, EMS_APP_STOP,  NULL);
			ems_send_message(ty_ctrl, ty_client, EMS_APP_START, NULL);
		}
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


static ems_int 
ems_cmd_status_fill_system(ems_core *core, json_object *root)
{
	json_object *rsp, *net;

	rsp = json_object_new_object();

	json_object_object_add(rsp, "sn",  json_object_new_string(core_sn()));
	json_object_object_add(rsp, "mac", json_object_new_string(core_ac_mac()));

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

	ems_app_process(ty_ctrl, ty_g_fw, EMS_APP_RULES_UPDATE, NULL);
	cfg_write(emscfg());

	return EMS_OK;
}


static ems_int ems_cmd_bwlist_get(ems_core *core, ems_session *sess, json_object *req)
{
	ems_uint flg;
	json_object *rsp;

	ems_json_get_int_def(req, "flag", flg, 0x07);

	rsp = json_object_new_object();

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

static ems_int
ems_cmd_network_up(ems_core *core, ems_session *sess, json_object *req)
{
	if (ems_flag_like(core->flg, FLG_NETWORK_BRIDGE))
		return EMS_OK;

	if (ems_flag_like(core->flg, FLG_NETWORK_READY))
		return EMS_OK;

	ems_app_process(ty_ctrl, ty_net, EMS_APP_STOP, NULL);
	ems_app_process(ty_ctrl, ty_net, EMS_APP_START, NULL);
	ems_app_process(ty_ctrl, ty_client, EMS_APP_EVT_FW_RELOAD, NULL);

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
	ems_app_process(ty_ctrl, ty_net, EMS_APP_STOP,  NULL);
	ems_flush_system_info(emscorer());
	ems_app_process(ty_ctrl, ty_net, EMS_APP_START, NULL);

	ems_send_message(ty_ctrl, ty_g_fw, EMS_APP_EVT_FW_RELOAD, NULL);
	ems_send_message(ty_ctrl, ty_nic,  EMS_APP_EVT_FW_RELOAD, NULL);
	ems_send_message(ty_ctrl, ty_client, EMS_APP_EVT_FW_RELOAD, NULL);

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

		return ems_send_message(ty_ctrl, ty_nic, EMS_APP_CMD_RADIUS_LOGOUT, req);
	}

	sess_response_set(sess, ems_app_radius_userlist());

	return EMS_OK;
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

extern ems_int ems_cmd_lan(ems_core *core, ems_session *sess, json_object *req)
{
	json_object *obj;
	obj = json_object_object_get(req, "method");

	if (!(obj && json_object_is_type(obj, json_type_string)))
		return MSG_ST_INVALID_ARG;

	if (!strcasecmp(json_object_get_string(obj), "reload"))  {
		return ems_send_message(ty_ctrl, ty_nic, EMS_APP_EVT_WIRED_RELOAD, req);
	}

	return EMS_OK;
}
