

#include "ems_core.h"
#include "ems_client.h"
#include "app.h"
#include "ems_radius.h"
#include "ems_fw.h"

static ems_device *radius_find_dev(ems_radius *ra, ems_cchar *ip)
{
	ems_queue  *p;
	ems_device *dev;

	ems_queue_foreach(&ra->dev_entry, p) {
		dev = ems_container_of(p, ems_device, entry);

		if (!strcmp(str_text(&dev->user.ip), ip))
			return dev;
	}

	return NULL;
}

static rc_handle *radius_create_handle()
{
	ems_cchar  *fl;
	rc_handle *rh = NULL;

	rh = rc_new();
	if (!rh)
		return NULL;

	rc_config_init(rh);

	fl = cfg_get(emscfg(), CFG_radius_dict);
	if (!fl) {
		cfg_set(emscfg(), CFG_radius_dict, "/usr/ems/conf/dictionary");
		fl = cfg_get(emscfg(), CFG_radius_dict);
	}

	if (rc_read_dictionary(rh, fl) != 0) {
		rc_destroy(rh);
		return NULL;
	}

	return rh;
}

static ems_int radius_start(ems_radius *ra)
{
	ems_cfg   *cfg = emscfg();
	ems_cchar *val = NULL;

	val = cfg_get(cfg, CFG_app_radius_addr);

	if (val) {
		str_set(&ra->auth_addr, val);
		str_set(&ra->acct_addr, val);
	} else {
		ems_assert(0 && "never be here");
		ems_l_trace("radius start failed, %s missing", CFG_app_radius_addr);
		return EMS_ERR;
	}

	ra->auth_port = ems_atoi(cfg_get(cfg, CFG_app_radius_port_auth));
	ra->acct_port = ems_atoi(cfg_get(cfg, CFG_app_radius_port_acct));

	val = cfg_get(cfg, CFG_app_radius_shared_key);
	if (val)
		str_set(&ra->secret, val);

	val = cfg_get(cfg, CFG_app_radius_retry_times);
	if (val)
		ra->retry_times = ems_atoi(val);

	val = cfg_get(cfg, CFG_app_radius_retry_timeout);
	if (val)
		ra->retry_timeout = ems_atoi(val);

	val = cfg_get(cfg, CFG_app_radius_report_period);
	if (val)
		ra->report_period = ems_atoi(val);

	val = cfg_get(cfg, CFG_app_radius_disconnect);
	if (val)
		ra->disconnect = ems_atoi(val);

	ra->lasterr = 0;

	return EMS_OK;
}

static ems_int radius_stop(ems_radius *ra)
{
	ems_queue   *p;
	ems_device  *dev;

	ra->lasterr = 0;
	ems_queue_foreach(&ra->dev_entry, p) {
		dev = ems_container_of(p, ems_device, entry);
		dev->err = 7; /* RFC2866 p20, Admin Reboot */
		dev_change_status(dev, st_acct_stop);
	}

	return EMS_OK;
}

static ems_radius *radius_new()
{
	return (ems_radius *)ems_malloc(sizeof(ems_radius));
}

static ems_void  radius_destroy(ems_radius *ra)
{
	if (ra)
		ems_free(ra);
}

static ems_int radius_init(ems_radius *ra)
{
	ems_assert(ra != NULL);
	memset(ra, 0, sizeof(ems_radius));

	ems_queue_init(&ra->dev_entry);
	ra->seq_nbr = time(NULL);
	str_init(&ra->secret);
	str_init(&ra->auth_addr);
	str_init(&ra->acct_addr);

	ra->retry_times   = 3;
	ra->retry_timeout = 10;
	ra->report_period = 300;
	ra->disconnect    = 300;

	ra->rh = radius_create_handle();
	if (!ra->rh)
		return EMS_ERR;

	return EMS_OK;
}

