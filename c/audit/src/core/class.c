
#include "audit.h"
#include "class.h"

extern audit_class c_ctrl;
extern audit_class c_out;
extern audit_class c_net;

audit_class *_gcls[] = {
	[mod_ctrl] = &c_ctrl,
	[mod_out]  = &c_out,
	[mod_net]  = &c_net,
	NULL
};


audit_class *class_find(ems_uint d) 
{
	module_id id = (module_id)d;

	ems_assert(id >= mod_ctrl && id <= mod_sys);

	/* for speed up this */
	return _gcls[id];
}


ems_int audit_sendmsg(ems_uint s, ems_uint d, ems_uint evt, ems_uchar *arg)
{
	audit_class *cls = NULL;

	cls = class_find(d);
	if (cls && cls->process) {
		ems_l_trace("s(%d) --> d(%d: %s), msg: 0x%x(%d)", s, d, cls->nick(), evt, evt);
		return cls->process(cls, evt, arg);
	}

	return EMS_ERR;
}

ems_int audit_modules_init()
{
	ems_int      i;
	audit_class *cls;

	for (i = 0; _gcls[i] != NULL ; i++) {
		cls = _gcls[i];

		if (cls->init) {
			ems_l_trace("module --- %s --- INIT", cls->nick());
			cls->init(cls);
		}
	}

	return EMS_OK;
}

ems_void audit_modules_uninit()
{
	ems_int      i;
	audit_class *cls;

	for (i = 0; _gcls[i] != NULL ; i++) {
		cls = _gcls[i];

		if (cls->uninit) {
			ems_l_trace("module --- %s --- UNINIT", cls->nick());
			cls->uninit(cls);
		}
	}
}

ems_int output_log(ems_uint src, ems_uint ty, ems_cchar *log)
{
	return audit_sendmsg(src, mod_out, A_AUDIT_LOG, (ems_uchar *)log);
}
