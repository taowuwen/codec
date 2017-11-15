
#include "ems_core.h"
#include "ems_client.h"
#include "app.h"
#include "ems_dns.h"
#include "app_nic.h"
#include "ems_split.h"
#include "ems_uci.h"
#include "ems_fw.h"

typedef ems_int (*pnic_walk_ssid_cb)(ems_void *arg, ems_nic_wireless *ssid);

static ems_int nic_walk_ssid(ems_nic *nic, pnic_walk_ssid_cb cb, ems_void *arg)
{
	ems_int ret;
	ems_queue        *p;
	ems_nic_wireless *ssid;

	ems_assert(nic && cb);

	ems_queue_foreach(&nic->list, p) {
		ssid = ems_container_of(p, ems_nic_wireless, entry);

		ret = cb(arg, ssid);
		if (ret != EMS_OK)
			return ret;
	}

	return EMS_OK;
}


static ems_int nic_init(app_module *mod)
{
	ems_nic *nic = NULL;

	nic = (ems_nic *)ems_malloc(sizeof(ems_nic));

	if (!nic) 
		return EMS_ERR;

	memset(nic, 0, sizeof(ems_nic));
	str_init(&nic->nick);
	ems_queue_init(&nic->list);

	nic->wired.lan_free = 0;
	nic->wired.iface_id = 0;
	nic->jcfg = NULL;
	nic->jhttp = NULL;

	mod->ctx = (ems_void *)nic;

	return EMS_OK;
}

static ems_int nic_uninit(app_module *mod)
{
	ems_nic *nic;

	if (mod->ctx) {
		nic = (ems_nic *) mod->ctx;

		ems_assert(ems_queue_empty(&nic->list));
		str_uninit(&nic->nick);
		json_object_put(nic->jcfg);
		json_object_put(nic->jhttp);
		json_object_put(nic->jserv);

		ems_free(nic);
		mod->ctx = NULL;
	}

	return EMS_OK;
}

static ems_int nic_reload_wireless_cfg(ems_nic *nic)
{
	json_object *ary = NULL;
	ems_int      rtn;

	if (nic->jcfg != NULL) {
		json_object_put(nic->jcfg);
		nic->jcfg = NULL;
	}

	ary = json_object_new_array();

	rtn = ems_uci_load_cfg(UCI_CFG_WIRELESS, UCI_SEARCH_PATH, ary);

	if (rtn != EMS_OK)
		json_object_put(ary);
	else
		nic->jcfg = ary;

	return rtn;
}

static ems_nic_wireless *nic_ssid_find(ems_nic *nic, ems_int id)
{
	ems_queue *p;
	ems_nic_wireless *ssid;

	ems_queue_foreach(&nic->list, p) {
		ssid = ems_container_of(p, ems_nic_wireless, entry);

		if (ssid->_iface->id == id)
			return ssid;
	}

	return NULL;
}

static ems_nic_wireless *nic_ssid_new()
{
	ems_nic_wireless *ssid = NULL;

	ssid = (ems_nic_wireless *)ems_malloc(sizeof(ems_nic_wireless));
	if (ssid) {
		memset(ssid, 0, sizeof(ems_nic_wireless));

		ems_queue_init(&ssid->entry);
		ems_queue_init(&ssid->msg_entry);
		ssid->_nic   = NULL;
		ssid->_iface = NULL;
		ssid->_modl  = NULL;
	}

	return ssid;
}

static ems_wifi_iface *nic_ssid_iface_new()
{
	return (ems_wifi_iface *)ems_malloc(sizeof(ems_wifi_iface));
}


static ems_cchar *nic_json_get_or_set_string(json_object *root, ems_cchar *key, ems_cchar *val)
{
	json_object *obj;

	obj = json_object_object_get(root, key);
	if (!obj) {
		json_object_object_add(root, key, json_object_new_string(val));
		return val;
	}

	return json_object_get_string(obj);
}

#define nic_json_get_or_set_int(root, key, val) \
	ems_atoi(nic_json_get_or_set_string(root, key, ems_itoa(val)))


