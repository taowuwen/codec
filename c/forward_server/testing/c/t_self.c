
#include "t_main.h"
#include "t_user.h"
#include "t_session.h"
#include "t_logger.h"
#include "fwd_sock.h"
#include "fwd_buffer.h"
#include "t_worker.h"
#include "json.h"
#include "t_self.h"
#include "t_status.h"
#include "fwd_transinfo.h"

static dt_void self_cm_timeoub_cb(t_timeout *to);
static dt_void self_cm_evt_cb(t_event_fd *evt, dt_int flgs);

static dt_int self_cm_timeout_request_fwd(t_user *user);

static user_timeout_func self_cm_timeout_handler[] = 
{
	[ST__CM_INIT]        = NULL,
	[ST__CM_REQUEST_FWD] = self_cm_timeout_request_fwd,
	[ST__CM_MAX]	    = NULL
};


static dt_int self_cm_evt_init(t_user *user, dt_int flgs);
static dt_int self_cm_evt_request_fwd(t_user *user, dt_int flgs);

static user_event_func self_cm_evt_handler[] =
{
	[ST__CM_INIT]        = self_cm_evt_init,
	[ST__CM_REQUEST_FWD] = self_cm_evt_request_fwd,
	[ST__CM_MAX]         = NULL
};


static dt_int self_cm_timeout_run(t_user *user)
{
	dt_assert(user && user_session(user) && "bug");

	if (self_cm_timeout_handler[user_cm_status(user)])
		return self_cm_timeout_handler[user_cm_status(user)](user);

	return T_OK;
}

static dt_int self_cm_evt_run(t_user *user, dt_int flgs)
{
	dt_assert(user && user_session(user) && "bug");

	if (self_cm_evt_handler[user_cm_status(user)])
		return self_cm_evt_handler[user_cm_status(user)](user, flgs);

	return T_OK;
}


static dt_int self_cm_request_fill(t_user *user)
{
#pragma pack(push, 1)
typedef struct {
	unsigned short	len;
	unsigned short  ver;
	unsigned short  cmd;
	char		uuid[64];
} t_ticket_rsq;
#pragma pack(pop)
	t_ticket_rsq *rsq = (t_ticket_rsq *)buf_wr(&user->cm_buf);

	dt_assert(user && user_session(user));

	rsq->len = htons(sizeof(t_ticket_rsq));
	rsq->ver = htons(1);
	rsq->cmd = htons(MSG_CM_FWD_REQUEST);

	dt_assert(session_uuid(user_session(user)) && "peer set, not self");
	snprintf(rsq->uuid, 64, "%s", session_uuid(user_session(user)));

	fwd_buffer_seek_wr(&user->cm_buf, sizeof(t_ticket_rsq), FWD_BUFFER_SEEK_CUR);

	return T_OK;
}


static dt_int self_cm_timeout_request_fwd(t_user *user)
{
	t_logger_info(user_session(user), user, "user request for fwd timeout");

	if (user_retry(user)) {
		user_retry_set(user, user_retry(user) -1);

		user_cm_timeout_cancel(user);
		user_cm_event_cancel(user);
		fwd_sock_close(&user->cm);

		return t_self_cm_change_status(user, ST__CM_INIT);
	}

	return t_session_changestatus(user_session(user), ST_SESSION_ERROR);
}

static dt_int self_cm_evt_init(t_user *user, dt_int flgs)
{
	dt_assert(user && (user == session_self(user_session(user))));

	fwd_sock_setaddr(&user->cm, t_main_cm_server());
	fwd_sock_setport(&user->cm, t_main_cm_port());

	if (t_user_cm_connect(user) != T_OK)
		return t_session_changestatus(user_session(user), ST_SESSION_ERROR);

	self_cm_request_fill(user);

	user_cm_timeout_set(user, T_TIMEOUT_GETTICKET, self_cm_timeoub_cb);
	user_cm_event_set(user, FWD_EVT_WRITE | FWD_EVT_EDGE_TRIGGER, self_cm_evt_cb);

	return t_self_cm_change_status(user, ST__CM_REQUEST_FWD);
}

