
#include "ems_core.h"
#include "ems_client.h"
#include "app.h"
#include "ems_cmd.h"


extern app_module app_portal;
extern app_module app_radius;
extern app_module app_bwlist;
extern app_module app_downlink;
extern app_module app_fw;
extern app_module app_ctrl;
extern app_module app_bridge;
extern app_module app_net;
extern app_module app_client;
extern app_module app_statistic;

static app_module *apps[] = {
	[ty_fw]       = &app_fw,
	[ty_portal]   = &app_portal,
	[ty_radius]   = &app_radius,
	[ty_bwlist]   = &app_bwlist,
	[ty_downlink] = &app_downlink,
	[ty_bridge]   = &app_bridge,
	[ty_ctrl]     = &app_ctrl,
	[ty_net]      = &app_net,
	[ty_client]   = &app_client,
	[ty_statistic] = &app_statistic,
	NULL
};

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

ems_int ems_app_detach(ems_app *app)
{
	ems_assert(app);
	ems_queue_remove(&app->entry);
	return EMS_OK;
}

ems_app *ems_app_find(ems_cchar *key)
{
	ems_queue *p;
	ems_app   *app;
	ems_core  *core = emscorer();

	ems_assert(key != NULL);
	if (!key)
		return NULL;

	ems_queue_foreach(&core->app_entry, p) {
		app = ems_container_of(p, ems_app, entry);

		if (!strcmp(key, str_text(&app->nick))) 
		{
			return app;
		}
	}

	return NULL;
}


app_module *app_module_load(ems_cchar *key)
{
	ems_int   i;
	app_module *mod = NULL;

	ems_assert(key != NULL);
	if (!key)
		return NULL;

	for (i = 0; ; i++) {
		mod = apps[i];
		if (!mod)
			break;

		if (!strcmp(str_text(&mod->desc), key))
			return mod;
	}

	return NULL;
}

app_module *app_module_find(ems_uint id)
{
	ems_app_type  ty = (ems_app_type)id;
#if 0
	app_module   *mod = NULL;
	ems_int   i;

	for (i = 0; ; i++) {
		mod = apps[i];
		if (!mod)
			break;

		if (mod->ty == id)
			return mod;
	}
#else
	if ((ty > ty_min) && (ty < ty_max)) {
		return apps[ty];
	}
#endif

	return NULL;
}

ems_int ems_app_modules_init()
{
	ems_int   i, rtn;
	app_module *mod = NULL;

	rtn = EMS_OK;
	for (i = 0; ; i++) {
		mod = apps[i];
		if (!mod)
			break;

		ems_assert(mod->init);
		if (mod->init) {
			ems_l_trace("module:(%d, %s) INIT", mod->ty, str_text(&mod->desc));
			rtn = mod->init(mod);
			if (rtn != EMS_OK)
				break;
		}
	}

//	cfg_write(emscfg());

	return rtn;
}

ems_int ems_app_modules_uninit()
{
	ems_int   i, rtn;
	app_module *mod = NULL;

	rtn = EMS_OK;
	for (i = 0; ; i++) {
		mod = apps[i];
		if (!mod)
			break;

		ems_assert(mod->uninit);
		if (mod->uninit) {
			ems_l_trace("module:(%d, %s) UNINIT", mod->ty, str_text(&mod->desc));
			rtn = mod->uninit(mod);
			if (rtn != EMS_OK)
				break;
		}
	}

	return rtn;
}

ems_int  ems_app_modules_run(ems_int run)
{
	ems_int   i, rtn;
	app_module *mod = NULL;

	rtn = EMS_OK;
	for (i = 0; ; i++) {
		mod = apps[i];
		if (!mod)
			break;

		ems_assert(mod->run);
		if (mod->run) {
			rtn = mod->run(mod, run);
			if (rtn != EMS_OK)
				break;
		}
	}

	return rtn;
}