static ems_int ssid_load_iface_cfg(ems_wifi_iface *iface, json_object *root)
{
	iface->id       = nic_json_get_or_set_int(root, "ems_iface_id", 0);
	iface->disabled = nic_json_get_or_set_int(root, "disabled",  0);

	ems_json_get_string_def(root, "ssid", &iface->ssid, NULL);
	ems_json_get_string_def(root, "ifname", &iface->ifname, NULL);
	if (str_len(&iface->ifname) <= 0)
		return EMS_ERR;

	str_set(&iface->bssid, ems_popen_get(
		"ip addr show dev %s | awk '/link/ {print $2}'", str_text(&iface->ifname)));

	if (str_len(&iface->bssid) <= 0)
		return EMS_ERR;

	ems_json_get_string_def(root, "section_name", &iface->nick, NULL); 

	ems_l_trace("[nic][ssid %d] disabled: %d, section: %s,  ssid: %s, ifname: %s, bssid: %s", 
		iface->id, iface->disabled, 
		str_text(&iface->nick), str_text(&iface->ssid), str_text(&iface->ifname), str_text(&iface->bssid));


	return EMS_OK;
}

static ems_int ssid_load_auth_cfg(ems_wifi_iface *iface, json_object *root)
{
	iface->auth.enable = nic_json_get_or_set_int(root, "ems_portal_enable", 1);
	iface->auth.offline_disconnect = nic_json_get_or_set_int(root, "ems_offline_disconnect", 1800);

	ems_l_trace("[nic][ssid (%d: %s)] auth(enable: %d, offline_disconnect: %d)", 
			iface->id, str_text(&iface->nick), iface->auth.enable, iface->auth.offline_disconnect);

	return EMS_OK;
}

static ems_int ssid_load_ptl_cfg(ems_wifi_iface *iface, json_object *root)
{
	str_set(&iface->auth.ptl.addr,  nic_json_get_or_set_string(root, "ems_portal_address",        "127.0.0.1"));
	iface->auth.ptl.port             = nic_json_get_or_set_int(root, "ems_portal_port",            2000);
	iface->auth.ptl.redirect_port    = nic_json_get_or_set_int(root, "ems_portal_redirect_port",   80);
	iface->auth.ptl.register_period  = nic_json_get_or_set_int(root, "ems_portal_register_period", 600);
	iface->auth.ptl.heartbeat_period = nic_json_get_or_set_int(root, "ems_portal_heartbeat_period",30);

	ems_l_trace("[nic][ssid (%d: %s)] ptl(%s, %d, %d, %d, %d)", 
			iface->id, str_text(&iface->nick),
			str_text(&iface->auth.ptl.addr), iface->auth.ptl.port, iface->auth.ptl.redirect_port,
			iface->auth.ptl.register_period, iface->auth.ptl.heartbeat_period);
	return EMS_OK;
}

static ems_int ssid_load_radius_cfg(ems_wifi_iface *iface, json_object *root)
{
	str_set(&iface->auth.radius.addr,   nic_json_get_or_set_string(root, "ems_radius_address",    "127.0.0.1"));
	str_set(&iface->auth.radius.secret, nic_json_get_or_set_string(root, "ems_radius_shared_key", "admin"));
	iface->auth.radius.auth_port   = nic_json_get_or_set_int(root, "ems_radius_auth_port",   1812);
	iface->auth.radius.acct_port   = nic_json_get_or_set_int(root, "ems_radius_acct_port",   1813);
	iface->auth.radius.acct_period = nic_json_get_or_set_int(root, "ems_radius_acct_period", 100);

	ems_l_trace("[nic][ssid (%d: %s)] radius(%s, %d, %d, %d, %s)", 
			iface->id, str_text(&iface->nick),
			str_text(&iface->auth.radius.addr),
			iface->auth.radius.auth_port, 
			iface->auth.radius.acct_port, 
			iface->auth.radius.acct_period, 
			str_text(&iface->auth.radius.secret));
	return EMS_OK;
}

#define SSID_SPLITOR	","

static ems_int ssid_load_bwlist_cfg(ems_wifi_iface *iface, json_object *root)
{
	ems_str str;

	str_init(&str);

	ems_split_clear(&iface->auth.bwlist.whitelist);
	ems_split_clear(&iface->auth.bwlist.whitemac);
	ems_split_clear(&iface->auth.bwlist.blackmac);

	ems_json_get_string_def(root, "ems_bwlist_whitelist", &str, NULL);
	if (str_len(&str) > 0) {
		ems_string_split(&iface->auth.bwlist.whitelist, str_text(&str), SSID_SPLITOR);
	}

	ems_json_get_string_def(root, "ems_bwlist_mac_white",  &str, NULL);
	if (str_len(&str) > 0) {
		ems_string_split(&iface->auth.bwlist.whitemac, str_text(&str), SSID_SPLITOR);
	}

	ems_json_get_string_def(root, "ems_bwlist_mac_black",  &str, NULL);
	if (str_len(&str) > 0) {
		ems_string_split(&iface->auth.bwlist.blackmac, str_text(&str), SSID_SPLITOR);
	}

	str_uninit(&str);

	return EMS_OK;
}

