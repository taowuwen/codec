
#include "fwd_main.h"
#include "fwd_logger.h"
#include "fwd_sock.h"


dt_void 
fwd_logger(dt_int level, fwd_session *sess, fwd_user *user, dt_cchar *fmt, ...)
{
	dt_char *buf = NULL, *p;
	dt_int  ret, left;
	va_list args;

	if (level > log_level())
		return;

	left = 4096 + fwd_strlen(fmt);

	buf = (dt_char *)dt_malloc(left * sizeof(dt_char));
	if (!buf)
		return;

	ret  = 0;
	p    = buf;

	if (sess) {
		ret = snprintf(p, left, "[%s] ", str_text(&sess->ticket));
		p    += ret;
		left -= ret;
	}

	if (user) {
		ret = snprintf(p, left, "<%s:%5d (sess: %4d)> ",
					fwd_sock_addr(&user->sock),
					fwd_sock_port(&user->sock),
					fwd_sock_fd(&user->sock)
					);

		p    += ret;
		left -= ret;
	}

	va_start(args, fmt);
	vsnprintf(p, left, fmt, args);
	va_end(args);

	switch(level) {

	case LOG_TRACE:
		log_trace("%s", buf);
		break;

	case LOG_INFO:
		log_info("%s", buf);
		break;

	case LOG_ERROR:
		log_err("%s", buf);
		break;
	case LOG_WARN:
		log_warn("%s", buf);
		break;

	case LOG_FAULT:
		log_flt("%s", buf);

	default:
		log_trace("%s", buf);
		break;
	}

	dt_free(buf);
}
