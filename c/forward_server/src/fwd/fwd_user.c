
#include "fwd_main.h"
#include "fwd_user.h"
#include "fwd_buffer.h"
#include "fwd_sock.h"
#include "fwd_session.h"
#include "fwd_logger.h"
#include "fwd_child.h"
#include "fwd_status.h"
#include "json.h"
#include "fwd_transinfo.h"


static dt_void user_timeout_cb(fwd_timeout *to);
static dt_void user_event_cb(fwd_event_fd *event, dt_int flg);

typedef dt_int (*user_timeout_func)(fwd_user *user);
typedef dt_int (*user_event_func)  (fwd_user *user, dt_int flg);


static dt_int user_event_accepted(fwd_user *user, dt_int flg);
static dt_int user_event_auth    (fwd_user *user, dt_int flg);
static dt_int user_event_waitting(fwd_user *user, dt_int flg);
static dt_int user_event_est     (fwd_user *user, dt_int flg);
static dt_int user_event_error   (fwd_user *user, dt_int flg);
static dt_int user_event_clean   (fwd_user *user, dt_int flg);


static dt_int user_timeout_accepted(fwd_user *user);
static dt_int user_timeout_auth (fwd_user *user);
static dt_int user_timeout_est  (fwd_user *user);
static dt_int user_timeout_error(fwd_user *user);
static dt_int user_timeout_waitting(fwd_user *user);
static dt_int user_timeout_clean(fwd_user *user);

static user_timeout_func  user_timeout_handler[] = {
	[ST_ACCEPTED] = user_timeout_accepted,
	[ST_AUTH]     = user_timeout_auth, 
	[ST_EST]      = user_timeout_est,
	[ST_ERROR]    = user_timeout_error,
	[ST_WATTING]  = user_timeout_waitting,
	[ST_CLEAN]    = user_timeout_clean,
	[ST_MAX]      = NULL
};

static user_event_func  user_event_handler[] = {
	[ST_ACCEPTED] = user_event_accepted,
	[ST_AUTH]     = user_event_auth,
	[ST_WATTING]  = user_event_waitting,
	[ST_EST]      = user_event_est,
	[ST_ERROR]    = user_event_error,
	[ST_CLEAN]    = user_event_clean,
	[ST_MAX]      = NULL
};

static dt_int user_status(fwd_user *user)
{
	dt_assert(fwd_user_status(user) > ST_MIN && fwd_user_status(user) < ST_MAX);
	
	if (fwd_user_status(user) > ST_MIN && fwd_user_status(user) < ST_MAX)
		return fwd_user_status(user);

	return ST_MAX;
}

#define user_timeout_run(user) do {				\
	if (user_timeout_handler[user_status(user)])		\
		user_timeout_handler[user_status(user)](user);	\
} while (0)

#define user_event_run(user, flg) do {				  \
	if(user_event_handler[user_status(user)]) 		  \
		user_event_handler[user_status(user)](user, flg); \
} while (0)

#define MSG_USER_REGISTER	0x1504

static dt_int user_send_msg(fwd_user *user, dt_uint tag, dt_int st)
{
	return fwd_buffer_pack(&user->buffer, tag, st, NULL, NULL);
}

static dt_int user_accept_handle_msg(fwd_user *user)
{
	fwd_request *req;

	dt_assert(user);

	req = (fwd_request *)buf_rd(&user->buffer);

	fwd_printhex(buf_rd(&user->buffer), buf_len(&user->buffer));

	if (ntohs(req->cmd) != MSG_USER_REGISTER) {
		fwd_logger_error(fwd_user_session(user), user,
			"invalid tag, shut it down: 0x%x, should be: 0x%x",
			ntohs(req->cmd), MSG_USER_REGISTER);
		user->reason = -FWD_ERR_INVALID_TAG;
		return FWD_ERR;
	}

	if (ntohs(req->len) > buf_len(&user->buffer)) 
		return FWD_CONTINUE;

	return FWD_OK;
}