static ems_int radius_uninit(ems_radius *ra)
{
	ems_queue  *p, *q;
	ems_device *dev;

	if (!ra)
		return EMS_OK;

	if (ra->rh) {
		rc_destroy(ra->rh);
		ra->rh = NULL;
	}

	ems_queue_foreach_safe(&ra->dev_entry, p, q) {
		dev = ems_container_of(p, ems_device, entry);
		dev_change_status(dev, st_err);
	}

	str_uninit(&ra->secret);
	str_uninit(&ra->auth_addr);
	str_uninit(&ra->acct_addr);

	return EMS_OK;
}

static ems_int radius_cfg_init()
{
	ems_cfg *cfg = emscfg();
	if (!cfg_get(cfg, CFG_app_radius_addr))
		cfg_set(cfg, CFG_app_radius_addr, "zuhu-pa.lekewifi.com");

	if (!cfg_get(cfg, CFG_app_radius_port_auth))
		cfg_set(cfg, CFG_app_radius_port_auth, ems_itoa(1812));

	if (!cfg_get(cfg, CFG_app_radius_port_acct))
		cfg_set(cfg, CFG_app_radius_port_acct, ems_itoa(1813));

	if (!cfg_get(cfg, CFG_app_radius_shared_key))
		cfg_set(cfg, CFG_app_radius_shared_key, "admin");

	if (!cfg_get(cfg, CFG_app_radius_report_period))
		cfg_set(cfg, CFG_app_radius_report_period, ems_itoa(300));

	if (!cfg_get(cfg, CFG_app_radius_disconnect))
		cfg_set(cfg, CFG_app_radius_disconnect, ems_itoa(300));

	if (!cfg_get(cfg, CFG_app_radius_auto))
		cfg_set(cfg, CFG_app_radius_auto, ems_itoa(1));

	if (!cfg_get(cfg, CFG_app_radius_retry_times))
		cfg_set(cfg, CFG_app_radius_retry_times, ems_itoa(3));

	if (!cfg_get(cfg, CFG_app_radius_retry_timeout))
		cfg_set(cfg, CFG_app_radius_retry_timeout, ems_itoa(5));

	return EMS_OK;
}


static ems_int ra_init(app_module *mod)
{
	radius_cfg_init();

	mod->ctx = (ems_void *)radius_new();
	if (!mod->ctx)
		return EMS_ERR;

	radius_init((ems_radius *)mod->ctx);

	return EMS_OK;
}

static ems_int ra_uninit(app_module *mod)
{
	ems_radius *ra = (ems_radius *)mod->ctx;

	if (ra) {
		mod->ctx = NULL;

		radius_uninit(ra);
		radius_destroy(ra);
		ra = NULL;
	}

	return EMS_OK;
}

static ems_int radius_get_rules(app_module *mod, json_object *root)
{
	ems_str    buf;
	ems_uint   tmp;
	ems_cfg   *cfg = emscfg();

	ems_l_trace("radius process rule: %s", json_object_to_json_string(root));

	if (!json_object_is_type(root, json_type_object))
		return EMS_OK;

	str_init(&buf);

	ems_json_get_string_def(root, "addr", &buf, NULL);
	if (str_len(&buf) > 0)
		cfg_set(cfg, CFG_app_radius_addr, str_text(&buf));

	ems_json_get_int_def(root, "auth_port", tmp, 0);
	if (tmp > 0)
		cfg_set(cfg, CFG_app_radius_port_auth, ems_itoa(tmp));

	ems_json_get_int_def(root, "acct_port", tmp, 0);
	if (tmp > 0)
		cfg_set(cfg, CFG_app_radius_port_acct, ems_itoa(tmp));

	ems_json_get_string_def(root, "secret", &buf, NULL);
	if (str_len(&buf) > 0)
		cfg_set(cfg, CFG_app_radius_shared_key, str_text(&buf));

	ems_json_get_int_def(root, "report_period", tmp, 0);
	if (tmp > 0)
		cfg_set(cfg, CFG_app_radius_report_period, ems_itoa(tmp));

	ems_json_get_int_def(root, "disconnect", tmp, 0);
	if (tmp > 0)
		cfg_set(cfg, CFG_app_radius_disconnect, ems_itoa(tmp));

	str_uninit(&buf);

	ems_l_trace("radius rule: addr: %s: port auth: %s, port count: %s shared key: %s, report period: %s, disconnect: %s",
			cfg_get(cfg, CFG_app_radius_addr),
			cfg_get(cfg, CFG_app_radius_port_auth),
			cfg_get(cfg, CFG_app_radius_port_acct),
			cfg_get(cfg, CFG_app_radius_shared_key),
			cfg_get(cfg, CFG_app_radius_report_period),
			cfg_get(cfg, CFG_app_radius_disconnect)
			);
	return EMS_OK;
}