static dt_int self_cm_evt_request_fwd(t_user *user, dt_int flgs)
{
	dt_int ret;
	dt_int again;

	if (flgs & FWD_EVT_WRITE) {
		ret = fwd_sock_send(&user->cm, &user->cm_buf);

		if (ret <= 0)
			return t_session_changestatus(user_session(user), ST_SESSION_ERROR);

		if (buf_len(&user->cm_buf) <= 0) {
			user_cm_event_set(user, FWD_EVT_READ|FWD_EVT_EDGE_TRIGGER, self_cm_evt_cb);
			fwd_buffer_refresh(&user->cm_buf);
		}

		return T_OK;
	}

	if (flgs & FWD_EVT_READ) {
		again = YES;
rd_again:
		ret = fwd_sock_read(&user->cm, &user->cm_buf);

		if (ret <= 0) {
			if (ret == -EAGAIN)
				return T_OK;
			return t_session_changestatus(user_session(user), ST_SESSION_ERROR);
		}

		if (buf_left(&user->cm_buf) > 0)
			again = NO;

		if (user_fwd_status(user) == ST_FWD_MIN) {

			switch(t_user_cm_handle_msg(user)) {

			case T_OK:
				user_cm_timeout_cancel(user);
				t_user_fwd_change_status(user, ST_FWD_INIT); /* ready to fwd */
				break;

			case FWD_CONTINUE:
				break;

			default:
				return t_session_changestatus(user_session(user), ST_SESSION_ERROR);
			}
		} else
			fwd_buffer_clear(&user->cm_buf);

		if (again)
			goto rd_again;

		return T_OK;
	}

	dt_assert(0 && "should not be here for now");

	return T_ERR;
}

static dt_void self_cm_timeoub_cb(t_timeout *to)
{
	t_user *user = dt_container_of(to, t_user, cm_to);

	self_cm_timeout_run(user);
}

static dt_void self_cm_evt_cb(t_event_fd *evt, dt_int flgs)
{
	t_user *user = dt_container_of(evt, t_user, cm_evt);

	t_logger_trace(user_session(user), user,
			"[cm] evt, error?%s, flg: 0x%x", evt->error?"yes":"no", flgs);

	if (evt->error){
		t_logger_warn(user_session(user), user, "[cm]event error: %s", t_lasterrstr());
		t_session_changestatus(user_session(user), ST_SESSION_ERROR);
	} else
		self_cm_evt_run(user, flgs);
}

static dt_int self_cm_st_init(t_user *user)
{
	return self_cm_evt_run(user, 0);
}

dt_int t_self_cm_change_status(t_user *user, t_cm_status st)
{
	dt_int ret;
	dt_assert(user && user_session(user) && "bug");
	dt_assert(user == session_self(user_session(user)));

	t_logger_trace(user_session(user), user, "[cm] change status [%s --> %s]",
				t_cm_status_str(user_cm_status(user)), t_cm_status_str(st));

	ret = T_OK;
	user->cm_st = st;

	switch(st) {

	case ST__CM_INIT:
		ret = self_cm_st_init(user);
		break;

	case ST__CM_REQUEST_FWD:
		break;

	default:
		dt_assert(0 && "bug?");
		break;
	}

	return ret;
}


/*******************************************************************************

	fwd information goes from here
*******************************************************************************/

static dt_void self_fwd_timeout_cb(t_timeout *to);
static dt_void self_fwd_evt_cb(t_event_fd *evt, dt_int flgs);

static dt_int self_fwd_est(t_user *user, dt_int flgs);
static dt_int self_fwd_hb(t_user *user, dt_int flgs);
static dt_int self_fwd_sendfile(t_user *user, dt_int flgs);

static dt_int self_fwd_timeout_est(t_user *user);

static user_timeout_func self_fwd_timeout_handler[] =
{
	[ST_FWD_EST]      = self_fwd_timeout_est,
	[ST_FWD_MAX]      = NULL
};


static user_event_func   self_fwd_handler[] = 
{
	[ST_FWD_EST]      = self_fwd_est,
	[ST_FWD_HB]       = self_fwd_hb,
	[ST_FWD_SENDFILE] = self_fwd_sendfile,
	[ST_FWD_MAX]      = NULL
};


