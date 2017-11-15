
#include "ems_core.h"
#include "ems_client.h"
#include "app.h"
#include "app_nic.h"

extern app_module app_portal;
extern app_module app_radius;
extern app_module app_bwlist;
//extern app_module app_downlink;
extern app_module app_fw;
extern app_module app_ctrl;
//extern app_module app_bridge;
extern app_module app_net;
extern app_module app_client;
extern app_module app_http;
extern app_module app_tunnel;
extern app_module app_g_fw;
extern app_module app_nic;

static app_module *apps[] = {
	[ty_fw]       = &app_fw,
	[ty_portal]   = &app_portal,
	[ty_radius]   = &app_radius,
	[ty_bwlist]   = &app_bwlist,
	[ty_downlink] = NULL,
	[ty_bridge]   = NULL,
	[ty_ctrl]     = &app_ctrl,
	[ty_net]      = &app_net,
	[ty_client]   = &app_client,
	[ty_http]     = &app_http,
	[ty_tunnel]   = &app_tunnel,
	[ty_g_fw]     = &app_g_fw,
	[ty_nic]      = &app_nic,
	[ty_max]      = NULL,
};


app_module *ems_app_modules_new()
{
	app_module *mod = NULL;

	mod = (app_module *)ems_malloc(sizeof(app_module));

	if (mod) {
		memset(mod, 0, sizeof(app_module));
		str_init(&mod->desc);
		mod->ctx  = NULL;
		mod->rsrv = NULL;
		mod->flg  = 0;

		mod->init 		= NULL;
		mod->uninit 		= NULL;
		mod->process 		= NULL;
		mod->run 		= NULL;
		mod->process_rule 	= NULL;
		mod->version_match 	= NULL;
		mod->install 		= NULL;
	}

	return mod;
}

ems_void ems_app_module_destroy(app_module *mod)
{
	ems_assert(mod);

	if (mod) {
		str_uninit(&mod->desc);
		ems_free(mod);
	}
}

app_module *ems_app_module_dup(ems_app_type ty)
{
	app_module *mod, *tmp;

	if (ty <= ty_min || ty >= ty_max)
		return NULL;

	mod = tmp = NULL;
	if (apps[ty] != NULL) {

		mod = ems_app_modules_new();
		if (!mod)
			return NULL;

		tmp = apps[ty];

		mod->ty = tmp->ty;
		str_cpy(&mod->desc, &tmp->desc);

		mod->init    = tmp->init;
		mod->uninit  = tmp->uninit;
		mod->process = tmp->process;
		mod->run     = tmp->run;
		mod->install = tmp->install;
		mod->process_rule  = tmp->process_rule;
		mod->version_match = tmp->version_match;
	}

	return mod;
}

app_module **ems_app_module_array()
{
	ems_app_type ty;

	app_module **mod;

	mod = (app_module **)ems_malloc(sizeof(apps));
	if (mod) {

		for (ty = ty_min + 1; ty < ty_max; ty++) {
			mod[ty] = NULL;
		}
	}

	return mod;
}

ems_void  ems_app_module_array_destroy(app_module **mod)
{
	ems_app_type ty;
	app_module *_mod = NULL;

	if (mod) {
		for (ty = ty_min + 1; ty < ty_max; ty++) {
			if (mod[ty] != NULL) {
				_mod = mod[ty];

				if (_mod && _mod->uninit) {
					ems_l_trace("module:(%d, %s) UNINIT", 
							_mod->ty, str_text(&_mod->desc));
					_mod->uninit(_mod);
				}

				ems_app_module_destroy(mod[ty]);
				mod[ty] = NULL;
			}
		}

		ems_free(mod);
	}
}

ems_app  *ems_app_new()
{
	ems_app *app = NULL;


	app = (ems_app *)ems_malloc(sizeof(ems_app));
	if (app) {
		memset(app, 0, sizeof(ems_app));

		app->obj = NULL;
		str_init(&app->nick);
		str_init(&app->rsrv);
		ems_queue_init(&app->entry);
	}

	return app;
}

ems_void  ems_app_destroy(ems_app *app)
{
	if (app) {
		if (app->obj) {
			json_object_put(app->obj);
			app->obj = NULL;
		}
		str_uninit(&app->nick);
		str_uninit(&app->rsrv);
		ems_free(app);
	}
}

