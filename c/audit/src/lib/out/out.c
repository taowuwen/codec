
#include "audit.h"
#include "ems_core.h"
#include "ems_client.h"
#include "out.h"

static ems_queue *_out_flts = NULL;

ems_queue *out_filters()
{
	return _out_flts;
}

static ems_uint out_id() 
{
	return mod_out;
}

static ems_uint out_type() 
{
	return MODULE_TYPE_IN;
}

static ems_cchar *out_nick() 
{
	return "out";
}

static ems_int out_init(audit_class *cls) 
{
	out_core *out = NULL;

	ems_l_trace("out do init...");

	out = (out_core *)ems_malloc(sizeof(out_core));
	if (!out)
		return EMS_ERR;

	memset(out, 0, sizeof(out_core));

	ems_queue_init(&out->flt);
	_out_flts = &out->flt;

	cls->ctx = (ems_void *)out;

	return EMS_OK;
}

/*
   load plugins 
 */
static ems_int out_start(out_core *out)
{
	out_load_plugins(&out->flt, id_out);

	if (ems_queue_empty(&out->flt)) {
		ems_l_trace("out at least one filter should supply");
		return EMS_ERR;
	}

	/* the size should be read from a cfg file, 1M by default */
	if (ems_buffer_init(&out->buf, EMS_BUFFER_8k * 8) != EMS_OK) 
	{
		out_unload_plugins(&out->flt);
		return EMS_ERR;
	}

	out_plug_broadcast(&out->flt, A_AUDIT_START, NULL);

	return EMS_OK;
}

static ems_int out_stop(out_core *out)
{
	out_plug_broadcast(&out->flt, A_AUDIT_STOP, NULL);
	out_unload_plugins(&out->flt);

	if (buf_size(&out->buf) > 0)
		ems_buffer_uninit(&out->buf);

	return EMS_OK;
}

static ems_int out_run(audit_class *cls, ems_int run)
{
	out_core *out = (out_core *)cls->ctx;

	if (run) {
		if (ems_flag_like(cls->flg, FLG_RUN))
			return EMS_OK;

		if (out_start(out) != EMS_OK)
			return EMS_ERR;

		ems_flag_set(cls->flg, FLG_RUN);
	} else {
		if (ems_flag_unlike(cls->flg, FLG_RUN))
			return EMS_OK;

		out_stop(out);
		ems_flag_unset(cls->flg, FLG_RUN);
	}

	return EMS_OK;
}

static ems_int output_buffer(out_core *out, ems_cchar *log)
{
	ems_int len = 0;
	ems_assert(out && log);

	len = ems_strlen(log);

	if (len <= 0)
		return EMS_ERR;

	if ((buf_left(&out->buf) < len + 2)) {
		ems_buffer_refresh(&out->buf);
		if (buf_left(&out->buf) < len + 2)
			return EMS_ERR;
	}

	ems_buffer_write(&out->buf, log, strlen(log));
	ems_buffer_write(&out->buf, "\n", 1);

	return out_plug_broadcast(&out->flt, A_AUDIT_LOG, (ems_void *)(&out->buf));
}

static ems_int out_process(audit_class *cls, ems_uint evt, ems_uchar *arg) 
{
	ems_l_trace("out, got msg 0x%x", evt);

	switch(evt) {
	case A_AUDIT_START:
		ems_l_trace("out start...");
		return out_run(cls, EMS_YES);

	case A_AUDIT_STOP:
		ems_l_trace("out stop...");
		return out_run(cls, EMS_NO);

	default:
		break;
	}

	if (ems_flag_unlike(cls->flg, FLG_RUN)) {
		ems_l_trace("output module not running");
		return EMS_ERR;
	}

	switch(evt) {
	case A_AUDIT_LOG:
		ems_l_trace("rule> out: %s", (ems_cchar *) arg);
		output_buffer((out_core *)cls->ctx, (ems_cchar *)arg);
		break;

	default:
		break;
	}

	return EMS_OK;
}

static ems_int out_uninit(audit_class *cls) 
{
	out_core *out = (out_core *)cls->ctx;

	ems_l_trace("out do uninit..");
	ems_assert(out);
	ems_assert(buf_size(&out->buf) == 0);
	ems_assert(ems_queue_empty(&out->flt));

	ems_free(out);

	_out_flts = NULL;
	cls->ctx = NULL;
	return EMS_OK;
}

audit_class c_out={
	.id   = out_id,
	.type = out_type, 
	.nick = out_nick,
	.init = out_init,
	.process = out_process,
	.uninit  = out_uninit,
	.ctx  = NULL
};
