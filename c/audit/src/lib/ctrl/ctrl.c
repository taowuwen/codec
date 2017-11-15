
#include "audit.h"
#include "ems_client.h"
#include "ctrl.h"

static ems_uint ctrl_id() 
{
	return mod_ctrl;
}

static ems_uint ctrl_type() 
{
	return MODULE_TYPE_IN;
}

static ems_cchar *ctrl_nick() 
{
	return "ctrl";
}

static ems_int ctrl_init(audit_class *cls) 
{
	audit_ctrl  *ctrl = NULL;

	ems_l_trace("ctrl do init...");

	ctrl = (audit_ctrl *)ems_malloc(sizeof(audit_ctrl));
	if (!ctrl)
		return EMS_ERR;

	memset(ctrl, 0, sizeof(audit_ctrl));
	ctrl->sess = NULL;
	ctrl->st   = st_stop;
	ems_queue_init(&ctrl->cmd);

	cls->ctx = (ems_void *)ctrl;

	return EMS_OK;
}


static ems_int ctrl_run(audit_class *cls, ems_int run)
{
	audit_ctrl *ctrl = (audit_ctrl *)cls->ctx;

	if (run) {
		if (ems_flag_like(cls->flg, FLG_RUN))
			return EMS_OK;

		if (ctrl_change_status(ctrl, st_start) != EMS_OK)
			return EMS_ERR;

		ems_flag_set(cls->flg, FLG_RUN);
	} else {
		if (ems_flag_unlike(cls->flg, FLG_RUN)) 
			return EMS_OK;

		ems_flag_unset(cls->flg, FLG_RUN);
		ctrl_change_status(ctrl, st_stop);
	}

	return EMS_OK;
}

static ems_int ctrl_process(audit_class *cls, ems_uint evt, ems_uchar *arg) 
{
	ems_l_trace("ctrl, got msg 0x%x", evt);

	switch(evt) {
	case A_AUDIT_START:
		ems_l_trace("audit start...");
		return ctrl_run(cls, EMS_YES);

	case A_AUDIT_STOP:
		ems_l_trace("audit stop...");
		return ctrl_run(cls, EMS_NO);

	default:
		break;
	}

	return EMS_OK;
}

static ems_int ctrl_uninit(audit_class *cls) 
{
	audit_ctrl *ctrl = (audit_ctrl *)cls->ctx;

	ems_l_trace("ctrl do uninit..");

	ems_assert(ctrl && ctrl->sess == NULL);
	ems_assert(ems_queue_empty(&ctrl->cmd));
	ems_assert(ctrl->st == st_stop);

	ems_free(ctrl);
	cls->ctx = NULL;

#if 0
	if (ctrl->sess) {
		ems_session_shutdown_and_destroy(ctrl->sess);
		ctrl->sess = NULL;
	}

	ems_queue_clear(&ctrl->cmd, ems_session, entry, ems_session_shutdown_and_destroy);
#endif
	return EMS_OK;
}

audit_class c_ctrl={
	.id   = ctrl_id,
	.type = ctrl_type, 
	.nick = ctrl_nick,
	.init = ctrl_init,
	.process = ctrl_process,
	.uninit  = ctrl_uninit,
	.ctx  = NULL
};
