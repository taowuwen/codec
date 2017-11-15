
#include "ems_core.h"
#include "ems_client.h"
#include "app.h"
#include "app_nic.h"
#include "ems_fw.h"
#include "ems_dns.h"

static ems_int bwlist_init(app_module *mod)
{
	ems_app *app = NULL;
	ems_assert(mod);

	app = ems_app_new();
	if (!app)
		return EMS_ERR;

	app->ty  = mod->ty;
	str_set(&app->nick, str_text(&mod->desc));
	app->flg = 0;

	ems_app_attach(app);

	mod->ctx = (ems_void *)app;

	return EMS_OK;
}

static ems_int bwlist_uninit(app_module *mod)
{
	mod->ctx = NULL;
	return EMS_OK;
}

static ems_int bwlist_run(app_module *mod, ems_int run)
{
	ems_app *app = (ems_app *)mod->ctx;
	ems_assert(mod && app);

	if (run) {
		if (ems_flag_like(mod->flg, FLG_RUN))
			return EMS_OK;

		ems_flag_set(mod->flg, FLG_RUN);
	} else {
		if (ems_flag_unlike(mod->flg, FLG_RUN))
			return EMS_OK;

		if (app->obj) {
			json_object_put(app->obj);
			app->obj = NULL;
		}

		ems_flag_unset(mod->flg, FLG_RUN);
	}

	return EMS_OK;
}


static ems_int 
bwlist_server_rules_update(app_module *mod, json_object *root)
{
	ems_nic_wireless *ssid = NULL;
	ems_app *app = (ems_app *)mod->ctx;

	if (!root)
		return EMS_OK;

	if (!ems_json_object_to_string_cmp(root, app->obj))
		return EMS_OK;

	if (app->obj) {
		json_object_put(app->obj);
		app->obj = NULL;
	}

	app->obj = ems_json_tokener_parse(json_object_to_json_string(root));

	ems_assert(app->obj && "never show up this line");

	ssid = ems_module_attached(ems_nic_wireless, mod);

	return nic_processmsg(ssid, ty_bwlist, ty_fw, EMS_APP_SERVER_RULES_UPDATE, NULL);
}

static ems_int
bwlist_fw_reload(app_module *mod, json_object *req)
{
	return EMS_OK;
}

static ems_int 
bwlist_server_rules(app_module *mod, json_object *root)
{
	json_object *jobj, *ary;
	ems_app *app = (ems_app *)mod->ctx;

	if (app->obj) {
		ary = json_object_object_get(app->obj, "white");
		if (ary) {
			jobj = ems_json_tokener_parse(json_object_to_json_string(ary));

			if (!jobj)
				return EMS_ERR;

			json_object_object_add(root, "white", jobj);
		}

		ary = json_object_object_get(app->obj, "black");
		if (ary) {
			jobj = ems_json_tokener_parse(json_object_to_json_string(ary));

			if (!jobj)
				return EMS_ERR;

			json_object_object_add(root, "black", jobj);
		}
	}

	return EMS_OK;
}

static ems_int
bwlist_process(app_module *mod, ems_uint s, ems_uint d, ems_uint evt, json_object *root)
{
	switch(evt) {
	case EMS_APP_SERVER_RULES_UPDATE:
		return bwlist_server_rules_update(mod, root);

	case EMS_APP_EVT_FW_RELOAD:
		return bwlist_fw_reload(mod, root);

	case EMS_APP_SERVER_BWLIST:
		return bwlist_server_rules(mod, root);

	case EMS_APP_START:
		return bwlist_run(mod, EMS_YES);

	case EMS_APP_STOP:
		return bwlist_run(mod, EMS_NO);

	default:
		break;
	}

	return EMS_OK;
}


app_module app_bwlist = 
{
	.ty      = ty_bwlist,
	.desc    = ems_string("bwlist"),
	.ctx     = NULL,
	.flg     = 0,

	.init    = bwlist_init,
	.uninit  = bwlist_uninit,
	.run     = bwlist_run,
	.process = bwlist_process,
	.process_rule  = NULL,
	.version_match = NULL,
	.install       = NULL
};