ems_int ems_app_attach(ems_app *app)
{
	ems_core *core = emscorer();
	ems_assert(app);
	ems_queue_insert_tail(&core->app_entry, &app->entry);
	return EMS_OK;
}


ems_int _ems_app_module_load(app_module **mod_ary, ems_app_type ty, ems_void *ctx)
{
	app_module *mod = NULL;

	mod = ems_app_module_dup(ty);
	if (!mod)
		return EMS_ERR;

	mod->rsrv = ctx;

	if (mod->init) {
		ems_l_trace("module:(%d, %s) INIT", mod->ty, str_text(&mod->desc));
		if (mod->init(mod) != EMS_OK) {
			ems_app_module_destroy(mod);
			return EMS_ERR;
		}
	}

	mod_ary[ty] = mod;

	return EMS_OK;
}

app_module *app_module_find(app_module **ary_mod, ems_uint id)
{
	ems_app_type  ty = (ems_app_type)id;

	ems_assert(ary_mod);

	if ((ty > ty_min) && (ty < ty_max)) {
		return ary_mod[ty];
	}

	return NULL;
}

ems_int ems_app_modules_run(app_module **ary_mod, ems_int run)
{
	ems_app_type ty;
	ems_int      rtn;
	app_module  *mod = NULL;

	ems_assert(ary_mod != NULL);

	rtn = EMS_OK;
	for (ty = ty_min + 1; ty < ty_max; ty++) {
		mod = ary_mod[ty];

		if (mod && mod->run) {
			rtn = mod->run(mod, run);
			if (rtn != EMS_OK)
				break;
		}
	}

	return rtn;
}


#if (EMS_LOGGER_FILE || DEBUG)

ems_cchar *ems_app_desc(ems_app_type ty)
{
	app_module *mod = NULL;

	if (ty > ty_min && ty < ty_max)
		mod = apps[ty];

	if (mod)
		return str_text(&mod->desc);

	return "unknown";
}

ems_cchar *ems_evt_desc(ems_uint evt)
{
	static ems_char buf[128];

#ifdef DEBUG
	switch(evt) {
	case EMS_APP_CMD_RADIUS_AUTH:
		return "EMS_APP_CMD_RADIUS_AUTH";
	case EMS_APP_CMD_RADIUS_AUTH_RSP:
		return "EMS_APP_CMD_RADIUS_AUTH_RSP";
	case EMS_APP_CMD_RADIUS_LOGOUT:
		return "EMS_APP_CMD_RADIUS_LOGOUT";
	case EMS_APP_CMD_PORTAL_LOGOUT:
		return "EMS_APP_CMD_PORTAL_LOGOUT";
	case EMS_APP_EVT_FW_RELOAD:
		return "EMS_APP_EVT_FW_RELOAD";
	case EMS_APP_RULES_UPDATE:
		return "EMS_APP_RULES_UPDATE";
	case EMS_APP_SERVER_RULES_UPDATE:
		return "EMS_APP_SERVER_RULES_UPDATE";
	case EMS_EVT_DOWNLINK_NUM:
		return "EMS_EVT_DOWNLINK_NUM";
	case EMS_EVT_DOWNLINK_IN:
		return "EMS_EVT_DOWNLINK_IN";
	case EMS_APP_STOP:
		return "EMS_APP_STOP";
	case EMS_APP_START:
		return "EMS_APP_START";
	case EMS_APP_FW_ADDRESS_ADD:
		return "EMS_APP_FW_ADDRESS_ADD";
	case EMS_APP_FW_ADDRESS_DEL:
		return "EMS_APP_FW_ADDRESS_DEL";
	case EMS_APP_FW_RADIUS_DEVICE_FREE:
		return "EMS_APP_FW_RADIUS_DEVICE_FREE";
	case EMS_APP_CHECK_SUBDOMAIN:
		return "EMS_APP_CHECK_SUBDOMAIN";
	case EMS_APP_DNS_INTERCEPT_START:
		return "EMS_APP_DNS_INTERCEPT_START";
	case EMS_APP_DNS_INTERCEPT_STOP:
		return "EMS_APP_DNS_INTERCEPT_STOP";
	case EMS_APP_EMS_STATUS:
		return "EMS_APP_EMS_STATUS";
	case EMS_APP_FW_CLEAR:
		return "EMS_APP_FW_CLEAR";
	case EMS_APP_SERVER_BWLIST:
		return "EMS_APP_SERVER_BWLIST";
	case EMS_APP_FW_SET_NAS_FREE:
		return "EMS_APP_FW_SET_NAS_FREE";
	case EMS_APP_FW_SET_HTTP_FREE:
		return "EMS_APP_FW_SET_HTTP_FREE";

	case EMS_APP_FW_SET_GREY_FREE:
		return "EMS_APP_FW_SET_GREY_FREE";

	case EMS_APP_EVT_DETAIL:
		return "EMS_APP_DETAIL";

	case EMS_APP_PROCESS_MSGQUEUE:
		return "EMS_APP_PROCESS_MSGQUEUE";

	case EMS_APP_EVT_USERINFO:
		return "EMS_APP_EVT_USERINFO";

	case EMS_APP_EVT_BSSIDINFO:
		return "EMS_APP_EVT_BSSIDINFO";

	case EMS_APP_EVT_USERNUMBER:
		return "EMS_APP_EVT_USERNUMBER";

	case EMS_APP_EVT_USERLIST:
		return "EMS_APP_EVT_USERLIST";

	case EMS_APP_EVT_USERNAME:
		return "EMS_APP_EVT_USERNAME";

	case EMS_APP_EVT_WIRED_RELOAD:
		return "EMS_APP_EVT_WIRED_RELOAD";

	default:
		break;
	}
#endif

	snprintf(buf, sizeof(buf), "evt(%d, 0x%04x)", evt, evt);
	return buf;
}
#endif