static ems_int nic_ssid_iface_init(ems_wifi_iface *iface, json_object *root)
{
	ems_assert(iface && root);

	memset(iface, 0, sizeof(ems_wifi_iface));
	str_init(&iface->ssid); 
	str_init(&iface->ifname); 
	str_init(&iface->bssid); 
	str_init(&iface->nick); 
	str_init(&iface->auth.ptl.addr);
	str_init(&iface->auth.radius.addr);
	str_init(&iface->auth.radius.secret);
	ems_queue_init(&iface->auth.bwlist.whitelist);
	ems_queue_init(&iface->auth.bwlist.whitemac);
	ems_queue_init(&iface->auth.bwlist.blackmac);

	return EMS_OK;
}


static ems_int 
nic_ssid_do_init(ems_nic_wireless *ssid, ems_nic *nic, json_object *root)
{
	ems_int rtn;

	ems_assert(ssid && root);

	if (!(ssid && root))
		return EMS_ERR;

	do {
		rtn = EMS_ERR;
		ssid->_modl = ems_app_module_array();
		if (!ssid->_modl) break;

		if (ems_app_module_load(ssid->_modl, ty_fw,     ssid) != EMS_OK) break;
		if (ems_app_module_load(ssid->_modl, ty_portal, ssid) != EMS_OK) break;
		if (ems_app_module_load(ssid->_modl, ty_radius, ssid) != EMS_OK) break;
		if (ems_app_module_load(ssid->_modl, ty_bwlist, ssid) != EMS_OK) break;
		if (ems_app_module_load(ssid->_modl, ty_http,   ssid) != EMS_OK) break;

		ssid->_nic = nic;
		ssid->_iface = nic_ssid_iface_new();
		if (!ssid->_iface) break;

		nic_ssid_iface_init(ssid->_iface, root);

		rtn = EMS_OK;
	} while (0);

	return rtn;
}

static ems_int nic_ssid_update_mod(ems_nic_wireless *ssid, ems_app_type ty, ems_int disabled)
{
	if (!disabled) {
		if (!nic_app_run(ssid, ty))
			nic_sendmsg(ssid, ty_nic, ty, EMS_APP_START, NULL);
		else
			nic_sendmsg(ssid, ty_nic, ty, EMS_APP_SERVER_RULES_UPDATE, NULL);
	} else {
		if (nic_app_run(ssid, ty))
			nic_sendmsg(ssid, ty_nic, ty, EMS_APP_STOP, NULL);
	}

	return EMS_OK;
}

static ems_int 
nic_ssid_run_mod(ems_nic_wireless *ssid, ems_app_type ty, ems_int run)
{
	if (run) {
		if (!nic_app_run(ssid, ty))
			nic_sendmsg(ssid, ty_nic, ty, EMS_APP_START, NULL);
	} else {
		if (nic_app_run(ssid, ty))
			nic_sendmsg(ssid, ty_nic, ty, EMS_APP_STOP, NULL);
	}

	return EMS_OK;
}

static ems_int nic_ssid_stop(ems_nic_wireless *ssid);
static ems_int nic_ssid_destroy(ems_nic_wireless *ssid);

static ems_int nic_ssid_update_cfg(ems_nic_wireless *ssid, ems_nic *nic, json_object *root)
{
	ems_int  disable;
	ems_wifi_iface *iface = NULL;

	ems_assert(ssid && nic && root);

	iface = ssid->_iface;

	if (ssid_load_iface_cfg(iface, root) != EMS_OK) {
		ems_l_warn("[nic] invalid cfg: %s", json_object_to_json_string(root));
		ems_queue_remove(&ssid->entry);
		nic_ssid_stop(ssid);
		nic_ssid_destroy(ssid);
		return EMS_OK;
	}

	ssid_load_auth_cfg(iface, root);
	ssid_load_ptl_cfg(iface, root);
	ssid_load_radius_cfg(iface, root);
	ssid_load_bwlist_cfg(iface, root);
//	ssid_load_http_cfg(iface, root);

	if (iface->disabled || !iface->auth.enable)
		disable = EMS_YES;
	else
		disable = EMS_NO;

	nic_ssid_update_mod(ssid, ty_portal, disable);
	nic_ssid_update_mod(ssid, ty_radius, disable);

	nic_ssid_run_mod(ssid, ty_http,   !disable);
	nic_ssid_run_mod(ssid, ty_bwlist, !disable);

	/* do not handle module fw here */
	return EMS_OK;
}

