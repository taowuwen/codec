
#include "ems_core.h"
#include "ems_client.h"
#include "app.h"
#include "ems_fw.h"
#include "ems_portal.h"

static ems_int ptl_start(ems_portal *ptl);
static ems_int ptl_stop(ems_portal *ptl);

static ems_int portal_cfg_init()
{
	ems_cfg *cfg = emscfg();
	if (!cfg_get(cfg, CFG_app_portal_addr))
		cfg_set(cfg, CFG_app_portal_addr, "zuhu-pa.lekewifi.com");

	if (!cfg_get(cfg, CFG_app_portal_port))
		cfg_set(cfg, CFG_app_portal_port, ems_itoa(2000));

	if (!cfg_get(cfg, CFG_app_portal_redirect))
		cfg_set(cfg, CFG_app_portal_redirect, ems_itoa(80));

	if (!cfg_get(cfg, CFG_app_portal_reg))
		cfg_set(cfg, CFG_app_portal_reg, ems_itoa(600));

	if (!cfg_get(cfg, CFG_app_portal_hb))
		cfg_set(cfg, CFG_app_portal_hb, ems_itoa(30));

	if (!cfg_get(emscfg(), CFG_app_portal_auto))
		cfg_set(cfg, CFG_app_portal_auto, ems_itoa(1));

	return EMS_OK;
}

static ems_int portal_init(app_module *mod)
{
	ems_portal *ptl = NULL;
	ems_assert(mod);

	portal_cfg_init();

	ptl = (ems_portal *)ems_malloc(sizeof(ems_portal));
	if (!ptl)
		return EMS_ERR;

	memset(ptl, 0, sizeof(ems_portal));

	ems_queue_init(&ptl->users);
	ptl->sess = NULL;
	str_init(&ptl->addr);
	ptl->st = st_stopped;

	mod->ctx = (ems_void *)ptl;

	return EMS_OK;
}


static ems_int portal_uninit(app_module *mod)
{
	ems_portal *ptl = (ems_portal *)mod->ctx;
	/* stop portal here */
	mod->ctx = NULL;

	if (ptl) {
		ems_queue_clear(&ptl->users, portal_user, entry, ptl_user_destroy);
		str_uninit(&ptl->addr);
		ems_free(ptl);
	}
	return EMS_OK;
}

static ems_int portal_rules_update(ems_portal *ptl, json_object *req, ems_int run)
{
	ems_uint  tmp, restart;
	ems_str   buf;
	ems_cfg  *cfg  = emscfg();

	restart = 0;
	str_init(&buf);

	ems_json_get_string_def(req, "addr", &buf, NULL);
	if (str_len(&buf) > 0) {
		cfg_set(cfg, CFG_app_portal_addr, str_text(&buf));

		if (!restart && 
			((str_len(&ptl->addr) != str_len(&buf)) || 
			strcmp(str_text(&ptl->addr), str_text(&buf)))) 
		{
			restart = 1;
		}
	}

	if (strcmp("127.0.0.1", cfg_get(emscfg(), CFG_app_portal_addr))){
		ems_flag_set(emscorer()->flg, FLG_CONFIG_READY);
	}

	ems_json_get_int_def(req, "port", tmp, 2000);
	cfg_set(cfg, CFG_app_portal_port, ems_itoa(tmp));
	if (!restart && tmp != ptl->port) restart = 1;

	ems_json_get_int_def(req, "reg_period", tmp, 600);
	cfg_set(cfg, CFG_app_portal_reg, ems_itoa(tmp));
	if (tmp > 0) ptl->reg_period = tmp;

	ems_json_get_int_def(req, "hb_period", tmp, 30);
	cfg_set(cfg, CFG_app_portal_hb, ems_itoa(tmp));
	if (tmp > 0) ptl->hb_period = tmp;

	ems_json_get_int_def(req, "redirect_port", tmp, 80);
	cfg_set(cfg, CFG_app_portal_redirect, ems_itoa(tmp));

	str_uninit(&buf);

	if (run && restart) {
		ems_l_trace("portal restart...");
		ptl_stop(ptl);
		ptl_start(ptl);
	}

	return EMS_OK;
}

static ems_int portal_app_rules_update(app_module *mod, json_object *req, ems_int run)
{
	ems_cfg   *cfg = emscfg();
	ems_uint   tmp;
	ems_core  *core = emscorer();
	ems_portal *ptl = (ems_portal *)mod->ctx;

	ems_json_get_int_def(req, "auto", tmp, 1);
	cfg_set(cfg, CFG_app_portal_auto, ems_itoa(tmp));

	if (tmp)
		ems_flag_set(core->flg, FLG_PORTAL_AUTO);
	else
		ems_flag_unset(core->flg, FLG_PORTAL_AUTO);

	if (tmp)
		return EMS_OK;

	return portal_rules_update(ptl, req, run);
}

static ems_int
portal_app_auth_rsp(app_module *mod, json_object *root)
{
	json_object *obj;
	ems_int      err;
	ems_portal  *ptl  = (ems_portal *)mod->ctx;

	ems_json_get_int_def(root, "error_code", err, 0);

	obj = json_object_object_get(root, "userip");
	ems_assert(obj != NULL);

	if (!(obj && json_object_is_type(obj, json_type_string)))
		return EMS_ERR;

	return ptl_user_auth_rsp(ptl, ptl_user_find(ptl, json_object_get_string(obj)), err);
}

static ems_int
portal_app_logout(app_module *mod, json_object *root)
{
	json_object *obj;
	ems_portal  *ptl  = (ems_portal *)mod->ctx;

	obj = json_object_object_get(root, "userip");
	ems_assert(obj != NULL);

	if (!(obj && json_object_is_type(obj, json_type_string)))
		return EMS_ERR;

	return ptl_user_ntf_logout(ptl, ptl_user_find(ptl, json_object_get_string(obj)));
}

