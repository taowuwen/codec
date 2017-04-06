
#include "dt.h"

static dt_int	_g_log_level = LOG_INFO;
static dt_char	_g_file[1024] = {0};

dt_int log_level()
{
	return _g_log_level;
}

dt_int log_set(dt_int l, const char *fl)
{
	if ( l >= 0)
		_g_log_level = l;
	
	if ( fl)
		snprintf(_g_file, 1024, "%s", fl);
#ifdef DEBUG
#ifdef WIN32
	else
		snprintf(_g_file, 1024, "%s", "c:\\fwd_log.log");
#else
	else
		snprintf(_g_file, 1024, "%s", "/tmp/fwd_log.log");
#endif
#endif

	return OK;
}

#define BUF_LEN	1024
static char *dt_timenow()
{
	static char time_buff[256];
#ifndef WIN32
	time_t	time_now;
	struct tm *timeinfo;
	time(&time_now);
	timeinfo = localtime(&time_now);
	snprintf(time_buff, 256, "%02d:%02d:%02d",
			timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
#else
	SYSTEMTIME tm;

	GetSystemTime(&tm);
	snprintf(time_buff, 256, "%02d:%02d:%02d",
			tm.wHour + 8, tm.wMinute, tm.wSecond);
#endif
	
	return time_buff;
}

#ifdef WIN32
#define NEWLINE	"\r\n"
#else
#define NEWLINE	"\n"
#endif
#define LEN_NEWLINE	strlen(NEWLINE)

static dt_void traceLogIntoFile(dt_cchar *fl, dt_int len, dt_cchar *v)
{
#ifdef WIN32
	OutputDebugString(v);
#else
#if 0
	FILE *fp = NULL;
	fp = fopen(fl, "ab+");
	if ( fp) {
		fwrite(v, len, 1, fp);
		fwrite(NEWLINE, LEN_NEWLINE, 1, fp);
		fclose(fp);
	}
#endif

	fprintf(stdout, "%s\n", v);
	fflush(stdout);
#endif
}

dt_cchar *str_level(dt_int l)
{
	static dt_char buf[16] = {0};
	switch(l) {
		case LOG_ERROR:
			snprintf(buf, 16, "%s", "err");
			break;

		case LOG_FAULT:
			snprintf(buf, 16, "%s", "flt");
			break;
		case LOG_WARN:
			snprintf(buf, 16, "%s", "wrn");
			break;
		case LOG_INFO:
			snprintf(buf, 16, "%s", "inf");
			break;
		case LOG_TRACE:
			snprintf(buf, 16, "%s", "tra");
			break;
		default:
			snprintf(buf, 16, "%s", "unknown");
			break;
	}

	return buf;
}

#define PARASE_ARG(A) do {						\
	dt_int rtn = 0;							\
	dt_char log[4096] = {0};					\
	va_list parg;							\
									\
	if ( A > _g_log_level) break; 					\
									\
	rtn = snprintf(log, BUF_LEN,					\
				"%s==%u==[0x%012lx][net][%s]\t",	\
				dt_timenow(), dt_getpid(), 		\
				dt_gettid(), str_level(A));		\
									\
	va_start(parg, fmt);						\
	vsnprintf(log + rtn, BUF_LEN - rtn, fmt, parg);			\
	va_end(parg);							\
	traceLogIntoFile(_g_file, strlen(log), log);			\
} while (0)

void _log_error(const char *fmt, ...)
{
	PARASE_ARG(LOG_ERROR);
}

void _log_fault(const char *fmt, ...)
{
	PARASE_ARG(LOG_FAULT);
}

void _log_warn(const char *fmt, ...)
{
	PARASE_ARG(LOG_WARN);
}

void _log_info(const char *fmt, ...)
{
	PARASE_ARG(LOG_INFO);
}

void _log_trace(const char *fmt, ...)
{
	PARASE_ARG(LOG_TRACE);
}