static dt_int user_event_accepted(fwd_user *user, dt_int flg)
{
	dt_int ret;

	if (flg & FWD_EVT_READ) {
		ret = fwd_sock_read(&user->sock, &user->buffer);

		fwd_logger_trace(fwd_user_session(user), user, 
			"user read return: %d bytes on (%d), %d bytes total", 
			ret, fwd_sock_fd(&user->sock), buf_len(&user->buffer));

		if (ret <= 0 ) {
			if (ret == -EAGAIN) 
				return FWD_OK;
			user->reason = ret;
			fwd_user_changestatus(user, ST_CLEAN);
			return FWD_OK;
		}

		if (buf_len(&user->buffer) >= sizeof(fwd_request))  {

			switch(user_accept_handle_msg(user)) {

			case FWD_OK:
				fwd_user_changestatus(user, ST_AUTH);
				user_event_run(user, 0);
				return FWD_OK;

			case FWD_CONTINUE:
				break;
			default:
				fwd_user_changestatus(user, ST_CLEAN);
				return FWD_ERR;
			}
		}

		/* well, not enough, continue */
	}

	dt_assert(!(flg & FWD_EVT_WRITE) && "not care about this event");

	return FWD_OK;
}

static dt_int user_get_ticket(dt_cchar *str, dt_str *ticket)
{
	struct json_object *root, *obj;

	root = json_tokener_parse(str);

	if (is_error(root)) {
		log_warn("[cm] error parsing json: %s: %s", str, 
			json_tokener_errors[-(unsigned long)root]);
		return FWD_ERR_INVALID_ARG;
	}

	obj = json_object_object_get(root, "ticket");
	if (obj)
		str_set(ticket, json_object_get_string(obj));

	json_object_put(root);

	return FWD_OK;
}

static dt_int 
user_auth_and_bind_session(fwd_user *user, dt_cchar *ticket)
{
	fwd_session *sess = NULL;
	fwd_user    *peer = NULL;

	sess = fwd_main_session_get(ticket);

	if (!sess) {
		fwd_logger_warn(fwd_user_session(user), user, 
			"ticket: %s [invalid ticket]: not avaliable",
			ticket);

		return FWD_ERR_TICKET_NOT_EXIST;
	}

	if (    fwd_session_status(sess) == ST_EST
	    || (fwd_session_user1(sess) && fwd_session_user2(sess))
		) {
		fwd_logger_warn(sess, user,  
			"ticket: %s [invalid ticket]: in use",
			ticket);

		return FWD_ERR_TICKET_NOT_EXIST;
	}


	fwd_user_attach(sess, user);

	user->peer = fwd_session_next_user(sess, user);
	peer       = fwd_user_peer(user);

	/* peer ready? 
	   yes: release control
	   no:  both sess and current user goes into waitting status
	   */
	if (peer) {
		fwd_logger_trace(sess, user, "peer ready, release control and watting");

		dt_assert(fwd_user_status(peer) == ST_WATTING);
		dt_assert(fwd_session_next_user(sess, peer) == user);

		peer->peer = user;

		/* maybe left it @ status change */
		fwd_user_event_cancel(user);
		fwd_user_event_cancel(peer);
		fwd_user_timeout_cancel(peer);
		fwd_user_timeout_cancel(user);

		dt_assert(fwd_session_next_user(sess, peer) == user);
		dt_assert(fwd_user_peer(peer) == user && peer == fwd_user_peer(user));

		/* this gonna make session goes into established status */
		fwd_main_session_release_ctrl(sess);
	} else {
		fwd_logger_trace(sess, user, "peer's not ready, waitting ...");
		dt_assert(fwd_session_next_user(sess, user) == NULL);
		fwd_session_changestatus(sess, ST_WATTING);
		fwd_user_changestatus(user, ST_WATTING);
		user_event_run(user, 0);
	}

	return FWD_OK;
}

static dt_int user_event_auth(fwd_user *user, dt_int flg)
{
	fwd_request  req;
	dt_char      *val = NULL;
	dt_int       ret;
	dt_str       ticket;

	dt_assert(user);

	memset(&req, 0, sizeof(req));
	fwd_logger_trace(user->sess, user, "user event auth");

	str_init(&ticket);
	do {
		ret = FWD_ERR;

		ret = fwd_buffer_unpack(&user->buffer, &req, NULL, &val);

		if ((ret != OK) || (val == NULL)) {
			dt_assert(ret != FWD_CONTINUE);
			break;
		}

		user_get_ticket(val, &ticket);
		dt_free(val);

		fwd_logger_info(fwd_user_session(user), user, "user json: %s", str_text(&ticket));

		if (str_len(&ticket) <= 0) {
			fwd_logger_info(fwd_user_session(user), user, "ticket invalid");
			ret = FWD_ERR_INVALID_ARG;
			break;
		}

		ret = user_auth_and_bind_session(user, str_text(&ticket));
	} while (0);

	str_uninit(&ticket);

	if (ret != FWD_OK) {
		user->reason = -ret;
		fwd_user_changestatus(user, ST_CLEAN);
	}

	return ret;
}

