
#include "ems_core.h"
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

ems_buffer  *core_buffer()
{
	return &_gcore->buf;
}

ems_int ems_main(ems_int cmd, ems_int argc, ems_char **argv)
{
	ems_int   rtn  = EMS_OK;
	ems_core *core = NULL;

#ifdef EMS_LOGGER_FILE
	FILE     *fp = NULL;
	ems_logger_reset(logger());
	fp = fopen("/tmp/mpaudit.log", "a+");
	if (fp)
		ems_logger_set_fp(logger(), fp);

	ems_logger_set_level(logger(), EMS_LOG_TRACE);
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
