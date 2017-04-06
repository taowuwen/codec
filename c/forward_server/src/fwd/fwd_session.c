
#include "fwd_main.h"
#include "fwd_session.h"
#include "fwd_user.h"
#include "fwd_logger.h"
#include "fwd_child.h"
#include "fwd_transinfo.h"
#include "fwd_status.h"


typedef dt_int (*sess_timeout_func)(fwd_session *);
static dt_void sess_timeout_cb(fwd_timeout *to);

static dt_int sess_timeout_init(fwd_session *);
static dt_int sess_timeout_est(fwd_session  *);
static dt_int sess_timeout_err(fwd_session *);
static dt_int sess_timeout_clean(fwd_session *);

static sess_timeout_func sess_timeout_entry[] = 
{
	[ST_INITED]  = sess_timeout_init,
	[ST_AUTH]    = NULL,
	[ST_WATTING] = NULL,
	[ST_EST]     = sess_timeout_est,
	[ST_ERROR]   = sess_timeout_err,
	[ST_CLEAN]   = sess_timeout_clean,
	[ST_MAX]     = NULL
};

static dt_int sess_status(fwd_session *sess)
{
	if (sess->st <= ST_MAX && sess->st > ST_MIN)
		return sess->st;

	return ST_MAX;
}

#define sess_timeout_run(sess)        do {				\
	if (sess_timeout_entry[sess_status(sess)])			\
		sess_timeout_entry[sess_status(sess)](sess);		\
} while (0)

static dt_void sess_timeout_cb(fwd_timeout *to)
{
	fwd_session *sess = dt_container_of(to, fwd_session, timeout);

	sess_timeout_run(sess);
}

static dt_int sess_timeout_init(fwd_session *sess)
{
	fwd_logger_trace(sess, NULL, "session timeout: no user fire it");

	dt_assert((sess->base == NULL) && "bug");

	fwd_session_changestatus(sess, ST_ERROR);

	sess_timeout_run(sess);

	return FWD_OK;
}

dt_int  fwd_session_detach(fwd_session *sess)
{
	dt_assert(sess);

	if (!sess)
		return FWD_ERR;

	if (sess->base) {
		fwd_child_session_detach(sess->base, sess);
		sess->base = NULL;
	}

	return FWD_OK;
}

dt_int fwd_session_shutdown(fwd_session *sess)
{
	if (!sess)
		return FWD_OK;

	fwd_session_timeout_cancel(sess);

	fwd_user_destroy(fwd_session_user1(sess));
	fwd_user_destroy(fwd_session_user2(sess));

	fwd_session_user1_set(sess, NULL);
	fwd_session_user2_set(sess, NULL);

	fwd_session_detach(sess);
#if 0
	fwd_session_changestatus(sess, ST_INITED);
#else
	fwd_main_session_down(sess);
#endif

	return FWD_OK;
}

static dt_int sess_timeout_est(fwd_session  *sess)
{
	struct timeval tv;

	fwd_logger_trace(sess, NULL, "session timeout trigged @ established");

	gettimeofday(&tv, NULL);

	if (fwd_time_diff(&tv, &sess->trans.time.access) > FWD_TIMEOUT_ESTABLISHED) 
	{
		fwd_logger_info(sess, NULL,
				"session stay in inactive for %d secs, auto shutdown",
				FWD_TIMEOUT_ESTABLISHED);

		fwd_session_shutdown(sess);
	} 
	else
		fwd_session_timeout_set(sess, FWD_TIMEOUT_ESTABLISHED);

	return FWD_OK;
}

static dt_int sess_timeout_err(fwd_session *sess)
{
	fwd_logger_info(sess, NULL, "session timeout:  delete");
	dt_assert((sess->base == NULL) && "bug");

	return fwd_main_session_fired(sess);
}

static dt_int sess_timeout_clean(fwd_session *sess)
{
	dt_assert(sess && sess->base && sess->user1 && sess->user2 && "bug");

	fwd_logger_trace(sess, NULL, "session timeout clean triggerd, shutdown forcely");

	fwd_session_shutdown(sess);
	return FWD_OK;
}


fwd_session *fwd_session_create(dt_cchar *ticket)
{
	fwd_session *sess = NULL;

	sess = (fwd_session *)dt_malloc(sizeof(fwd_session));

	if (sess) {
		if (fwd_session_init(sess, ticket) != FWD_OK) {
			dt_free(sess);
			sess = NULL;
		}
	}

	return sess;
}

dt_int  fwd_session_init(fwd_session *sess, dt_cchar *ticket)
{
	dt_assert(sess && ticket);

	if (!(sess && ticket))
		return FWD_ERR_INVALID_ARG;

	memset(sess, 0, sizeof(fwd_session));

	sess->user1 = NULL;
	sess->user2 = NULL;
	sess->st    = ST_MIN;
	sess->base  = NULL;

	fwd_trans_init(&sess->trans);
	dt_queue_init(&sess->list);
	str_init(&sess->ticket);
	fwd_hash_fd_init(&sess->hash);

	str_set(&sess->ticket, ticket);
	fwd_hash_fd_set_ticket(&sess->hash, ticket);

	return FWD_OK;
}