static ems_int nic_ssid_stop(ems_nic_wireless *ssid)
{
	nic_processmsg(ssid, ty_nic, ty_portal, EMS_APP_STOP, NULL);
	nic_processmsg(ssid, ty_nic, ty_radius, EMS_APP_STOP, NULL);
	nic_processmsg(ssid, ty_nic, ty_http,   EMS_APP_STOP, NULL);
	nic_processmsg(ssid, ty_nic, ty_bwlist, EMS_APP_STOP, NULL);
	nic_processmsg(ssid, ty_nic, ty_fw,     EMS_APP_STOP, NULL);

	ems_queue_clear(&ssid->msg_entry, msgqueue, entry, ems_mq_destroy);

	return EMS_OK;
}

static ems_void nic_ssid_iface_destroy(ems_wifi_iface *iface)
{
	if (!iface)
		return;

	str_uninit(&iface->ssid);
	str_uninit(&iface->bssid);
	str_uninit(&iface->ifname);
	str_uninit(&iface->nick);

	str_uninit(&iface->auth.ptl.addr);
	str_uninit(&iface->auth.radius.addr);
	str_uninit(&iface->auth.radius.secret);

	ems_split_clear(&iface->auth.bwlist.whitelist);
	ems_split_clear(&iface->auth.bwlist.blackmac);
	ems_split_clear(&iface->auth.bwlist.whitemac);

	ems_free(iface);
}

static ems_int nic_ssid_destroy(ems_nic_wireless *ssid)
{
	if (ssid) {
		nic_ssid_iface_destroy(ssid->_iface);
		if (ssid->_modl) {
			ems_app_module_array_destroy(ssid->_modl);
			ssid->_modl = NULL;
		}

		ems_queue_clear(&ssid->msg_entry, msgqueue, entry, ems_mq_destroy);
		ems_free(ssid);
	}

	return EMS_OK;
}


/*
 */
static ems_int nic_start(ems_nic *nic)
{
	ems_app_process(ty_nic, ty_g_fw, EMS_APP_START, NULL);
	ems_send_message(ty_nic, ty_nic, EMS_APP_EVT_FW_RELOAD, NULL);

	return EMS_OK;
}

static ems_int nic_stop(ems_nic *nic)
{
	ems_queue *p, *q;
	ems_nic_wireless *ssid;

	if (nic->jcfg) {
		json_object_put(nic->jcfg);
		nic->jcfg = NULL;
	}

	ems_app_process(ty_nic, ty_g_fw, EMS_APP_STOP, NULL);

	/* stop all ssid here ..*/
	ems_queue_foreach_safe(&nic->list, p, q) {
		ssid = ems_container_of(p, ems_nic_wireless, entry);
		ems_assert(ssid != NULL);

		ems_queue_remove(p);
		nic_ssid_stop(ssid);
		nic_ssid_destroy(ssid);
	}

	return EMS_OK;
}


static ems_int nic_run(app_module *mod, ems_int run)
{
	ems_nic *nic = (ems_nic *)mod->ctx;
	ems_assert(mod && nic);

	if (run) {
		if (ems_flag_like(mod->flg, FLG_RUN))
			return EMS_OK;

		if (nic_start(nic) == EMS_OK)
			ems_flag_set(mod->flg, FLG_RUN);

	} else {
		if (ems_flag_unlike(mod->flg, FLG_RUN))
			return EMS_OK;

		nic_stop(nic);
		ems_flag_unset(mod->flg, FLG_RUN);
	}

	return EMS_OK;
}