static ems_int
portal_app_server_rules_update(app_module *mod, json_object *root)
{
	ems_portal  *ptl  = (ems_portal *)mod->ctx;

	if (!ems_atoi(cfg_get(emscfg(), CFG_app_portal_auto))) {
		ems_l_trace("use user defined configs...");
		return EMS_OK;
	}

	return portal_rules_update(ptl, root, EMS_YES);
}

static ems_int
portal_app_evt_status(app_module *mod, json_object *root)
{
	ems_portal  *ptl  = (ems_portal *)mod->ctx;
	return ptl->lasterr;
}

static ems_int
portal_evt_address(app_module *mod, json_object *root)
{
	ems_portal  *ptl  = (ems_portal *)mod->ctx;
	return ptl_current_address(ptl);
}

static ems_int portal_run(app_module *mod, ems_int run);

static ems_int 
portal_process(app_module *mod, ems_uint s, ems_uint d, ems_uint evt, json_object *root)
{
	ems_l_trace("portal got evt: 0x%x, args: %s", evt, json_object_to_json_string(root));

	switch(evt) {
	case EMS_APP_START:
		return portal_run(mod, EMS_YES);

	case EMS_APP_STOP:
		return portal_run(mod, EMS_NO);

	case EMS_APP_RULES_UPDATE:
		return portal_app_rules_update(mod, root, ems_flag_like(mod->flg, FLG_RUN));

	default:
		break;
	}

	if (ems_flag_unlike(mod->flg, FLG_RUN))
		return EMS_ERR;

	switch(evt) {
	case EMS_APP_EVT_FW_RELOAD:
	{
		ems_portal  *ptl = (ems_portal *)mod->ctx;
		json_object *ary;
		ary = json_object_new_array();

		json_object_array_add(ary, json_object_new_string(str_text(&ptl->addr)));
		ems_app_process(ty_portal, ty_fw, EMS_APP_FW_ADDRESS_ADD, ary);

		json_object_put(ary);
		return EMS_OK;
	}
	break;

	case EMS_APP_RULES_UPDATE:
		return portal_app_rules_update(mod, root, EMS_YES);

	case EMS_APP_CMD_RADIUS_AUTH_RSP:
		return portal_app_auth_rsp(mod, root);

	case EMS_APP_CMD_PORTAL_LOGOUT:
		return portal_app_logout(mod, root);

	case EMS_APP_SERVER_RULES_UPDATE:
		return portal_app_server_rules_update(mod, root);

	case EMS_APP_EVT_STATUS:
		return portal_app_evt_status(mod, root);

	case EMS_APP_PORTAL_ADDRESS:
		return portal_evt_address(mod, root);

	default:
		break;
	}

	return EMS_OK;
}

static ems_int portal_process_rule(app_module *mod, json_object *root)
{
	return EMS_OK;
}

static ems_int ptl_start(ems_portal *ptl)
{
	ems_cfg *cfg = emscfg();

	str_set(&ptl->addr, cfg_get(cfg, CFG_app_portal_addr));
	ptl->port       = ems_atoi(cfg_get(cfg, CFG_app_portal_port));
	ptl->reg_period = ems_atoi(cfg_get(cfg, CFG_app_portal_reg)); 
	ptl->hb_period  = ems_atoi(cfg_get(cfg, CFG_app_portal_hb));

	ems_l_trace("portal start: (%s:%d, reg: %d, hb: %d)",
			str_text(&ptl->addr), ptl->port, ptl->reg_period, ptl->hb_period);
	{
		json_object *ary;
		ary = json_object_new_array();

		json_object_array_add(ary, json_object_new_string(str_text(&ptl->addr)));
		ems_app_process(ty_portal, ty_fw, EMS_APP_FW_ADDRESS_ADD, ary);

		json_object_put(ary);
	}
	return portal_change_status(ptl, st_start);
}

static ems_int ptl_stop(ems_portal *ptl)
{
	ems_assert(str_len(&ptl->addr) > 0 && str_text(&ptl->addr));
	{
		json_object *ary;
		ary = json_object_new_array();

		json_object_array_add(ary, json_object_new_string(str_text(&ptl->addr)));
		ems_app_process(ty_portal, ty_fw, EMS_APP_FW_ADDRESS_DEL, ary);

		json_object_put(ary);
	}
	return portal_change_status(ptl, st_stopped);
}

static ems_int portal_run(app_module *mod, ems_int run)
{
	ems_portal *ptl = (ems_portal *)mod->ctx;
	ems_assert(mod && ptl);

	if (run) {
		if (ems_flag_like(mod->flg, FLG_RUN))
			return EMS_OK;

		ems_l_trace("portal running here");
		if (ptl_start(ptl) == EMS_OK)
			ems_flag_set(mod->flg, FLG_RUN);
	} else {
		if (ems_flag_unlike(mod->flg, FLG_RUN))
			return EMS_OK;

		ptl_stop(ptl);
		ems_flag_unset(mod->flg, FLG_RUN);
		ems_l_trace("portal stopped");
	}

	return EMS_OK;
}

app_module app_portal = {

	.ty      = ty_portal,
	.desc    = ems_string("portal"),
	.ctx     = NULL,
	.flg     = 0,

	.init    = portal_init,
	.uninit  = portal_uninit,
	.run     = portal_run,
	.process = portal_process,
	.process_rule = portal_process_rule,
	.version_match = NULL,
	.install       = NULL
};
