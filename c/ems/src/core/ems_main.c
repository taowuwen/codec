
#include "ems_core.h"
#include "ems_cmd.h"
#include "ems_client.h"

#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/stat.h>

static ems_core *_gcore = NULL;

ems_core  *emscorer()
{
	ems_assert(_gcore != NULL);
	return _gcore;
}

ems_cfg   *emscfg()
{
	ems_assert(_gcore != NULL);
	return &_gcore->cfg;
}

ems_event *eventer()
{
	ems_assert(_gcore != NULL);
	return &_gcore->evt;
}

ems_queue *timeouter()
{
	ems_assert(_gcore != NULL);

	return &_gcore->evt.timeout;
}

app_module  **core_moduler()
{
	return _gcore->_core_module;
}

ems_buffer *core_buffer()
{
	return &_gcore->buf;
}

ems_cchar *core_gw_addr()
{
	return str_text(&_gcore->net.lan.ip);
}

ems_cchar *core_gw_ifname()
{
	return str_text(&_gcore->net.lan.ifname);
}

ems_cchar *core_ac_mac()
{
	return str_text(&_gcore->dev.mac);
}

ems_cchar *core_devicetype()
{
	return str_text(&_gcore->dev.ty);
}

ems_cchar *core_sn()
{
	return str_text(&_gcore->dev.sn);
}

ems_cchar *core_sysver()
{
	return str_text(&_gcore->dev.ver);
}

ems_cchar *core_wan_addr()
{
	return str_text(&_gcore->net.wan.ip);
}

ems_cchar *core_wan_gw()
{
	return str_text(&_gcore->net.wan.gw);
}

ems_int ems_main(ems_int cmd, ems_int argc, ems_char **argv)
{
	ems_int   rtn  = EMS_OK;
	ems_core *core = NULL;

#ifdef EMS_LOGGER_FILE
	FILE     *fp = NULL;
	ems_logger_reset(logger());
	fp = fopen("/tmp/ems_run_time.log", "a+");
	if (fp)
		ems_logger_set_fp(logger(), fp);

#ifdef DEBUG
	ems_logger_set_level(logger(), EMS_LOG_TRACE);
#else
	ems_logger_set_level(logger(), EMS_LOG_INFO);
#endif
#endif
	core = (ems_core *)ems_malloc(sizeof(ems_core));
	if (core) {
		_gcore = core;
		if (ems_core_init(core) == EMS_OK)
			rtn = ems_core_main(core, argc, argv);

		ems_core_uninit(core);
		_gcore = NULL;
		ems_free(core);
	}

#ifdef EMS_LOGGER_FILE
	if (fp) fclose(fp);
#endif

	return rtn;
}