static dt_int user_event_waitting(fwd_user *user, dt_int flg)
{
	fwd_logger_trace(fwd_user_session(user), user, 
			"user event @ watting, update the timeout ticket, cancel event?");

	return FWD_OK;
}

static dt_int user_event_read(fwd_session *sess, fwd_user *peer, fwd_user *user)
{
	dt_int      ret;

	dt_assert(user && fwd_user_peer(user) && fwd_user_session(user));
	dt_assert(sess == fwd_user_session(user));
	dt_assert(peer == fwd_user_peer(user));
	dt_assert(buf_left(&user->buffer) > 0);

	ret = fwd_sock_read(&user->sock, &user->buffer);

	fwd_logger_trace(sess, user, 
			"read [%d] bytes, buffer length: %d, left: %d",
			ret, buf_len(&user->buffer), buf_left(&user->buffer));

	if (ret <= 0) {
		if (ret == -EAGAIN)
			return FWD_OK;

		return FWD_ERR;
	}

	fwd_trans_read(&sess->trans, ret);

	if (buf_left(&user->buffer) <= 0) {
		fwd_logger_trace(sess, user, "user send speed too fast, slow it down forcely");
		if (buf_len(&peer->buffer) > 0)
			fwd_user_event_set(user, FWD_EVT_WRITE|FWD_EVT_EDGE_TRIGGER);
		else
			fwd_user_event_cancel(user);
	}

	if (buf_left(&peer->buffer) <= 0)
		fwd_user_event_set(peer, FWD_EVT_WRITE | FWD_EVT_EDGE_TRIGGER);
	else
		fwd_user_event_set(peer, FWD_EVT_WRITE| FWD_EVT_READ | FWD_EVT_EDGE_TRIGGER);

	return FWD_OK;
}

static dt_int user_event_write(fwd_session *sess, fwd_user *peer, fwd_user *user)
{
	dt_int      ret;

	dt_assert(user && fwd_user_peer(user) && fwd_user_session(user));
	dt_assert(sess == fwd_user_session(user));
	dt_assert(peer == fwd_user_peer(user));

	ret = fwd_sock_send(&user->sock, &peer->buffer);
	
	/*
	 * for decrease memcpy in memory
	 * */
	if (buf_left(&peer->buffer) <= (buf_size(&peer->buffer) >> 2))
		fwd_buffer_refresh(&peer->buffer);

	fwd_logger_trace(sess, user, 
			"send [%d] bytes, buffer length: %d, left: %d",
			ret, buf_len(&peer->buffer), buf_left(&peer->buffer));

	if (ret <= 0)
		return FWD_ERR;

	fwd_trans_write(&sess->trans, ret);
	
	if (buf_len(&peer->buffer) <= 0) {

		if (buf_left(&user->buffer) > 0)
			fwd_user_event_set(user, FWD_EVT_READ | FWD_EVT_EDGE_TRIGGER);
		else
			fwd_user_event_cancel(user);
	}

	if (buf_len(&user->buffer) <= 0)
		fwd_user_event_set(peer, FWD_EVT_READ | FWD_EVT_EDGE_TRIGGER);
	else
		fwd_user_event_set(peer, FWD_EVT_READ | FWD_EVT_WRITE | FWD_EVT_EDGE_TRIGGER);

	return FWD_OK;
}


/*

  read  +---------+  wr      +-------+  wr
------> |  User 1 | ------>  | User2 |-----> 
        +---------+          +-------+

case read failed: 
	if user1 has already recvd msgs, then
		1. set user2's write event
		2. unset user2's read event
		3. copy user1's buffer into user2's

		switch to ST_CLEAN: which goona used to send out the last words

	else:
		there's nothing left in user1, just shutdown

	
  wr   +---------+  wr      +-------+  read
  <--- |  User 1 | <-----   | User2 | <----
       +---------+          +-------+

case write failed:

	if user1 has recvd msgs, then
		1. set user2's write event
		2. unset user2's read event
		3. copy user1's buffer into user2's

		switch status into ST_CLEAN

	else:
		shutdown sessoin


handle error steps:
	1. unset self's event
	2. set peer event into write | edge_trigger (no timeout)
	3. clear peer buffer
	4. copy self's buffer into peer
	5. change user statuss into ST_CLEAN (no timeout)
	6. session change status into ST_CLEAN (set timeout)

*/