static dt_int self_fwd_evt_run(t_user *user, dt_int flgs)
{
	dt_assert(user && user_session(user) && "bug");
	dt_assert(user == session_self(user_session(user)));

	if (self_fwd_handler[user_fwd_status(user)])
		return self_fwd_handler[user_fwd_status(user)](user, flgs);

	dt_assert(0 && "should not be here for now.");
	return T_ERR;
}

static dt_int self_fwd_timeout_run(t_user *user)
{
	dt_assert(user && user_session(user) && "bug");
	dt_assert(user == session_self(user_session(user)));

	if (self_fwd_timeout_handler[user_fwd_status(user)])
		return self_fwd_timeout_handler[user_fwd_status(user)](user);

	dt_assert(0 && "should not be here for now.");
	return T_ERR;
}

static dt_int file_pack_cb(dt_char *buf, dt_int len, dt_cchar *arg)
{
	t_user *user = (t_user *)arg;
	dt_int cp;

	cp = len - 1;

	if (cp > str_len(&user->fl.marks))
		cp = str_len(&user->fl.marks);

	memcpy(buf, str_text(&user->fl.marks), cp);
	buf[cp] = '\0';

	user->fl.left -= cp;

	return T_OK;
}

static dt_int self_fwd_send_file(t_user *user)
{
	user_fwd_event_cancel(user);

	if (fwd_buffer_pack(&user->fwd_out, T_MSG_FILE, 0, file_pack_cb, (dt_cchar *)user) 
				== FWD_BUFFER_FULL) {
		t_logger_warn(user_session(user), user, "send file buffer full, no left space");
	}

	fwd_printhex(buf_rd(&user->fwd_out), 8);
	user_fwd_event_set(user, FWD_EVT_WRITE|FWD_EVT_EDGE_TRIGGER, self_fwd_evt_cb);

	return T_OK;
}

static dt_int self_fwd_send_heartbeat(t_user *user)
{
	if (fwd_buffer_pack(&user->fwd_out, T_MSG_HELLO, 0, NULL, NULL) == FWD_BUFFER_FULL) {
		t_logger_warn(user_session(user), user, "heartbeat buffer full, no left space");
	}

	user_fwd_event_set(user, FWD_EVT_WRITE|FWD_EVT_EDGE_TRIGGER, self_fwd_evt_cb);
	user_fwd_timeout_set(user, T_TIMEOUT_FWD_HB, self_fwd_timeout_cb);

	return T_OK;
}

dt_int t_self_fwd_established(t_user *user, dt_int flgs)
{
	user->n_hb = t_main_hb();
	if (t_test(T_FLG_SEND_FILE))
		user->flgs = T_FLG_SEND_FILE;

	user_fwd_event_cancel(user);
	user_fwd_timeout_cancel(user);

	return self_fwd_send_heartbeat(user);
}

static dt_int self_fwd_write(t_user *user)
{
	dt_int ret;
	ret = fwd_sock_send(&user->fwd, &user->fwd_out);

	if (ret <= 0)
		return t_session_changestatus(user_session(user), ST_SESSION_ERROR);

	fwd_trans_write(&user->fwd_trans, ret);
	fwd_trans_write(&user_session(user)->trans, ret);
	if (buf_len(&user->fwd_out) <= 0) {
		fwd_buffer_refresh(&user->fwd_out);
		user_fwd_event_set(user, FWD_EVT_READ|FWD_EVT_EDGE_TRIGGER, self_fwd_evt_cb);
	}

	return T_OK;
}

static dt_int self_fwd_msg_hello(t_user *user, fwd_response *rsp, dt_int len, dt_char *val)
{
	t_logger_trace(user_session(user), user, "heart beat responsed");

	if (user->flgs & T_FLG_SEND_FILE) {
		user->flgs ^= T_FLG_SEND_FILE;
		user->fl.sz_fl = user->fl.left = t_main_file_size();

		{
			dt_char *src = NULL;
			dt_char  magic = (char )(random()%255 + 1);

			src = (dt_char *)dt_malloc(1024 * sizeof(dt_char) + 4);
			
			if (src) {
				memset(src, magic, 1024);
				src[1024] = '\0';
				str_set(&user->fl.marks, src);

				dt_free(src);
			}
		}

		user_fwd_timeout_cancel(user);
		return self_fwd_send_file(user);
	}

	return T_OK;
}

