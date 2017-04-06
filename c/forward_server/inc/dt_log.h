

#ifndef USE_LOG_TRACE___HEADER
#define USE_LOG_TRACE___HEADER

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define LOG_ERROR	0
#define LOG_FAULT	1
#define LOG_WARN	2
#define LOG_INFO	3
#define LOG_TRACE	4

dt_void _log_error(dt_cchar *fmt, ...);
dt_void _log_fault(dt_cchar *fmt, ...);
dt_void _log_warn(dt_cchar *fmt, ...);
dt_void _log_info(dt_cchar *fmt, ...);
dt_void _log_trace(dt_cchar *fmt, ...);

#define log_err		_log_error
#define log_flt		_log_fault
#define log_warn	_log_warn
#define log_info	_log_info
#define log_trace	_log_trace

#define log_traceline()	log_trace("file: %s, line: %u", __FILE__, __LINE__)

dt_int log_set(dt_int l, dt_cchar *log_fl);
dt_int log_level();

#endif