ems_device *ems_device_new()
{
	ems_device *dev = NULL;

	dev = (ems_device *)ems_malloc(sizeof(ems_device));
	if (dev) {
		memset(dev, 0, sizeof(ems_device));
		ems_queue_init(&dev->entry);

		dev->sess_dev = NULL;
		dev->sess     = NULL;
		dev->ctx      = NULL;
		str_init(&dev->user.name);
		str_init(&dev->user.pass);
		str_init(&dev->user.mac);
		str_init(&dev->user.ip);
		str_init(&dev->user.sessid);

		dev->user.in_bytes  = 0;
		dev->user.out_bytes = 0;
		dev->user.in_pkgs   = 0;
		dev->user.out_pkgs  = 0;

		dev->retry_times   = 0;
		dev->retry_timeout = 0;
		dev->reason        = 0;
		dev->err           = 0;
		dev->vp_out = NULL;
		dev->vp_in  = NULL;
		dev->auth_out = NULL;
		dev->auth_in  = NULL;

		dev->st   = st_stopped;
	}

	return dev;
}

ems_void ems_device_destroy(ems_device *dev)
{
	if (dev) {
		if (dev->vp_out) {
			rc_avpair_free(dev->vp_out);
			dev->vp_out = NULL;
		}

		if (dev->vp_in) {
			rc_avpair_free(dev->vp_in);
			dev->vp_in = NULL;
		}

		str_uninit(&dev->user.name);
		str_uninit(&dev->user.pass);
		str_uninit(&dev->user.mac);
		str_uninit(&dev->user.ip);
		str_uninit(&dev->user.sessid);

		if (dev->sess_dev) {
			ems_session_shutdown_and_destroy(dev->sess_dev);
			dev->sess_dev = NULL;
		}

		if (dev->sess) {
			ems_session_shutdown_and_destroy(dev->sess);
			dev->sess = NULL;
		}

		ems_free(dev);
	}
}

static ems_int 
radius_user_in(ems_radius *ra, ems_str *name, ems_str *pass, ems_str *ip, ems_str *mac)
{
	ems_device *dev = ems_device_new();
	ems_l_trace("user user in : %s, %s, %s, %s", 
			str_text(name), str_text(pass),str_text(ip), str_text(mac));

	if (!dev)
		return EMS_ERR;

	str_cpy(&dev->user.name, name);
	str_cpy(&dev->user.pass, pass);
	str_cpy(&dev->user.ip,   ip);
	str_cpy(&dev->user.mac,  mac);

	dev->ctx = ra;
	dev->disconnect = ra->disconnect / (ra->report_period + 6) + 1;

	ems_queue_insert_tail(&ra->dev_entry, &dev->entry);

	return dev_change_status(dev, st_start);
}

