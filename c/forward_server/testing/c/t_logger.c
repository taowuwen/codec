
#include "t_main.h"
#include "t_logger.h"
#include "t_session.h"
#include "fwd_sock.h"
#include "t_user.h"



static struct _memory_block_{
	FILE *fp;
	dt_mtx	mtx;
} logger;

dt_int t_logger_init()
{
	logger.fp = fopen("/tmp/fwd_logger.log", "ab+");
	dt_mtx_init(logger.mtx);
	return 0;
}

dt_void t_logger_lock()
{
	dt_mtx_lock(logger.mtx);
}

dt_void t_logger_unlock()
{
	dt_mtx_unlock(logger.mtx);
}

dt_void t_logger_append(dt_cchar *str)
{
	t_logger_lock();
	fwrite(str, strlen(str), 1, logger.fp);
	fwrite("\n", 1, 1, logger.fp);
	t_logger_unlock();
}

dt_void t_logger_uninit()
{
	if(logger.fp) {
		fclose(logger.fp);
		logger.fp = NULL;
	}

	dt_mtx_destroy(logger.mtx);
}

dt_void 
t_logger(dt_int level, t_session *sess, t_user *user, dt_cchar *fmt, ...)
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

#if 0
	if (sess) {
		ret = snprintf(p, left, "[%s] ", session_uuid(sess)?session_uuid(sess):"uuid");
		p    += ret;
		left -= ret;
	}
#endif

	if (user) {
		ret = snprintf(p, left, "<%s:%s(fwd:%d)(cm:%d)>",
				sess?((user == session_self(sess))?"self":"peer"):"unknown",
				user_ticket(user)?user_ticket(user): "ticket",
				fwd_sock_fd(&user->fwd),
				fwd_sock_fd(&user->cm)
				);
		p    += ret;
		left -= ret;
	}

	va_start(args, fmt);
	vsnprintf(p, left, fmt, args);
	va_end(args);

	t_logger_append(buf);

	dt_free(buf);
}