static dt_int user_event_est(fwd_user *user, dt_int flg)
{
	fwd_user    *peer;
	fwd_session *sess;

	dt_assert(user && fwd_user_peer(user) && fwd_user_session(user));

	peer = fwd_user_peer(user);
	sess = fwd_user_session(user);
	if (!(peer && sess))
		return fwd_session_shutdown(fwd_user_session(user));

	if (flg & FWD_EVT_READ) {
		if (user_event_read(sess, peer, user) != FWD_OK)
			goto err_out;
	}

	if (flg & FWD_EVT_WRITE) {
		if (user_event_write(sess, peer, user) != FWD_OK)
			goto err_out;
	}

	return FWD_OK;

err_out:
	if (buf_len(&user->buffer) > 0) {
		fwd_user_event_cancel(user);
		fwd_user_event_cancel(peer);

		// do it in status change
		//fwd_user_event_set(peer, FWD_EVT_WRITE|FWD_EVT_EDGE_TRIGGER);
		fwd_buffer_clear(&peer->buffer);
		memcpy(buf_wr(&peer->buffer), buf_rd(&user->buffer), buf_len(&user->buffer));
		fwd_buffer_seek_wr(&peer->buffer, buf_len(&user->buffer), FWD_BUFFER_SEEK_CUR);

		peer->reason = 0;
		fwd_user_changestatus(peer, ST_CLEAN);
		fwd_session_changestatus(sess, ST_CLEAN);
	} else
		return fwd_session_shutdown(sess);

	return FWD_OK;
}

static dt_int user_event_clean_flush(fwd_user *user)
{
	fwd_session *sess;
	dt_int ret;

	fwd_printhex(buf_rd(&user->buffer), buf_len(&user->buffer));

	sess = fwd_user_session(user);
	ret  = fwd_sock_send(&user->sock, &user->buffer);
	fwd_buffer_refresh(&user->buffer);

	fwd_logger_trace(sess, user, 
			"send [%d] bytes, buffer length: %d, left: %d",
			ret, buf_len(&user->buffer), buf_left(&user->buffer));

	if (ret <= 0)
		return FWD_ERR;

	if (sess)
		fwd_trans_write(&sess->trans, ret);

	return FWD_OK;
}

static dt_int user_event_clean(fwd_user *user, dt_int flg)
{
	dt_assert(user);

	if (user->reason > 0) {
		fwd_buffer_pack(&user->buffer, MSG_USER_REGISTER, user->reason, NULL, NULL);
		user->reason = 0;
	}

	if (flg & FWD_EVT_WRITE) {
		if (user_event_clean_flush(user) != FWD_OK)
			goto err_out;
	} else {
		dt_assert(0 && "should not be here");
		goto err_out;
	}

	if (buf_len(&user->buffer) > 0)
		return FWD_OK;

err_out:
	if (fwd_user_session(user))
		return fwd_session_shutdown(fwd_user_session(user));
	else {
		fwd_user_changestatus(user, ST_ERROR);
		user_event_run(user, 0);
	}

	return FWD_OK;
}

static dt_int user_event_error(fwd_user *user, dt_int flg)
{
	fwd_logger_trace(fwd_user_session(user), user, "user in error");

	if (fwd_user_session(user))
		return fwd_session_shutdown(fwd_user_session(user));
	else
		fwd_user_destroy(user);

	return FWD_OK;
}

static dt_void user_event_cb(fwd_event_fd *event, dt_int flg)
{
	fwd_user *user = dt_container_of(event, fwd_user, evt);

	fwd_logger_trace(fwd_user_session(user), user, 
		"user event: error? %s flg: 0x%x", event->error?"yes":"no", flg); 

	if (event->error) {
		user->reason = 0;
		fwd_user_changestatus(user, ST_ERROR);
	}

	user_event_run(user, flg);
}


static dt_int user_timeout_accepted(fwd_user *user)
{
	fwd_logger_trace(fwd_user_session(user), user, 
			"user did not send his own register info on, shut it down");

	user->reason = -FWD_ERR_WAIT_TIMEOUT;
	fwd_user_changestatus(user, ST_CLEAN);

	return FWD_OK;

}

