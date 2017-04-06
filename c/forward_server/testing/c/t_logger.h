
#ifndef TEST_LOGGER_HEADER__
#define TEST_LOGGER_HEADER__


dt_void t_logger(dt_int level, t_session *sess, t_user *user, dt_cchar *fmt, ...);

#define t_logger_trace(sess, user, fmt, args...)	t_logger(LOG_TRACE, sess, user, fmt, ##args)
#define t_logger_info(sess, user, fmt, args...)		t_logger(LOG_INFO, sess, user,  fmt, ##args)
#define t_logger_warn(sess, user, fmt, args...)		t_logger(LOG_WARN, sess, user,  fmt, ##args)
#define t_logger_error(sess, user, fmt, args...)	t_logger(LOG_ERROR, sess, user, fmt, ##args)
#define t_logger_fault(sess, user, fmt, args...)	t_logger(LOG_FAULT, sess, user, fmt, ##args)



#endif
