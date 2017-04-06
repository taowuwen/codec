
#ifndef FWD_LOGGER_HEADER___
#define FWD_LOGGER_HEADER___


dt_void fwd_logger(dt_int level, fwd_session *sess, fwd_user *user, dt_cchar *fmt, ...);

#ifdef DEBUG

#define fwd_logger_trace(sess, user, fmt, args...)	fwd_logger(LOG_TRACE, sess, user, fmt, ##args)
#else
#define fwd_logger_trace(...)
#endif

#define fwd_logger_info(sess, user, fmt, args...)	fwd_logger(LOG_INFO, sess, user,  fmt, ##args)
#define fwd_logger_warn(sess, user, fmt, args...)	fwd_logger(LOG_WARN, sess, user,  fmt, ##args)
#define fwd_logger_error(sess, user, fmt, args...)	fwd_logger(LOG_ERROR, sess, user, fmt, ##args)
#define fwd_logger_fault(sess, user, fmt, args...)	fwd_logger(LOG_FAULT, sess, user, fmt, ##args)



#endif