static ems_int nic_handle_ssid_cb(ems_void *arg, json_object *jssid)
{
	ems_nic *nic = (ems_nic *)arg;
	ems_nic_wireless *ssid = NULL;
	ems_int  id, ret, disabled;
	ems_str  ty;

	ems_assert(arg && jssid);
	if (!(arg && jssid && json_object_is_type(jssid, json_type_object)))
		return EMS_ERR;

	str_init(&ty);

	do {
		ret = EMS_OK;

		ems_json_get_string_def(jssid, UCI_SECTION_TYPE, &ty, NULL);

		ems_assert(str_len(&ty) > 0);
		if (str_len(&ty) <= 0 || strcmp(str_text(&ty), "wifi-iface")) {
			break;
		}

		id = nic_json_get_or_set_int(jssid, "ems_iface_id", -1);
		if (id <= 0) {
			ret = EMS_ERR;
			break;
		}

		disabled = nic_json_get_or_set_int(jssid, "disabled",  0);

		ssid = nic_ssid_find(nic, id);
		if (disabled) {
			if (ssid) {
				ems_queue_remove(&ssid->entry);
				nic_ssid_stop(ssid);
				nic_ssid_destroy(ssid);
			}

			break;
		}

		if (!ssid) {
			ssid = nic_ssid_new();
			if (!ssid) {
				ret = EMS_ERR;
				break;
			}

			nic_ssid_do_init(ssid, nic, jssid);
			ems_queue_insert_tail(&nic->list, &ssid->entry);
		}

		nic_ssid_update_cfg(ssid, nic, jssid);
	} while (0);

	str_uninit(&ty);

	return ret;
}

static ems_int nic_reload_wired(ems_nic *nic)
{
	ems_nic_wireless *ssid;

	nic->wired.lan_free = ems_atoi(ems_uci_get("ykwifi.base.ems_lan_free"));
	nic->wired.iface_id = ems_atoi(ems_uci_get("ykwifi.base.ems_lan_bind"));

	ssid = nic_ssid_find(nic, nic->wired.iface_id);
	if (!ssid) {
		ssid = nic_ssid_find(nic, UCI_DEFAULT_ID);
		ems_assert(ssid != NULL);

		if (ssid == NULL) {
			ems_l_trace("[nic] did not find nic id: %d and 1", nic->wired.iface_id);
			return EMS_OK;
		}
	}

	fw_wired_nic_attach(ssid->_iface->id, EMS_NO);

	if (!nic_app_run(ssid, ty_fw))
		return EMS_OK;

	if (   !nic->wired.lan_free    && 
	       !ssid->_iface->disabled && 
		ssid->_iface->auth.enable) {
		fw_wired_nic_attach(ssid->_iface->id, EMS_YES);
	}

	return EMS_OK;
}

static ems_int nic_cfg_fix_cb(ems_void *arg, json_object *jssid)
{
	static ems_int id = 1;
	ems_str  ty;

	ems_assert(jssid != NULL);

	str_init(&ty);

	do {
		ems_json_get_string_def(jssid, UCI_SECTION_TYPE, &ty, NULL);

		ems_assert(str_len(&ty) > 0);
		if (str_len(&ty) <= 0 || strcmp(str_text(&ty), "wifi-iface")) 
		{
			break;
		}

		json_object_object_add(jssid, "ems_iface_id", json_object_new_string(ems_itoa(id++)));
	} while (0);

	str_uninit(&ty);

	return EMS_OK;
}

static ems_int nic_fix_wireless_fix(ems_nic *nic)
{
	json_object *ary;

	nic_stop(nic);

	ary = json_object_new_array();

	if (ems_uci_load_cfg(UCI_CFG_WIRELESS, NULL, ary) == EMS_OK) {
		ems_uci_foreach(ary, nic_cfg_fix_cb, NULL);
		ems_uci_write_cfg(UCI_CFG_WIRELESS, NULL, ary);
	}

	json_object_put(ary);

	nic_start(nic);

	return EMS_OK;
}

static ems_int 
nic_evt_server_rules_upt(ems_nic_wireless *ssid, json_object *root)
{
	json_object *jobj;

	jobj = json_object_object_get(root, "bwlist");
	if (jobj) {
		nic_sendmsg(ssid, ty_nic, ty_bwlist, EMS_APP_SERVER_RULES_UPDATE, jobj);
	}

	jobj = json_object_object_get(root, "http");
	if (jobj) {
		nic_sendmsg(ssid, ty_nic, ty_http, EMS_APP_SERVER_RULES_UPDATE, jobj);
	}

	return EMS_OK;
}