static ems_int radius_cmd_auth(ems_radius *ra, json_object *root)
{
	ems_int rtn;
	ems_str name, pass, ip, mac;
	ems_device   *dev;

	str_init(&name);
	str_init(&pass);
	str_init(&ip);
	str_init(&mac);

	do {
		rtn = EMS_ERR;

		ems_json_get_string_def(root, "username", &name, NULL);
		if (str_len(&name) <= 0) break;

		ems_json_get_string_def(root, "userpass", &pass, NULL);
		if (str_len(&pass) <= 0) break;

		ems_json_get_string_def(root, "userip",   &ip, NULL);
		if (str_len(&ip) <= 0) break;

		ems_json_get_string_def(root, "usermac",  &mac, NULL);
		if (str_len(&mac) <= 0) break;

		dev = radius_find_dev(ra, str_text(&ip));
		if (dev) {

			if (!strcmp(str_text(&dev->user.mac), str_text(&mac))) {

				str_set(&dev->user.name, str_text(&name));
				str_set(&dev->user.pass, str_text(&pass));
				rtn = dev_change_status(dev, st_start);
				break;
			}

			ems_l_trace("ip(%s) user mac changed: %s --> %s",
					str_text(&ip), str_text(&dev->user.mac), str_text(&mac));
			dev->err = 2; /* RFC2866 p20, */
			dev_change_status(dev, st_acct_stop);
		}

		rtn = radius_user_in(ra, &name, &pass, &ip, &mac);
	} while (0);

	str_uninit(&name);
	str_uninit(&pass);
	str_uninit(&ip);
	str_uninit(&mac);

	return rtn;
}

static ems_int radius_cmd_logout(ems_radius *ra, json_object *root)
{
	ems_int rtn;
	ems_str name, ip, mac;
	ems_device *dev = NULL;

	str_init(&name);
	str_init(&ip);
	str_init(&mac);

	do {
		rtn = EMS_ERR;

		ems_json_get_string_def(root, "username", &name, NULL);
		if (str_len(&name) <= 0) break;

		ems_json_get_string_def(root, "userip",   &ip,  NULL);
		if (str_len(&ip) <= 0) break;

		ems_json_get_string_def(root, "usermac",  &mac, NULL);
		if (str_len(&mac) <= 0) break;

		dev = radius_find_dev(ra, str_text(&ip));
		if (dev) {
			if (!strcmp(str_text(&dev->user.mac), str_text(&mac))) {
				ems_l_trace("ip(%s) user logout", str_text(&ip));
				dev->err = 1; /* RFC2866, page 20 user request maybe */
				dev_change_status(dev, st_acct_stop);
			}
		}

		rtn = EMS_OK;
	} while (0);

	str_uninit(&name);
	str_uninit(&ip);
	str_uninit(&mac);

	return rtn;
}

static ems_int radius_evt_fw_reload(ems_radius *ra, json_object *root)
{
	ems_queue  *p;
	ems_device *dev;
	json_object *obj;
	obj = json_object_new_object();

	ems_queue_foreach(&ra->dev_entry, p) {
		dev = ems_container_of(p, ems_device, entry);

		json_object_object_add(obj, "userip", 
				json_object_new_string(str_text(&dev->user.ip)));
		json_object_object_add(obj, "usermac", 
				json_object_new_string(str_text(&dev->user.mac)));
		json_object_object_add(obj, "add",    
				json_object_new_int(1));

		ems_app_process(ty_radius, ty_fw, EMS_APP_FW_RADIUS_DEVICE_FREE, obj);
	}

	json_object_put(obj);

	return EMS_OK;
}


