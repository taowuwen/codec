
#include "ems_core.h"
#include "ems_client.h"
#include "app.h"
#include "ems_fw.h"
#include "ems_dns.h"
#include "ems_bridge.h"

static ems_int br_init(app_module *mod)
{
	ems_bridge *br = NULL;

	br = (ems_bridge *)ems_malloc(sizeof(ems_bridge));

	if (br) {
		memset(br, 0, sizeof(ems_bridge));

		br->st   = st_stopped;
		br->sess = NULL;

		mod->ctx = (ems_void *)br;
	}
	
	return EMS_OK;
}

static ems_int br_uninit(app_module *mod)
{
	ems_bridge *br = (ems_bridge *)mod->ctx;

	if (!br)
		return EMS_OK;

	mod->ctx = NULL;

	ems_assert(br->st == st_stopped);
	ems_assert(br->sess == NULL);

	if (br->st != st_stopped)
		br_change_status(br, st_stopped);

	if (br->sess) {
		ems_session_shutdown_and_destroy(br->sess);
		br->sess = NULL;
	}

	br->st = st_max;
	ems_free(br);

	return EMS_OK;
}

static ems_int br_run(app_module *mod, ems_int run)
{
	ems_bridge *br = (ems_bridge *)mod->ctx;

	if (run) {
		if (ems_flag_like(mod->flg, FLG_RUN))
			return EMS_OK;

		ems_l_trace("bridge starting...");
		ems_flag_set(mod->flg, FLG_RUN);
		ems_flag_unset(emscorer()->flg, FLG_NETWORK_BRIDGE);
		return br_change_status(br, st_start);
	} else {
		if (ems_flag_unlike(mod->flg, FLG_RUN))
			return EMS_OK;

		ems_l_trace("bridge stopping...");
		ems_flag_unset(mod->flg, FLG_RUN);
		return br_change_status(br, st_stopped);
	}

	return EMS_OK;
}

static ems_int
br_process(app_module *mod, ems_uint s, ems_uint d, ems_uint evt, json_object *root)
{
	ems_l_trace("br evt: 0x%x, from: 0x%x, args: %s", 
			evt, s, root?json_object_to_json_string(root):"");

	switch(evt) {
	case EMS_APP_START:
		return br_run(mod, EMS_YES);

	case EMS_APP_STOP:
		return br_run(mod, EMS_NO);

	default:
		break;
	}

	return EMS_OK;
}

app_module app_bridge = 
{
	.ty      = ty_bridge,
	.desc    = ems_string("bridge"),
	.ctx     = NULL,
	.flg     = 0,

	.init    = br_init,
	.uninit  = br_uninit,
	.run     = br_run,
	.process = br_process,
	.process_rule  = NULL,
	.version_match = NULL,
	.install       = NULL
};
