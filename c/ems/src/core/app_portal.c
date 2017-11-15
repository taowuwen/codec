
#include "ems_core.h"
#include "ems_client.h"
#include "app.h"
#include "ems_fw.h"
#include "ems_portal.h"

static ems_int ptl_start(ems_portal *ptl);
static ems_int ptl_stop(ems_portal *ptl);

static ems_int portal_init(app_module *mod)
{
	ems_portal *ptl = NULL;
	ems_assert(mod);

	ptl = (ems_portal *)ems_malloc(sizeof(ems_portal));
	if (!ptl)
		return EMS_ERR;

	memset(ptl, 0, sizeof(ems_portal));

	ems_queue_init(&ptl->users);
	ptl->sess = NULL;
	str_init(&ptl->addr);
	ptl->st = st_stopped;
	ptl->ssid = ems_module_attached(ems_nic_wireless, mod);
	ems_assert(ptl->ssid != NULL);

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
	ems_wifi_iface *iface;
	ems_uint  restart;
	ems_str  *addr;

	ems_assert(ptl->ssid && ptl->ssid->_iface);
	iface = ptl->ssid->_iface;

	do {
		restart = EMS_YES;
		addr = &iface->auth.ptl.addr;
		ems_assert(addr != NULL && str_len(addr) > 0);

		if ((str_len(addr) != str_len(&ptl->addr)) ||
		     strcmp(str_text(addr), str_text(&ptl->addr))) 
		{
			break;
		}

		if (iface->auth.ptl.port             != ptl->port)       break;
		if (iface->auth.ptl.register_period  != ptl->reg_period) break;
		if (iface->auth.ptl.heartbeat_period != ptl->hb_period)  break;

		restart = EMS_NO;
	} while (0);

	if (run && restart) {
		ptl_stop(ptl);
		ptl_start(ptl);
	}

	return EMS_OK;
}

static ems_int portal_app_rules_update(app_module *mod, json_object *req, ems_int run)
{
	ems_portal *ptl = (ems_portal *)mod->ctx;

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

	portal_rules_update(ptl, root, ems_flag_like(mod->flg, FLG_RUN));

	return EMS_OK;
}

static ems_int
portal_app_evt_status(app_module *mod, json_object *root)
{
	if (ems_flag_unlike(mod->flg, FLG_RUN))
		return EMS_OK;

	ems_portal  *ptl  = (ems_portal *)mod->ctx;
	return ptl->lasterr;
}

static ems_int 
portal_app_evt_detail(app_module *mod, json_object *root)
{
	json_object *ptl_root;

	ptl_root = json_object_new_object();

	json_object_object_add(ptl_root, "status", json_object_new_int(ems_flag_like(mod->flg, FLG_RUN)?1:0));

	json_object_object_add(root, "portal", ptl_root);

	return EMS_OK;
}

static ems_int portal_run(app_module *mod, ems_int run);

static ems_int 
portal_process(app_module *mod, ems_uint s, ems_uint d, ems_uint evt, json_object *root)
{
	switch(evt) {
	case EMS_APP_START:
		return portal_run(mod, EMS_YES);

	case EMS_APP_STOP:
		return portal_run(mod, EMS_NO);

	case EMS_APP_RULES_UPDATE:
		return portal_app_rules_update(mod, root, ems_flag_like(mod->flg, FLG_RUN));

	case EMS_APP_EVT_STATUS:
		return portal_app_evt_status(mod, root);

	case EMS_APP_SERVER_RULES_UPDATE:
		return portal_app_server_rules_update(mod, root);

	case EMS_APP_EVT_DETAIL:
		return portal_app_evt_detail(mod, root);

	case EMS_APP_CMD_RADIUS_AUTH_RSP:
		return portal_app_auth_rsp(mod, root);

	case EMS_APP_CMD_PORTAL_LOGOUT:
		return portal_app_logout(mod, root);

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
		nic_processmsg(ptl->ssid, ty_portal, ty_fw, EMS_APP_FW_ADDRESS_ADD, ary);

		json_object_put(ary);
		return EMS_OK;
	}
	break;

	default:
		break;
	}

	return EMS_OK;
}

static ems_int ptl_start(ems_portal *ptl)
{
	ems_wifi_iface *iface = ptl->ssid->_iface;

	if (!iface->auth.enable){
		ems_l_info("[portal %s] portal disabled", str_text(&iface->bssid));
		nic_sendmsg(ptl->ssid, ty_portal, ty_fw,     EMS_APP_STOP, NULL);
		nic_sendmsg(ptl->ssid, ty_portal, ty_radius, EMS_APP_STOP, NULL);
		nic_sendmsg(ptl->ssid, ty_portal, ty_http,   EMS_APP_STOP, NULL);
		return EMS_ERR;
	}

	str_cpy(&ptl->addr, &iface->auth.ptl.addr);
	ptl->port       = iface->auth.ptl.port;
	ptl->reg_period = iface->auth.ptl.register_period;
	ptl->hb_period  = iface->auth.ptl.heartbeat_period;

	if (strcmp("127.0.0.1", str_text(&ptl->addr))) {
		ems_flag_set(emscorer()->flg, FLG_CONFIG_READY);
	}

	ems_l_trace("[portal] ptl(%s:%d, reg: %d, hb: %d)",
		str_text(&ptl->addr), ptl->port, ptl->reg_period, ptl->hb_period);

	return portal_change_status(ptl, st_start);
}

static ems_int ptl_stop(ems_portal *ptl)
{
	ems_assert(str_len(&ptl->addr) > 0 && str_text(&ptl->addr));
	{
		json_object *ary;
		ary = json_object_new_array();

		json_object_array_add(ary, json_object_new_string(str_text(&ptl->addr)));
		nic_processmsg(ptl->ssid, ty_portal, ty_fw, EMS_APP_FW_ADDRESS_DEL, ary);

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

		if (ptl_start(ptl) == EMS_OK)
			ems_flag_set(mod->flg, FLG_RUN);
	} else {
		if (ems_flag_unlike(mod->flg, FLG_RUN))
			return EMS_OK;

		ptl_stop(ptl);
		ems_flag_unset(mod->flg, FLG_RUN);
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
	.process_rule = NULL,
	.version_match = NULL,
	.install       = NULL
};
