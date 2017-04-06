
#include "ems_core.h"
#include "ems_client.h"
#include "ems_cmd.h"
#include "ems_radius.h"
#include "app.h"
#include "ems_ctrl.h"

static ems_ctrl *ctrl_new()
{
	return (ems_ctrl *)ems_malloc(sizeof(ems_ctrl));
}

static ems_void ctrl_destroy(ems_ctrl *ctrl)
{
	if (ctrl)
		ems_free(ctrl);
}

static ems_int ctrl_init(app_module *mod)
{
	ems_ctrl  *ctrl = NULL;

	ctrl = ctrl_new();
	if (ctrl) {
		memset(ctrl, 0, sizeof(ems_ctrl));

		ctrl->sess = NULL;
		ctrl->st   = st_stopped;
		ems_queue_init(&ctrl->cmd);

		mod->ctx = (ems_void *)ctrl;
	}

	return EMS_OK;
}

static ems_int ctrl_uninit(app_module *mod)
{
	ems_ctrl  *ctrl = (ems_ctrl *)mod->ctx;

	if (!ctrl)
		return EMS_OK;

	mod->ctx = NULL;

	ems_assert(ctrl && ctrl->st == st_stopped);
	ems_assert(ctrl->sess == NULL);
	ems_assert(ems_queue_empty(&ctrl->cmd));

	if (ctrl->sess) {
		ems_session_shutdown_and_destroy(ctrl->sess);
		ctrl->sess = NULL;
	}

	ems_queue_clear(&ctrl->cmd, ems_session, entry, ems_session_shutdown_and_destroy);
	ctrl->st   = st_max;

	ctrl_destroy(ctrl);

	return EMS_OK;
}

static ems_int ctrl_run(app_module *mod, ems_int run)
{
	ems_ctrl  *ctrl = (ems_ctrl *)mod->ctx;

	ems_assert(ctrl != NULL);

	if (run) {
		if (ems_flag_like(mod->flg, FLG_RUN))
			return EMS_OK;

		ems_l_trace("ctrl running");
		ems_flag_set(mod->flg, FLG_RUN);
		ctrl_change_status(ctrl, st_start);
	} else {
		if (ems_flag_unlike(mod->flg, FLG_RUN))
			return EMS_OK;

		ems_l_trace("ctrl stopping");
		ems_flag_unset(mod->flg, FLG_RUN);
		ctrl_change_status(ctrl, st_stopped);
	}

	return EMS_OK;
}

static ems_int
ctrl_process(app_module *mod, ems_uint s, ems_uint d, ems_uint evt, json_object *root)
{
	ems_l_trace("ctrl evt: 0x%x, from: 0x%x, args: %s", 
			evt, s, root?json_object_to_json_string(root):"");

	switch(evt) {
	case EMS_APP_START:
		return ctrl_run(mod, EMS_YES);

	case EMS_APP_STOP:
		return ctrl_run(mod, EMS_NO);

	default:
		break;
	}

	return EMS_OK;
}

app_module app_ctrl = 
{
	.ty      = ty_ctrl,
	.desc    = ems_string("ctrl"),
	.ctx     = NULL,
	.flg     = 0,

	.init    = ctrl_init,
	.uninit  = ctrl_uninit,
	.run     = ctrl_run,
	.process = ctrl_process,
	.process_rule  = NULL,
	.version_match = NULL,
	.install       = NULL
};