ems_int  ems_applist_process_evt(ems_uint evt, json_object *root)
{
	ems_int      i, rtn;
	app_module   *mod = NULL;

	rtn = EMS_OK;
	for (i = 0; ; i++) {
		mod = apps[i];
		if (!mod)
			break;

		if (mod->process) {
			rtn = mod->process(mod, 0, mod->ty, evt, root);
			if (rtn != EMS_OK)
				break;
		}
	}

	return rtn;
}


ems_int ems_applist_process(json_object *root)
{
	ems_int      i, rtn;
	app_module   *mod = NULL;
	json_object  *obj = NULL;

	rtn = EMS_OK;
	for (i = 0; ; i++) {
		mod = apps[i];
		if (!mod)
			break;

		obj = json_object_object_get(root, str_text(&mod->desc));

		if (obj && mod->process_rule) {
			rtn = mod->process_rule(mod, obj);
			if (rtn != EMS_OK)
				break;
		}
	}

	cfg_write(emscfg());

	return rtn;
}

#if (EMS_LOGGER_FILE || DEBUG)

ems_cchar *ems_app_desc(ems_uint ty)
{
	app_module *mod;

	mod = app_module_find(ty);
	if (mod)
		return str_text(&mod->desc);

	ems_assert(0 && "never be here");
	return "unknown";
}

ems_cchar *ems_evt_desc(ems_uint evt)
{
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
	case EMS_APP_PORTAL_ADDRESS:
		return "EMS_APP_PORTAL_ADDRESS";
	case EMS_APP_FW_ADDRESS_ADD:
		return "EMS_APP_FW_ADDRESS_ADD";
	case EMS_APP_FW_ADDRESS_DEL:
		return "EMS_APP_FW_ADDRESS_DEL";
	case EMS_APP_FW_RADIUS_DEVICE_FREE:
		return "EMS_APP_FW_RADIUS_DEVICE_FREE";
	case EMS_APP_CHECK_SUBDOMAIN:
		return "EMS_APP_CHECK_SUBDOMAIN";
	case EMS_APP_CHECK_PARAM_APPLE_COM:
		return "EMS_APP_CHECK_PARAM_APPLE_COM";
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

	case CMD_STATICSTIC:
		return "CMD_STATICSTIC";

	case CMD_UPDATESTATUS:
		return "CMD_UPDATESTATUS";

	case CMD_GET_DC:
		return "CMD_GET_DC";

	case CMD_GET_CONF:
		return "CMD_GET_CONF";

	case CMD_GET_UPDATEFILE:
		return "CMD_GET_UPDATEFILE";

	case CMD_DOWNLOAD:
		return "CMD_DOWNLOAD";

	default:
		break;
	}

	return "****UNKNOWN****";
}
#endif


ems_int ems_app_process(ems_uint s, ems_uint d, ems_uint evt, json_object *root)
{
	app_module *mod;

	ems_l_trace("\033[01;33m[PROCESS EVT (%s --> %s: %s)]\033[00m args: %s",
			ems_app_desc(s), 
			ems_app_desc(d), 
			ems_evt_desc(evt),
			root?json_object_to_json_string(root):"");

	mod = app_module_find(d);

	if (mod && mod->process)
		return mod->process(mod, s, d, evt, root);

	return EMS_OK;
}

ems_int ems_app_run(ems_uint id)
{
	app_module *mod;

	mod = app_module_find(id);

	if (mod && ems_flag_like(mod->flg, FLG_RUN))
		return EMS_YES;

	return EMS_NO;
}

ems_cchar *ems_app_radius_username(ems_cchar *ip)
{
extern ems_cchar *radius_username(app_module *mod, ems_cchar *ip);
	return radius_username(app_module_find(ty_radius), ip);
}

ems_int ems_app_radius_user_number()
{
extern ems_int radius_online_user_number(app_module *mod);
	return radius_online_user_number(app_module_find(ty_radius));
}

json_object *ems_app_radius_userlist()
{
extern json_object *radius_userlist(app_module *mod);
	return radius_userlist(app_module_find(ty_radius));
}