static dt_int self_fwd_msg_file(t_user *user, fwd_response *rsp, dt_int len, dt_char *val)
{
	if (rsp->st != 0){
		t_logger_trace(user_session(user), user, "file response error: %d", rsp->st);
		return t_session_changestatus(user_session(user), ST_SESSION_ERROR);
	}

	if (user->fl.left > 0)
		return self_fwd_send_file(user);

	return t_session_changestatus(user_session(user), ST_SESSION_ERROR);
}

static dt_int self_fwd_handle_rsp(t_user *user)
{
	dt_int       ret;
	fwd_response rsp;
	dt_int        len = 0;
	dt_char	     *val = NULL;

	ret = fwd_buffer_unpack(&user->fwd_in, (fwd_request *)&rsp, NULL, &val);

	if (ret == T_OK) {
		fwd_buffer_refresh(&user->fwd_in);

		switch(rsp.cmd) {

		case T_MSG_HELLO:
			self_fwd_msg_hello(user, &rsp, len, val);
			break;

		case T_MSG_FILE:
			self_fwd_msg_file(user, &rsp, len, val);
			break;

		default:
			break;
		}

		dt_free(val);
	}

	return ret;

}

static dt_int self_fwd_read(t_user *user)
{
	dt_int ret;
	dt_int again;

	again = NO;
rd_again:
	ret = fwd_sock_read(&user->fwd, &user->fwd_in);
	
	if (ret <= 0) {
		if (ret == -EAGAIN)
			return T_OK;
		return t_session_changestatus(user_session(user), ST_SESSION_ERROR);
	}

	fwd_trans_read(&user->fwd_trans, ret);
	fwd_trans_read(&user_session(user)->trans, ret);

	if (buf_left(&user->fwd_in) <= 0)
		again = YES;

	switch(self_fwd_handle_rsp(user)) {

	case T_ERR:
		return t_session_changestatus(user_session(user), ST_SESSION_ERROR);
		break;

	default:
		break;
	}

	if (again) {
		if(buf_left(&user->fwd_in) <= 0) {
			t_logger_error(user_session(user), user, "user has no left space for read");
			return t_session_changestatus(user_session(user), ST_SESSION_ERROR);
		}
		goto rd_again;
	}

	return T_OK;
}

static dt_int self_fwd_est(t_user *user, dt_int flgs)
{
	if (flgs & FWD_EVT_WRITE)
		return self_fwd_write(user);

	if (flgs & FWD_EVT_READ)
		return self_fwd_read(user);

	dt_assert(0 && "should not be here for now");
	return T_OK;
}

static dt_int self_fwd_hb(t_user *user, dt_int flgs)
{
	dt_assert(0 && "should not be here for now");
	return T_OK;
}

static dt_int self_fwd_sendfile(t_user *user, dt_int flgs)
{
	dt_assert(0 && "should not be here for now");
	return T_OK;
}

static dt_void self_fwd_timeout_cb(t_timeout *to)
{
	t_user *user = dt_container_of(to, t_user, fwd_to);

	self_fwd_timeout_run(user);
}

static dt_void self_fwd_evt_cb(t_event_fd *evt, dt_int flgs)
{
	t_user *user = dt_container_of(evt, t_user, fwd_evt);

	dt_assert(user && user_session(user) && "bug");
	dt_assert(dt_validblock(user) && dt_validblock(user_session(user)));

	t_logger_trace(user_session(user), user, 
		"[fwd] event, error? %s, flgs: 0x%x", evt->error?"yes":"no", flgs);

	if (evt->error) {
		t_logger_error(user_session(user), user, "[fwd] event: %s", t_lasterrstr());
		t_session_changestatus(user_session(user), ST_SESSION_ERROR);
	} else
		self_fwd_evt_run(user, flgs);
}

static dt_int self_fwd_timeout_est(t_user *user)
{
	if (user->n_hb) {
		self_fwd_send_heartbeat(user);
		user->n_hb--;
	} else {
		t_logger_trace(user_session(user), user, "[fwd] heartbeat finished");
		return t_session_changestatus(user_session(user), ST_SESSION_ERROR);
	}
		
	return T_OK;
}