static ems_int 
nic_server_rules_upt_cb(ems_void *arg, json_object *root)
{
	ems_nic   *nic = (ems_nic *)arg;
	ems_int    id;
	ems_str  s_id;
	ems_nic_wireless  *ssid;
	ems_wifi_iface    *iface;


	str_init(&s_id);
	do {
		ems_json_get_string_def(root, "ems_iface_id", &s_id, NULL);
		id = ems_atoi(str_text(&s_id));

		if (id <= 0) break;

		ssid = nic_ssid_find(nic, id);
		if (!ssid) break;

		iface = ssid->_iface;

		if (iface->disabled || !iface->auth.enable)
			break;

		nic_evt_server_rules_upt(ssid, root);
	} while (0);
	str_uninit(&s_id);

	return EMS_OK;
}

static ems_int nic_ssid_fw_reload(ems_void *arg, ems_nic_wireless *ssid)
{
	ems_wifi_iface *iface = NULL;

	ems_assert(ssid && ssid->_iface);

	iface = ssid->_iface;

	ems_assert(!iface->disabled);

	if (iface->disabled || !iface->auth.enable)
		return nic_sendmsg(ssid, ty_nic, ty_fw, EMS_APP_STOP, NULL);

	/* we won't start fw module here, fw started by module portal*/
	if (nic_app_run(ssid, ty_fw)) {
		nic_sendmsg(ssid, ty_nic, ty_fw, EMS_APP_FW_CLEAR, NULL);
		nic_sendmsg(ssid, ty_nic, ty_fw, EMS_APP_EVT_FW_RELOAD, NULL);
	}

	return EMS_OK;
}

static ems_int
nic_fw_reload(app_module *mod, json_object *req)
{
	ems_nic *nic = (ems_nic *)mod->ctx;

	if (nic_reload_wireless_cfg(nic) != EMS_OK)
		return EMS_ERR;

	if (ems_uci_foreach(nic->jcfg, nic_handle_ssid_cb, (ems_void *) nic) != EMS_OK) {
		nic_fix_wireless_fix(nic);
		return EMS_OK;
	}

	if (nic->jserv)
		ems_uci_foreach(nic->jserv, nic_server_rules_upt_cb, (ems_void *)nic);

	return nic_walk_ssid(nic, nic_ssid_fw_reload, NULL);
}

static ems_void nic_handle_ssid_msgs(ems_nic_wireless *ssid)
{
	ems_queue *p;
	msgqueue  *mq;

	while (!ems_queue_empty(&ssid->msg_entry)) {

		p = ems_queue_head(&ssid->msg_entry);
		ems_queue_remove(p);

		mq = ems_container_of(p, msgqueue, entry);

		_ems_app_process(ssid->_modl, mq->s, mq->d, mq->evt, mq->obj);
		ems_mq_destroy(mq);
	}
}

static ems_int
nic_process_msgqueue(ems_nic *nic, json_object *req)
{
	ems_queue        *p;
	ems_nic_wireless *ssid;

	ems_queue_foreach(&nic->list, p) {
		ssid = ems_container_of(p, ems_nic_wireless, entry);

		nic_handle_ssid_msgs(ssid);
	}

	return EMS_OK;
}

static ems_int
nic_server_rules_update(app_module *mod, json_object *req)
{
	ems_nic *nic = (ems_nic *)mod->ctx;

	ems_assert(mod && req && json_object_is_type(req, json_type_array));

	if (nic->jserv) {
		json_object_put(nic->jserv);
		nic->jserv = NULL;
	}

	nic->jserv = ems_json_tokener_parse(json_object_to_json_string(req));

	ems_send_message(ty_nic, ty_nic, EMS_APP_EVT_FW_RELOAD, NULL);

	return EMS_OK;
}

static ems_int nic_evt_bssidinfo(ems_void *arg, ems_nic_wireless *ssid)
{
	json_object     *jobj, *ary;

	ary = (json_object *)arg;

	if (ssid->_iface->disabled)
		return EMS_OK;

	jobj = json_object_new_object();
	json_object_object_add(jobj, "bssid", json_object_new_string(str_text(&ssid->_iface->bssid)));
	json_object_object_add(jobj, "ssid",  json_object_new_string(str_text(&ssid->_iface->ssid)));

	json_object_array_add(ary, jobj);

	return EMS_OK;
}

