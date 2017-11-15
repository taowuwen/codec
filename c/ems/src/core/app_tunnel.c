
#include "ems_core.h"
#include "ems_client.h"
#include "app.h"
#include "ems_dns.h"
#include "ems_tunnel.h"

static ems_int tunnel_init(app_module *mod)
{
	ems_tunnel *tunnel;

	ems_assert(mod);

	tunnel = (ems_tunnel *)ems_malloc(sizeof(ems_tunnel));
	if (!tunnel)
		return EMS_ERR;

	memset(tunnel, 0, sizeof(ems_tunnel));
	tunnel->sess = NULL;
	tunnel->st   = st_stopped;
	ems_hash_init(&tunnel->hash_sess, 32);
	ems_queue_init(&tunnel->list_sess);

	str_init(&tunnel->addr);
	tunnel->port = 0;
	tunnel->hb   = 0;
	tunnel->enable = EMS_NO;
	tunnel->flg    = 0;

	mod->ctx = (ems_void *)tunnel;

	return EMS_OK;
}

static ems_int tunnel_uninit(app_module *mod)
{
	ems_tunnel *tunnel = (ems_tunnel *)mod->ctx;

	ems_assert(tunnel != NULL);

	if (tunnel) {

		ems_assert(tunnel->sess == NULL);
		ems_assert(tunnel->st == st_stopped);
		ems_assert(ems_queue_empty(&tunnel->list_sess));

		str_uninit(&tunnel->addr);
		ems_hash_uninit(&tunnel->hash_sess);

		ems_free(tunnel);
	}

	mod->ctx = NULL;
	return EMS_OK;
}

static ems_int tunnel_do_start(ems_tunnel *tunnel)
{
	tunnel->enable = ems_atoi(cfg_get(emscfg(), CFG_tunnel_enable));
	if (!tunnel->enable) 
		goto stop_tunnel;

	str_set(&tunnel->addr,  cfg_get(emscfg(), CFG_tunnel_address));
	tunnel->port = ems_atoi(cfg_get(emscfg(), CFG_tunnel_port));
	tunnel->hb   = ems_atoi(cfg_get(emscfg(), CFG_tunnel_heartbeat));

	if (str_len(&tunnel->addr) <= 0)
		goto stop_tunnel;

	if (tunnel->port <= 0)
		goto stop_tunnel;

	if (tunnel->hb <= 0)
		tunnel->hb = 30;

	return tunnel_change_status(tunnel, st_init);

stop_tunnel:
	ems_l_trace("[tunnel: %s] start tunnel failed: <%s:%d, hb: %d>",
			tunnel->enable?"enabled":"disabled",
			str_text(&tunnel->addr), tunnel->port, tunnel->hb);

	ems_send_message(ty_tunnel, ty_tunnel, EMS_APP_STOP, NULL);
	return EMS_OK;
}

static ems_int tunnel_do_stop(ems_tunnel *tunnel)
{
	ems_assert(tunnel != NULL);

	ems_l_info("[tunnel: %s] do stopped: (%s:%d, hb: %d)",
			tunnel->enable?"enabled":"disabled",
			str_text(&tunnel->addr), tunnel->port, tunnel->hb);

	if (tunnel->st != st_stopped)
		tunnel_change_status(tunnel, st_stopped);
	return EMS_OK;
}

static ems_int tunnel_run(app_module *mod, ems_int run)
{
	ems_tunnel *tunnel = (ems_tunnel *)mod->ctx;
	ems_assert(mod && tunnel);

	if (run) {
		if (ems_flag_like(mod->flg, FLG_RUN))
			return EMS_OK;

		if (tunnel_do_start(tunnel) == EMS_OK)
			ems_flag_set(mod->flg, FLG_RUN);
	} else {
		if (ems_flag_unlike(mod->flg, FLG_RUN))
			return EMS_OK;

		tunnel_do_stop(tunnel);
		ems_flag_unset(mod->flg, FLG_RUN);
	}

	return EMS_OK;
}

static ems_int tunnel_process_rule(app_module *mod, json_object *root)
{
	return EMS_OK;
}


/*
   {
   	"enable":
	"address":
	"port":
	"heartbeat":
   }
   
 */
static ems_int 
tunnel_server_rules_update(app_module *mod, json_object *root)
{
	ems_str buf;
	ems_int tmp, enable;

	ems_tunnel *tunnel = (ems_tunnel *)mod->ctx;

	if (!root)
		return EMS_OK;

	str_init(&buf);

	ems_json_get_string_def(root, "address", &buf, NULL);
	cfg_set(emscfg(), CFG_tunnel_address, str_text(&buf));

	ems_json_get_int_def(root, "port", tmp, tunnel->port);
	cfg_set(emscfg(), CFG_tunnel_port, ems_itoa(tmp));

	ems_json_get_int_def(root, "heartbeat", tmp, tunnel->hb);
	cfg_set(emscfg(), CFG_tunnel_heartbeat, ems_itoa(tmp));

	ems_json_get_int_def(root, "enable", enable, tunnel->enable);
	cfg_set(emscfg(), CFG_tunnel_enable, ems_itoa(enable));

	str_uninit(&buf);

	ems_send_message(ty_tunnel, ty_tunnel, EMS_APP_STOP,  NULL);

	if (enable)
		ems_send_message(ty_tunnel, ty_tunnel, EMS_APP_START, NULL);

	return EMS_OK;
}

static ems_int
tunnel_process(app_module *mod, ems_uint s, ems_uint d, ems_uint evt, json_object *root)
{
	switch(evt) {
	case EMS_APP_SERVER_RULES_UPDATE:
		return tunnel_server_rules_update(mod, root);

	case EMS_APP_START:
		return tunnel_run(mod, EMS_YES);

	case EMS_APP_STOP:
		return tunnel_run(mod, EMS_NO);

	default:
		break;
	}

	return EMS_OK;
}


app_module app_tunnel = 
{
	.ty      = ty_tunnel,
	.desc    = ems_string("tunnel"),
	.ctx     = NULL,
	.flg     = 0,

	.init    = tunnel_init,
	.uninit  = tunnel_uninit,
	.run     = tunnel_run,
	.process = tunnel_process,
	.process_rule  = tunnel_process_rule,
	.version_match = NULL,
	.install       = NULL
};
