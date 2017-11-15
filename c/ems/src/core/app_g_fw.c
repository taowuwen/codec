
#include "ems_core.h"
#include "ems_client.h"
#include "app.h"
#include "ems_dns.h"

static ems_int g_fw_init(app_module *mod)
{
	return EMS_OK;
}

static ems_int g_fw_uninit(app_module *mod)
{
	mod->ctx = NULL;
	return EMS_OK;
}

extern ems_int g_fw_stop();
extern ems_int g_fw_start();
extern ems_int g_fw_flush_bwlist();

static ems_int set_bridge_nf_call_iptables_enable()
{
	if (!ems_atoi(ems_popen_get("cat /proc/sys/net/bridge/bridge-nf-call-iptables")))
		ems_systemcmd("sysctl -w net.bridge.bridge-nf-call-iptables=1");

	return EMS_OK;
}


static ems_int g_fw_do_start(app_module *mod)
{
	ems_int retry = 3;

	set_bridge_nf_call_iptables_enable();

	do {
		if (g_fw_start() == EMS_OK) break;

		g_fw_stop();
		ems_sleep(1);
	} while (--retry > 0);

	if (retry <= 0) {
		ems_systemcmd("/etc/init.d/network restart");
		exit(1);
	}

	return EMS_OK;
}

static ems_int g_fw_do_stop(app_module *mod)
{
	return g_fw_stop();
}

static ems_int g_fw_run(app_module *mod, ems_int run)
{
	if (run) {
		if (ems_flag_like(mod->flg, FLG_RUN))
			return EMS_OK;

		if (g_fw_do_start(mod) == EMS_OK)
			ems_flag_set(mod->flg, FLG_RUN);
	} else {
		if (ems_flag_unlike(mod->flg, FLG_RUN))
			return EMS_OK;

		g_fw_do_stop(mod);

		ems_flag_unset(mod->flg, FLG_RUN);
	}

	return EMS_OK;
}


static ems_int
g_fw_fw_reload(app_module *mod, json_object *req)
{
	g_fw_do_stop(mod);
	g_fw_do_start(mod);

	return EMS_OK;
}

static ems_int
g_fw_process(app_module *mod, ems_uint s, ems_uint d, ems_uint evt, json_object *root)
{
	switch(evt) {
	case EMS_APP_EVT_FW_RELOAD:
		return g_fw_fw_reload(mod, root);

	case EMS_APP_START:
		return g_fw_run(mod, EMS_YES);

	case EMS_APP_STOP:
		return g_fw_run(mod, EMS_NO);

	case EMS_APP_RULES_UPDATE:
		return g_fw_flush_bwlist();

	default:
		break;
	}

	return EMS_OK;
}


app_module app_g_fw = 
{
	.ty      = ty_g_fw,
	.desc    = ems_string("g_fw"),
	.ctx     = NULL,
	.flg     = 0,

	.init    = g_fw_init,
	.uninit  = g_fw_uninit,
	.run     = g_fw_run,
	.process = g_fw_process,
	.process_rule  = NULL,
	.version_match = NULL,
	.install       = NULL
};