static ems_int nic_evt_usernumber(ems_void *arg, ems_nic_wireless *ssid)
{
	ems_int *total = (ems_int *)arg;

	*total += nic_processmsg(ssid, ty_nic, ty_radius, EMS_APP_EVT_USERNUMBER, NULL);
	
	return EMS_OK;
}

static ems_int nic_evt_userlist(ems_void *arg, ems_nic_wireless *ssid)
{
	json_object *ary = (json_object *)arg;

	ems_assert(ary && ssid);
	if (!ary)
		return EMS_OK;

	nic_processmsg(ssid, ty_nic, ty_radius, EMS_APP_EVT_USERLIST, ary);
	
	return EMS_OK;
}

static ems_int nic_evt_username(ems_void *arg, ems_nic_wireless *ssid)
{
	json_object *jobj = (json_object *)arg;

	ems_assert(jobj && ssid);
	if (!jobj)
		return EMS_OK;

	if (nic_processmsg(ssid, ty_nic, ty_radius, EMS_APP_EVT_USERNAME, jobj) == EMS_OK)
		return EMS_ERR; /* return error for stop searching user */
	
	return EMS_OK;
}

static ems_int nic_evt_radius_logout(ems_void *arg, ems_nic_wireless *ssid)
{
	json_object *jobj = (json_object *)arg;

	ems_assert(jobj && ssid);
	if (!jobj)
		return EMS_ERR;

	nic_processmsg(ssid, ty_nic, ty_radius, EMS_APP_EVT_USERNAME, jobj);
	
	return EMS_OK;
}

static ems_int
nic_process(app_module *mod, ems_uint s, ems_uint d, ems_uint evt, json_object *root)
{
	if (ems_flag_like(mod->flg, FLG_RUN)) {
		switch(evt) {
		case EMS_APP_PROCESS_MSGQUEUE:
			return nic_process_msgqueue((ems_nic *)mod->ctx, root);

		case EMS_APP_EVT_BSSIDINFO:
			return nic_walk_ssid((ems_nic *)mod->ctx, nic_evt_bssidinfo, (ems_void *)root);
		case EMS_APP_EVT_FW_RELOAD:
			return nic_fw_reload(mod, root);

		case EMS_APP_EVT_WIRED_RELOAD:
			return nic_reload_wired((ems_nic *)mod->ctx);

		default:
			break;
		}
	}

	switch(evt) {
	case EMS_APP_START:
		return nic_run(mod, EMS_YES);

	case EMS_APP_STOP:
		return nic_run(mod, EMS_NO);

	case EMS_APP_SERVER_RULES_UPDATE:
		return nic_server_rules_update(mod, root);

	case EMS_APP_EVT_USERNUMBER:
	{
		ems_int total = 0;
		nic_walk_ssid((ems_nic *)mod->ctx, nic_evt_usernumber, (ems_void *)&total);
		return total;
	}
	case EMS_APP_EVT_USERLIST:
		return nic_walk_ssid((ems_nic *)mod->ctx, nic_evt_userlist, (ems_void *)root);

	case EMS_APP_EVT_USERNAME:
		nic_walk_ssid((ems_nic *)mod->ctx, nic_evt_username, (ems_void *)root);
		return EMS_OK;

	case EMS_APP_CMD_RADIUS_LOGOUT:
		return nic_walk_ssid((ems_nic *)mod->ctx, nic_evt_radius_logout, (ems_void *)root);

	default:
		break;
	}

	return EMS_OK;
}

ems_int nic_sendmsg(ems_nic_wireless *ssid, 
		ems_uint s, ems_uint d, ems_uint evt, json_object *obj)
{
	return _ems_send_message(&ssid->msg_entry, s, d, evt, obj);
}

ems_int nic_processmsg(ems_nic_wireless *ssid, 
		ems_uint s, ems_uint d, ems_uint evt, json_object *obj)
{
	return _ems_app_process(ssid->_modl, s, d, evt, obj);
}

ems_int nic_app_run(ems_nic_wireless *ssid, ems_app_type app)
{
	return _ems_app_run(ssid->_modl, app);
}


app_module app_nic = 
{
	.ty      = ty_nic,
	.desc    = ems_string("nic"),
	.ctx     = NULL,
	.flg     = 0,

	.init    = nic_init,
	.uninit  = nic_uninit,
	.run     = nic_run,
	.process = nic_process,
	.process_rule  = NULL,
	.version_match = NULL,
	.install       = NULL
};


