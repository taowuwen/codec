
#include "t_main.h"
#include "t_session.h"
#include "t_logger.h"
#include "t_status.h"
#include "t_user.h"
#include "fwd_transinfo.h"
#include "t_worker.h"
#include "t_user.h"
#include "t_peer.h"
#include "fwd_sock.h"

/*
1. create user/peer
2. user bind to each other
3. peer user connect to cm 
4. set user event 
5. change status
6. return.
*/
static dt_int t_session_st_init(t_session *sess)
{
	dt_assert(sess && session_base(sess));

	session_self_set(sess, t_user_create());
	session_peer_set(sess, t_user_create());

	if (!(session_self(sess) && session_peer(sess))) {
		dt_assert(0 && "bug???");
		return t_session_changestatus(sess, ST_SESSION_ERROR);
	}

	user_attach(session_self(sess), sess);
	user_attach(session_peer(sess), sess);
	user_peer_set(session_self(sess), session_peer(sess));
	user_peer_set(session_peer(sess), session_self(sess));
	user_retry_set(session_self(sess), t_main_retry());

	return t_session_changestatus(sess, ST_SESSION_NORMAL);
}

static dt_int t_session_st_normal(t_session *sess)
{
	t_user *user;

	user=session_self(sess);
	fwd_sock_setaddr(&user->fwd, t_main_cm_server());
	fwd_sock_setport(&user->fwd, t_main_cm_port());
	user->fwd_retry=t_main_fwd_times();

	if (t_main_fwd_args())
		str_set(&user->fwd_arg, t_main_fwd_args());
	else
		str_set(&user->fwd_arg, "");

	return t_user_fwd_change_status(user, ST_FWD_INIT);

//	return t_peer_cm_change_status(session_peer(sess), ST_CM_INIT);
}

static dt_int t_session_st_err(t_session *sess)
{
	return t_session_shutdown(sess, YES, YES);
}

t_session *t_session_create()
{
	t_session *sess = NULL;

	sess = (t_session *)dt_malloc(sizeof(t_session));

	if (sess) {
		if (t_session_init(sess) != T_OK) {
			dt_free(sess);
			sess = NULL;
		}
	}

	return sess;
}

dt_int t_session_init(t_session *sess)
{
	dt_assert(sess && "invalid args");

	memset(sess, 0, sizeof(t_session));

	session_self_set(sess, NULL);
	session_peer_set(sess, NULL);

	sess->st = ST_SESSION_MIN;

	fwd_trans_init(&sess->trans);
	str_init(&sess->uuid);
	dt_queue_init(&sess->list);

	session_attach(sess, NULL);

	return T_OK;
}

dt_int t_session_uninit(t_session *sess)
{
	dt_assert(sess && "invalid arg");

	return t_session_shutdown(sess, YES, NO);
}

dt_void t_session_destroy(t_session *sess)
{
	if (sess) {
		t_session_uninit(sess);

		dt_free(sess);
	}
}

static dt_void session_print(t_session *sess)
{
	t_trans *trans = &sess->trans;

	t_logger_info(sess, session_self(sess), "self");
	t_logger_info(sess, session_peer(sess), "peer");
	t_logger_info(sess, NULL, "\n"
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


dt_int t_session_shutdown(t_session *sess, dt_int detach, dt_int release)
{
	t_logger_trace(sess, session_self(sess),
			"shutdown session, detach? %s, release? %s", 
				detach?"yes":"no", release?"yes":"no");

	session_print(sess);

	t_user_destroy(session_self(sess));
	t_user_destroy(session_peer(sess));

	session_self_set(sess, NULL);
	session_peer_set(sess, NULL);

	sess->st = ST_SESSION_MIN;

	fwd_trans_uninit(&sess->trans);

	str_clear(&sess->uuid);

	if (release) {
		dt_assert(sess && session_base(sess));
		if (session_base(sess))
			session_base(sess)->left--;

		session_detach(sess);
		dt_free(sess);
		return T_OK;
	}

	if (detach) {
		dt_assert(sess && session_base(sess));
		if (session_base(sess))
			session_base(sess)->left--;
		session_detach(sess);
	}

	return T_OK;
}

dt_int t_session_changestatus(t_session *sess, t_session_status st)
{
	dt_int ret;
	dt_assert(sess && "invalid arg");

	if (!sess)
		return T_ERR;

	t_logger_trace(sess, NULL , "[sess] status [%s --> %s]", 
			t_session_status_str(session_status(sess)),
			t_session_status_str(st));

	sess->st = st;

	ret = T_OK;

	switch(st) {

	case ST_SESSION_INIT:
		ret = t_session_st_init(sess);
		break;

	case ST_SESSION_NORMAL:
		ret = t_session_st_normal(sess);
		break;

	case ST_SESSION_ERROR:
		ret = t_session_st_err(sess);
		break;

	default:
		break;
	}

	return ret;
}