static ems_int radius_rules_update(ems_radius *ra, json_object *req, ems_int run)
{
	ems_uint  tmp, restart;
	ems_str   buf;
	ems_cfg  *cfg  = emscfg();

	str_init(&buf);

	restart = 0;
	ems_json_get_string_def(req, "addr", &buf, NULL);
	if (str_len(&buf) > 0) {
		cfg_set(cfg, CFG_app_radius_addr, str_text(&buf));

		if (!restart && 
			(str_len(&buf) != str_len(&ra->auth_addr) || 
			 strcmp(str_text(&buf), str_text(&ra->auth_addr))
			)
		) {
			restart = 1;
		}
	}

	ems_json_get_string_def(req, "secret", &buf, NULL);
	if (str_len(&buf) > 0) {
		cfg_set(cfg, CFG_app_radius_shared_key, str_text(&buf));
		if (!restart && 
		        (str_len(&buf)!= str_len(&ra->secret) || 
			 strcmp(str_text(&buf), str_text(&ra->secret))
			 )
		) {
			restart = 1;
		}
	}

	ems_json_get_int_def(req, "authport", tmp, 1812);
	cfg_set(cfg, CFG_app_radius_port_auth, ems_itoa(tmp));
	if (!restart && tmp != ra->auth_port) restart = 1;

	ems_json_get_int_def(req, "acctport", tmp, 1813);
	cfg_set(cfg, CFG_app_radius_port_acct, ems_itoa(tmp));
	if (!restart && tmp != ra->acct_port) restart = 1;

	ems_json_get_int_def(req, "rp_period", tmp, 300);
	cfg_set(cfg, CFG_app_radius_report_period, ems_itoa(tmp));
	ra->report_period = tmp;

	ems_json_get_int_def(req, "disconnect", tmp, 0);
	if (tmp > 0) {
		cfg_set(cfg, CFG_app_radius_disconnect, ems_itoa(tmp));
		ra->disconnect = tmp;
	}

	ems_json_get_int_def(req, "retry_times", tmp, 3);
	cfg_set(cfg, CFG_app_radius_retry_times, ems_itoa(tmp));
	ra->retry_times = tmp;

	ems_json_get_int_def(req, "retry_timeout", tmp, 5);
	cfg_set(cfg, CFG_app_radius_retry_timeout, ems_itoa(tmp));
	ra->retry_timeout = tmp;

	str_uninit(&buf);

	if (run && restart) {
		ems_l_trace("radius restart...");
		radius_stop(ra);
		radius_start(ra);
	}

	return EMS_OK;
}

static ems_int radius_evt_rules_update(ems_radius *ra, json_object *req, ems_int run)
{
	ems_uint  tmp;
	ems_core *core = emscorer();

	ems_json_get_int_def(req, "auto", tmp, 1);
	if (tmp)
		ems_flag_set(core->flg, FLG_RADIUS_AUTO);
	else
		ems_flag_unset(core->flg, FLG_RADIUS_AUTO);

	cfg_set(emscfg(), CFG_app_radius_auto, ems_itoa(tmp));

	if (tmp)
		return EMS_OK;

	return radius_rules_update(ra, req, run);
}

static ems_int radius_evt_server_rules_update(ems_radius *ra, json_object *req)
{
	if (!ems_atoi(cfg_get(emscfg(), CFG_app_radius_auto))) {
		ems_l_trace("use user defined configs...");
		return EMS_OK;
	}

	return radius_rules_update(ra, req, EMS_YES);
}


static ems_int radius_evt_status(ems_radius *ra, json_object *root)
{
	switch(ra->lasterr) {
	case RADIUS_ERR_CANNOT_CONNECT:
	case RADIUS_ERR_REJECT:
	case RADIUS_ERR_NETWORK:
		return ra->lasterr;

	default:
		break;
	}

	/* normal */
	return 0;
}

static ems_int ra_run(app_module *mod, ems_int run);

static ems_int 
ra_process(app_module *mod, ems_uint s, ems_uint d, ems_uint evt, json_object *root)
{
	ems_radius *ra = (ems_radius *) mod->ctx;

	ems_l_trace("radius got evt: 0x%x, args: %s", evt, json_object_to_json_string(root));

	switch(evt) {
	case EMS_APP_STOP:
		return ra_run(mod, EMS_NO);

	case EMS_APP_START:
		return ra_run(mod, EMS_YES);

	case EMS_APP_RULES_UPDATE:
		return radius_evt_rules_update(ra, root, ems_flag_like(mod->flg, FLG_RUN));

	default:
		break;
	}

	if (ems_flag_unlike(mod->flg, FLG_RUN))
		return EMS_ERR;

	ems_assert(mod->ctx && ra);

	if (!ra->rh) {
		ems_l_warn("radius config file missing...");
		return EMS_ERR;
	}

	switch(evt) {
	case EMS_APP_CMD_RADIUS_AUTH:
		return radius_cmd_auth(ra, root);

	case EMS_APP_CMD_RADIUS_LOGOUT:
		return radius_cmd_logout(ra, root);

	case EMS_APP_EVT_FW_RELOAD:
		return radius_evt_fw_reload(ra, root);

	case EMS_APP_SERVER_RULES_UPDATE:
		return radius_evt_server_rules_update(ra, root);

	case EMS_APP_EVT_STATUS:
		return radius_evt_status(ra, root);

	default:
		break;
	}

	return EMS_OK;
}

