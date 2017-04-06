
#include "ems_common.h"
#include "ems_buffer.h"
#include "ems_logger.h"
#include <stdarg.h>

#ifdef EMS_USE_SYSLOG
#include <syslog.h>
#endif

#ifdef WIN32
#define NEWLINE	"\r\n"
#else
#define NEWLINE	"\n"
#endif

ems_int ems_logger_init(ems_logger *log, FILE *fp, ems_int level)
{
	memset(log, 0, sizeof(ems_logger));

	if (level < 0)
		level = 0;

	log->fp = fp;
	if (!log->fp)
		log->fp = stdout;

	log->level = level;

	ems_buffer_init(&log->buf, EMS_BUFFER_4k);

#ifdef EMS_USE_SYSLOG
	openlog("sniffer", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_DAEMON);
#endif

	return EMS_OK;
}

ems_int ems_logger_reset(ems_logger *log)
{
	ems_assert(log != NULL);

	if (log->fp)
		log->fp = stdout;

	ems_buffer_reset(&log->buf);

	log->level = 0;

	return EMS_OK;
}

ems_void ems_logger_uninit(ems_logger *log)
{
	ems_logger_reset(log);
	ems_buffer_uninit(&log->buf);
#ifdef EMS_USE_SYSLOG
	closelog();
#endif
}

#ifdef DEBUG
ems_void _ems_logger(
		ems_int level, 
		ems_cchar *func, 
		ems_uint l, 
		ems_logger *log, 
		ems_cchar *fmt, ...)
#else
ems_void _ems_logger(ems_int level, ems_logger *log, ems_cchar *fmt, ...)
#endif

{
	va_list args;
	ems_char *p;
	ems_int  left;
	ems_int  ret;
	time_t	tp;

	if (level > log->level)
		return;

	p    = buf_wr(&log->buf);
	left = buf_left(&log->buf);

#ifdef EMS_USE_SYSLOG
	if (log->level == EMS_LOG_TRACE) 
#endif
	{
		struct tm tm;

		time(&tp);
		localtime_r(&tp, &tm);

		ret = snprintf(p, left, "<%d/%d/%d %02d:%02d:%02d> ", 
				tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,
				tm.tm_hour, tm.tm_min, tm.tm_sec
				);

		left -= ret;
		p    += ret;

#ifdef DEBUG
		ret = snprintf(p, left, "[%30s:%5d]=%u= ", func, l, ems_getpid());

		left -= ret;
		p    += ret;
#endif
	}

	va_start(args, fmt);
	ret = vsnprintf(p, left, fmt, args);
	va_end(args);

#ifdef EMS_USE_SYSLOG
	if (log->level == EMS_LOG_TRACE) 
#endif
	{
		if (log->fp) {
			fwrite(buf_rd(&log->buf), strlen(buf_rd(&log->buf)), 1, log->fp);
			fwrite(NEWLINE, strlen(NEWLINE), 1, log->fp);
			fflush(log->fp);
		} else {
			fprintf(stdout, "%s\n", buf_rd(&log->buf));
			fflush(stdout);
		}
	}
#ifdef EMS_USE_SYSLOG
	else
		syslog(level + 2, "%s", buf_rd(&log->buf));
#endif
}
