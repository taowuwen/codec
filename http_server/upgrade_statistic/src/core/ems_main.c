
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

ems_void core_gw_addr_clear()
{
	str_set(&_gcore->gw, NULL);
}

ems_cchar *core_gw_addr()
{
	if (str_len(&_gcore->gw) <= 0) {
		str_set(&_gcore->gw, cfg_get(emscfg(), CFG_lan_addr));
	}

	return str_text(&_gcore->gw)?str_text(&_gcore->gw):"";
}

ems_void core_gw_ifname_clear()
{
	str_set(&_gcore->ifname, NULL);
}

ems_cchar *core_gw_ifname()
{
	if (str_len(&_gcore->gw) <= 0) {
		str_set(&_gcore->ifname, cfg_get(emscfg(), CFG_lan_ifname));
	}

	return str_text(&_gcore->ifname)?str_text(&_gcore->ifname):"br-lan";
}

ems_buffer  *core_buffer()
{
	return &_gcore->buf;
}

ems_cchar *core_ac_mac()
{
	if (str_len(&_gcore->ac_mac) <= 0) {
		str_set(&_gcore->ac_mac, cfg_get(emscfg(), CFG_ems_mac));
	}

	return str_text(&_gcore->ac_mac)?str_text(&_gcore->ac_mac):"";
}

ems_cchar *core_portal_addr()
{
	if (str_len(&_gcore->portal) <= 0) {
		str_set(&_gcore->portal, cfg_get(emscfg(), CFG_app_portal_addr));
	}

	return str_text(&_gcore->portal)?str_text(&_gcore->portal):"";
}

ems_int core_portal_redirect_port()
{
	if (_gcore->portal_redirect_port <= 0) {
		_gcore->portal_redirect_port = ems_atoi(cfg_get(emscfg(), CFG_app_portal_redirect));
	}

	return _gcore->portal_redirect_port;
}

ems_cchar *core_ssid()
{
	if (str_len(&_gcore->ssid) <= 0) {
		str_set(&_gcore->ssid, cfg_get(emscfg(), CFG_wireless_ssid));
	}

	return str_text(&_gcore->ssid)?str_text(&_gcore->ssid):"";
}

ems_cchar *core_devicetype()
{
	if (str_len(&_gcore->devty) <= 0) {
		str_set(&_gcore->devty, cfg_get(emscfg(), CFG_ems_devicetype));
	}

	return str_text(&_gcore->devty)?str_text(&_gcore->devty):"";
}

ems_cchar *core_sn()
{
	if (str_len(&_gcore->sn) <= 0) {
		str_set(&_gcore->sn, cfg_get(emscfg(), CFG_ems_sn));
	}

	return str_text(&_gcore->sn)?str_text(&_gcore->sn):"";
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
	ems_logger_set_level(logger(), EMS_LOG_WARN);
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
