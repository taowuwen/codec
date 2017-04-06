
#include "ems_common.h"
#include "ems_buffer.h"
#include "ems_logger.h"
#include <stdarg.h>

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

	ems_buffer_init(&log->buf, EMS_BUFFER_1K);

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
}

#ifdef DEBUG
ems_void _ems_logger(
		ems_int level, 
		ems_cchar *fl, 
		ems_uint l, 
		ems_logger *log, 
		ems_cchar *fmt, ...)
#else
ems_void _ems_logger(ems_int level, ems_logger *log, ems_cchar *fmt, ...)
#endif

{
	ems_char *p;
	ems_int  left;
	ems_int  ret;
	va_list args;
	time_t	tm;

	if (level > log->level)
		return;

	p    = buf_wr(&log->buf);
	left = buf_left(&log->buf);

	time(&tm);
	ret = snprintf(p, left, "%s", ctime(&tm));

	left -= ret -1;
	p    += ret -1;

#ifdef DEBUG
	ret = snprintf(p, left, "[%s:%d]==%u==\t", fl, l, ems_getpid());

	left -= ret;
	p    += ret;
#endif

	va_start(args, fmt);
	ret = vsnprintf(p, left, fmt, args);
	va_end(args);

	if (log->fp) {
		fwrite(buf_rd(&log->buf), strlen(buf_rd(&log->buf)), 1, log->fp);
		fwrite(NEWLINE, strlen(NEWLINE), 1, log->fp);
		fflush(log->fp);
	} else {
		fprintf(stdout, "%s\n", buf_rd(&log->buf));
		fflush(stdout);
	}
}

