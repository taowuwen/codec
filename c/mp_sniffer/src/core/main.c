
#include "ems_core.h"

#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/stat.h>

static ems_core *_gcore = NULL;
ems_logger  _glog;

ems_logger  *logger()
{
	return &_glog;
}

ems_core  *emscorer()
{
	ems_assert(_gcore != NULL);
	return _gcore;
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

ems_buffer *core_buffer()
{
	return &_gcore->buf;
}

static ems_int ems_main(ems_int argc, ems_char **argv)
{
	ems_int   rtn  = EMS_OK;
	ems_core *core = NULL;

#ifdef EMS_LOGGER_FILE
	FILE     *fp = NULL;
	fp = fopen("/tmp/sniffer.log", "a+");
	if (fp)
		ems_logger_set_fp(logger(), fp);
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

ems_int main(ems_int argc, ems_char **argv)
{
	ems_int  rtn;
	ems_int  c, l;

	l = EMS_LOG_WARN;
	while ((c = getopt(argc, argv, "v")) != -1) {

		switch(c) {
		case 'v':
			l = EMS_LOG_TRACE;
			break;
		default:
			break;
		}
	}

#ifdef DEBUG
	ems_reset_rlimit();
#endif
	ems_start_memory_trace(EMS_YES);
	ems_logger_init(&_glog, stderr, l);

	rtn = ems_main(argc, argv);

	ems_logger_uninit(&_glog);
	ems_stop_memory_trace();

	return rtn;
}