dt_void fwd_session_uninit(fwd_session *sess)
{
	if (!sess)
		return;

	fwd_session_shutdown(sess);

	dt_queue_remove(&sess->list);
	fwd_timeout_cancel(&sess->timeout);

	fwd_hash_remove(&sess->hash);
	fwd_hash_fd_uninit(&sess->hash);
	str_uninit(&sess->ticket);
	fwd_trans_uninit(&sess->trans);
}

dt_void fwd_session_destroy(fwd_session *sess)
{
	dt_assert(sess && validBlock(sess));

	if (!sess)
		return;

	fwd_session_uninit(sess);
	dt_free(sess);
}

dt_int fwd_session_attach(fwd_session *sess, fwd_child *base)
{
	dt_assert(sess);

	if (!sess)
		return FWD_ERR;

	sess->base = base;
	return FWD_OK;
}

dt_int fwd_session_status(fwd_session *sess)
{
	return sess->st;
}

dt_int fwd_session_timeout_set(fwd_session *sess, dt_uint to)
{
	switch(fwd_session_status(sess)) {

	case ST_CLEAN:
	case ST_EST:
		if (!sess->base) {
			dt_assert(0 && "call fwd_session_attach first");
			fwd_logger_error(sess, NULL, "call fwd_session_attach first");
			return FWD_ERR;
		}

		return fwd_child_timeout_set(sess->base, 
					    &sess->timeout,
					    to,
					    sess_timeout_cb,
					    0);
	default:
		return fwd_main_timeout_set(&sess->timeout,
				      to, 
				      sess_timeout_cb, 
				      0);
		break;
	}

	return FWD_OK;
}

dt_int fwd_session_timeout_cancel(fwd_session *sess)
{
	switch(fwd_session_status(sess)) {

	case ST_CLEAN:
	case ST_EST:
		dt_assert(sess->base);
		return fwd_child_timeout_cancel(&sess->timeout);

	case ST_WATTING:
		if (sess->base)
			return fwd_child_timeout_cancel(&sess->timeout);

		// no break; here
	default:
		dt_assert(sess->base == NULL);
		return fwd_main_timeout_cancel(&sess->timeout);
	}

	return FWD_OK;
}

static dt_int fwd_session_st_esablished(fwd_session *sess)
{
	dt_assert(fwd_session_user1(sess) && fwd_session_user2(sess));

	dt_assert(sess->base);

	if (!sess->base) {
		fwd_logger_error(sess, NULL, "call fwd_session_attach first");
		return FWD_ERR;
	}

	fwd_user_changestatus(fwd_session_user1(sess), ST_EST);
	fwd_user_changestatus(fwd_session_user2(sess), ST_EST);
	fwd_session_timeout_set(sess, FWD_TIMEOUT_ESTABLISHED);

	return FWD_OK;
}

static dt_int fwd_session_st_clean(fwd_session *sess)
{
	dt_assert(fwd_session_user1(sess) && fwd_session_user2(sess));
	dt_assert(sess->base);

	fwd_logger_trace(sess, NULL, "session into flush msg status...");
	fwd_session_timeout_set(sess, FWD_TIMEOUT_SENDMSG);

	return FWD_OK;
}

static dt_int fwd_session_st_init(fwd_session *sess)
{
	dt_assert(sess);

	fwd_session_timeout_set(sess, FWD_TIMEOUT_SESSION_INIT);

	return FWD_OK;
}

dt_int fwd_session_changestatus(fwd_session *sess, fwd_status st)
{
	dt_int ret;
	dt_int old_st;

	dt_assert(sess);
	if (!sess)
		return FWD_ERR;

	old_st = sess->st;
	fwd_logger_trace(sess, NULL, "session status change: %s --> %s",
				fwd_status_str(old_st), fwd_status_str(st));
	sess->st = st;
	ret = OK;
	switch(st) {

	case ST_INITED:
		ret = fwd_session_st_init(sess);
		if (old_st == ST_EST)
			fwd_session_print(sess, NULL, &sess->trans);
		break;

	case ST_ERROR:
		break;

	case ST_EST:
		ret = fwd_session_st_esablished(sess);
		break;

	case ST_CLEAN:
		fwd_session_print(sess, NULL, &sess->trans);
		ret = fwd_session_st_clean(sess);

	default:
		break;
	}

	fwd_trans_flush(&sess->trans);

	return ret;
}

fwd_user *fwd_session_next_user(fwd_session *sess, fwd_user *user)
{
	dt_assert(sess && user);

	if (!(sess && user))
		return NULL;

	if (user == fwd_session_user1(sess)) {
		dt_assert(fwd_session_user2(sess) != user);
		return fwd_session_user2(sess);
	} else {
		return fwd_session_user1(sess);
	}

	return NULL;
}

dt_void 
fwd_session_print(fwd_session *sess, fwd_user *user, fwd_transinfo *trans)
{
	fwd_logger_info(sess, user, "\n"
			"total transfer   : %10lld  bytes\n"
			"total send       : %10lld  bytes\n"
			"total recv       : %10lld  bytes\n"
			"real time speed  : %10lld  bytes/s\n"
			"average   speed  : %10lld  bytes/s\n"
			"read      speed  : %10lld  bytes/s\n"
			"write     speed  : %10lld  bytes/s\n",
				trans->total_bytes,
				trans->total_write,
				trans->total_read,
				trans->real_time_speed,
				trans->average_speed,
				trans->read_speed,
				trans->write_speed);
}