static ems_int ra_process_rules(app_module *mod, json_object *root)
{
#ifdef DEBUG
	ems_radius  *ra = (ems_radius *)mod->ctx;
#endif
	json_object *obj;

	ems_assert(mod && root && ra);

	ems_l_trace("radius app info: %s", json_object_to_json_string(root));

	obj = json_object_object_get(root, "rsrv");

	if (obj && radius_get_rules(mod, obj) == EMS_OK) 
	{
		/* flush radius here */
		ems_l_trace("update radius env here");
	}

	return EMS_OK;
}

static ems_int ra_run(app_module *mod, ems_int run)
{
	ems_radius *ra = (ems_radius *)mod->ctx;
	ems_assert(mod && ra);

	if (run) {
		if (ems_flag_like(mod->flg, FLG_RUN))
			return EMS_OK;

		ems_l_trace("radius running here");
		if (radius_start(ra) == EMS_OK)
			ems_flag_set(mod->flg, FLG_RUN);

	} else {
		if (ems_flag_unlike(mod->flg, FLG_RUN))
			return EMS_OK;

		ems_l_trace("radius stopped");
		radius_stop(ra);
		ems_flag_unset(mod->flg, FLG_RUN);
	}

	return EMS_OK;
}


ems_cchar *radius_username(app_module *mod, ems_cchar *ip)
{
	ems_device *dev;

	if (!(mod && mod->ctx && ip))
		return NULL;
	
	dev = radius_find_dev((ems_radius *)mod->ctx, ip);

	if (dev)
		return str_text(&dev->user.name);

	return NULL;
}

ems_int radius_online_user_number(app_module *mod)
{
	ems_int     n;
	ems_radius *ra = (ems_radius *)mod->ctx;
	ems_queue  *p;

	if (!(ra && mod && mod->ctx))
		return 0;
	
	n = 0;
	ems_queue_foreach(&ra->dev_entry, p) {
		n++;
	}

	return n;
}

json_object *radius_userlist(app_module *mod)
{
	json_object *root, *ary, *obj;
	ems_radius  *ra = (ems_radius *)mod->ctx;
	ems_queue   *p;
	ems_device  *dev;

	if (!(ra && mod && mod->ctx))
		return 0;

	root = json_object_new_object();
	ary  = json_object_new_array();

	json_object_object_add(root, "user", ary);
	
	ems_queue_foreach(&ra->dev_entry, p) {
		dev = ems_container_of(p, ems_device, entry);

		obj = json_object_new_object();
		json_object_object_add(obj, "nick", json_object_new_string(str_text(&dev->user.name)));
		json_object_object_add(obj, "pwd",  json_object_new_string(str_text(&dev->user.pass)));
		json_object_object_add(obj, "ip",   json_object_new_string(str_text(&dev->user.ip)));
		json_object_object_add(obj, "mac",  json_object_new_string(str_text(&dev->user.mac)));
		json_object_object_add(obj, "status", json_object_new_int(dev->st));

		json_object_array_add(ary, obj);
	}

	return root;
}


app_module app_radius = {

	.ty      = ty_radius,
	.desc    = ems_string("radius"),
	.ctx     = NULL,
	.flg     = 0,

	.init    = ra_init,
	.uninit  = ra_uninit,
	.run     = ra_run,
	.process = ra_process,
	.process_rule = ra_process_rules,
	.version_match = NULL,
	.install       = NULL
};