static dt_int user_timeout_auth(fwd_user *user)
{
	fwd_logger_trace(fwd_user_session(user), user,
		"should not be here for now, cause the auth is really very fast");

	user->reason = -FWD_ERR_HANDLE_OVERTIME;
	fwd_user_changestatus(user, ST_CLEAN);

	return FWD_OK;
}

static dt_int user_timeout_est(fwd_user *user)
{
	dt_assert(0 && "should delete user's timeout timer @ this status, a bug");

	fwd_logger_trace(fwd_user_session(user), user,
			"should delete user's timeout timer @ this status, a bug");

	fwd_user_timeout_cancel(user);

	return FWD_OK;
}

static dt_int user_timeout_waitting(fwd_user *user)
{
	fwd_logger_trace(fwd_user_session(user), user, "user in waitting status timeout");

	user->reason = -FWD_ERR_WAIT_TIMEOUT;
	fwd_user_changestatus(user, ST_CLEAN);

	return FWD_OK;
}

static dt_int user_timeout_clean(fwd_user *user)
{
	fwd_logger_trace(fwd_user_session(user), user, "user in clean status timeout");

	user->reason = 0;
	fwd_user_changestatus(user, ST_ERROR);
	user_event_run(user, 0);

	return FWD_OK;
}

static dt_int user_timeout_error(fwd_user *user)
{
	fwd_logger_trace(fwd_user_session(user), user, 
			"user error timeout, delete user");

	if (fwd_user_session(user))
		fwd_session_shutdown(fwd_user_session(user));
	else
		fwd_user_destroy(user);
	
	return FWD_OK;
}

static dt_void user_timeout_cb(fwd_timeout *to)
{
	fwd_user *user = dt_container_of(to, fwd_user, timeout);

	user_timeout_run(user);
}


static dt_int user_st_accepted(fwd_user *user)
{
	dt_assert(user);

	fwd_user_event_set(user,   FWD_EVT_READ | FWD_EVT_EDGE_TRIGGER);
	fwd_user_timeout_set(user, FWD_TIMEOUT_USER_ACCEPTED);

	return FWD_OK;
}

static dt_int user_st_auth(fwd_user *user)
{
	dt_assert(user);

	fwd_user_timeout_set(user, FWD_TIMEOUT_AUTH);

	return FWD_OK;
}

static dt_int user_st_established(fwd_user *user)
{
	fwd_user *peer;

	dt_assert(user && fwd_user_session(user) && fwd_user_peer(user) && "bug");

	/*
	set peer's buffer and set self's write event.
	*/
	peer = fwd_user_peer(user);
	if (!peer)
		return FWD_ERR;
	
	user_send_msg(peer, MSG_USER_REGISTER, 0);

	fwd_user_event_set(user, FWD_EVT_WRITE | FWD_EVT_READ | FWD_EVT_EDGE_TRIGGER);

	return FWD_OK;
}

static dt_int user_st_waitting(fwd_user *user)
{
	dt_assert(user && fwd_user_peer(user) == NULL && fwd_user_session(user) && "bug");

	fwd_user_timeout_set(user, FWD_TIMEOUT_WAITTING);
	return FWD_OK;
}

static dt_int user_st_clean(fwd_user *user)
{
	dt_assert(user);

	fwd_logger_trace(user->sess, user, "user in clean...");
	fwd_printhex(buf_rd(&user->buffer), buf_len(&user->buffer));

	fwd_user_event_set(user, FWD_EVT_WRITE | FWD_EVT_EDGE_TRIGGER);
	fwd_user_timeout_set(user, FWD_TIMEOUT_SENDMSG);

	return FWD_OK;
}

fwd_user *fwd_user_create()
{
	fwd_user *user = NULL;

	user = (fwd_user *)dt_malloc(sizeof(fwd_user));

	if (user)
		fwd_user_init(user);
	
	return user;
}

dt_int fwd_user_attach(fwd_session *sess, fwd_user *user)
{
	dt_assert(sess && user)

	if (!fwd_session_user1(sess)) {
		fwd_session_user1_set(sess, user);
	} else {
		dt_assert(fwd_session_user2(sess)== NULL);
		fwd_session_user2_set(sess, user);
	}

	user->sess = sess;
	fwd_session_timeout_cancel(sess);

	return FWD_OK;
}