ems_int _ems_app_process(app_module **ary_mod, ems_uint s, ems_uint d, ems_uint evt, json_object *root)
{
	app_module       *mod;
	ems_nic_wireless *ssid;
	ems_cchar        *desc;

	mod = app_module_find(ary_mod, d);

	if (mod && mod->process) {
		if (evt != EMS_APP_PROCESS_MSGQUEUE) { /* too much */
			ssid = ems_module_attached(ems_nic_wireless, mod);

			desc = "xx:xx:xx:xx:xx:xx";
			if (ssid && ssid->_iface && str_len(&ssid->_iface->bssid) > 0)
				desc = str_text(&ssid->_iface->bssid);

			ems_l_trace("[%s][%-8s %s] (%s -> %s): %s", 
				desc,
				ems_app_desc(d),
				ems_flag_like(mod->flg, FLG_RUN)?"RUNNING":"STOPPED",
				ems_app_desc(s), 
				ems_evt_desc(evt),
				root?json_object_to_json_string(root):"NULL");
		}

		return mod->process(mod, s, d, evt, root);
	}

	return EMS_OK;
}

ems_int _ems_app_run(app_module **ary_mod, ems_uint id)
{
	app_module *mod;

	mod = app_module_find(ary_mod, id);

	if (mod && ems_flag_like(mod->flg, FLG_RUN))
		return EMS_YES;

	return EMS_NO;
}

ems_cchar *ems_app_radius_username(ems_cchar *ip)
{
	static ems_char name[128];
	json_object *jobj, *obj;

	ems_assert(ip != NULL);

	jobj = json_object_new_object();

	json_object_object_add(jobj, "userip", json_object_new_string(ip));

	ems_app_process(ty_ctrl, ty_nic, EMS_APP_EVT_USERNAME, jobj);

	obj = json_object_object_get(jobj, "username");
	if (obj)
		snprintf(name, sizeof(name), "%s", json_object_get_string(obj));
	else
		name[0] = '\0';

	json_object_put(jobj);

	return name;
}

json_object *ems_app_radius_userlist()
{
	json_object *root, *ary;

	root = json_object_new_object();
	ary  = json_object_new_array();

	json_object_object_add(root, "user", ary);

	ems_app_process(ty_ctrl, ty_nic, EMS_APP_EVT_USERLIST, ary);

	return root;
}