dt_int fwd_user_changestatus(fwd_user *user, fwd_status st)
{
	dt_int ret;
	dt_assert(user);

	if (!user)
		return FWD_ERR;

	fwd_logger_trace(user->sess, user, "user change status: %s --> %s",
				fwd_status_str(user->st), fwd_status_str(st));

	user->st = st;
	ret = FWD_OK;

	switch(fwd_user_status(user)) {
	
	case ST_ACCEPTED:
		ret = user_st_accepted(user);
		break;

	case ST_AUTH:
		ret = user_st_auth(user);
		break;

	case ST_EST:
		ret = user_st_established(user);
		break;

	case ST_WATTING:
		ret = user_st_waitting(user);
		break;

	case ST_CLEAN:
		ret = user_st_clean(user);
		break;
	default:
		break;
	}

	return ret;
}

dt_void
fwd_user_destroy(fwd_user *user)
{
	if (user) {
		fwd_user_uninit(user);

		dt_free(user);
	}
}

dt_int
fwd_user_init(fwd_user *user)
{
	dt_assert(user != NULL);

	memset(user, 0, sizeof(fwd_user));
	user->sess    = NULL;
	user->peer    = NULL;
	user->st      = ST_MIN;
	user->reason  = 0;

	fwd_buffer_init(&user->buffer, FWD_BUFFER_DEFAULT_SIZE);
	fwd_sock_init(&user->sock);

	return FWD_OK;
}

dt_int fwd_user_uninit(fwd_user *user)
{
	if (user) {
		fwd_logger_info(user->sess, user, "shutdown and destroied");

		fwd_user_detach(user);
		user->peer = NULL;
		user->st   = ST_MAX;
		user->reason  = 0;

		fwd_buffer_uninit(&user->buffer);
		fwd_sock_close(&user->sock);
		fwd_sock_clear(&user->sock);
	}

	return FWD_OK;
}

dt_int fwd_user_detach(fwd_user *user)
{
	fwd_session *sess;
	fwd_user    *peer;

	dt_assert(user);

	if (!user)
		return FWD_ERR;

	fwd_user_event_cancel(user);
	fwd_user_timeout_cancel(user);

	sess = fwd_user_session(user);
	if (sess) {
		if (fwd_session_user1(sess) == user) {
			fwd_session_user1_set(sess, NULL);
		} else {
			dt_assert(fwd_session_user2(sess) == user);
			fwd_session_user2_set(sess, NULL);
		}

		user->sess = NULL;
	}

	peer = fwd_user_peer(user);
	
	if (peer) {
		peer->peer = NULL;
		user->peer = NULL;
	}

	return FWD_OK;
}

dt_int  fwd_user_event_set(fwd_user *user, dt_int flgs)
{
	fwd_session *sess;

	dt_assert(user);

	user->evt.fd = fwd_sock_fd(&user->sock);
	switch(fwd_user_status(user)) {

	case ST_CLEAN:
	case ST_EST:
		sess = fwd_user_session(user);

		if (sess && sess->base)
			fwd_child_event_set(sess->base, &user->evt, flgs, user_event_cb);
		else
			fwd_main_event_set(&user->evt, flgs, user_event_cb);
		break;

	default:
		fwd_main_event_set(&user->evt, flgs, user_event_cb);
		break;
	}

	return FWD_OK;
}

dt_int  fwd_user_event_cancel(fwd_user *user)
{
	fwd_session *sess;

	dt_assert(user);

	sess = fwd_user_session(user);

	if (sess && sess->base)
		fwd_child_event_cancel(sess->base, &user->evt);
	else
		fwd_main_event_cancel(&user->evt);

	return FWD_OK;
}

dt_int  fwd_user_timeout_set(fwd_user *user, dt_int timeout)
{
	fwd_session *sess;
	dt_assert(user);

	switch(fwd_user_status(user)) {

	case ST_CLEAN:
	case ST_EST:
		sess = fwd_user_session(user);

		if (sess && sess->base)
			fwd_child_timeout_set(sess->base, &user->timeout, timeout, user_timeout_cb, 0);
		else
			fwd_main_timeout_set(&user->timeout, timeout, user_timeout_cb, 0);
		break;

	default:
		fwd_main_timeout_set(&user->timeout, timeout, user_timeout_cb, 0);
		break;
	}

	return FWD_OK;
}

dt_int fwd_user_timeout_cancel(fwd_user *user)
{
	fwd_session *sess;

	dt_assert(user);

	sess = fwd_user_session(user);

	if (sess &&(sess->base))
		fwd_child_timeout_cancel(&user->timeout);
	else
		fwd_main_timeout_cancel(&user->timeout);

	return FWD_OK;
}
