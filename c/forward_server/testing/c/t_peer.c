
#include "t_main.h"
#include "t_user.h"
#include "t_session.h"
#include "t_logger.h"
#include "fwd_sock.h"
#include "fwd_buffer.h"
#include "t_worker.h"
#include "json.h"
#include "t_peer.h"
#include "t_self.h"
#include "t_status.h"
#include "fwd_transinfo.h"


static dt_void peer_cm_timeout_cb(t_timeout *to);
static dt_void peer_cm_evt_cb(t_event_fd *evt, dt_int flgs);

static dt_int peer_cm_timeout_register(t_user *user);
static dt_int peer_cm_timeout_hb(t_user *user);
static dt_int peer_cm_timeout_sendhb(t_user *user);

static user_timeout_func peer_cm_timeout_handler[] = 
{
	[ST__CM_INIT]     = NULL,
	[ST__CM_REGISTER] = peer_cm_timeout_register,
	[ST__CM_HB]       = peer_cm_timeout_hb,
	[ST__CM_SENDHB]   = peer_cm_timeout_sendhb,
	[ST__CM_MAX]	 = NULL
};


static dt_int peer_cm_evt_init(t_user *user, dt_int flgs);
static dt_int peer_cm_evt_register(t_user *user, dt_int flgs);
static dt_int peer_cm_evt_hb(t_user *user, dt_int flgs);
static dt_int peer_cm_evt_sendhb(t_user *user, dt_int flgs);

static user_event_func   peer_cm_evt_handler[] = 
{
	[ST__CM_INIT]     = peer_cm_evt_init,
	[ST__CM_REGISTER] = peer_cm_evt_register,
	[ST__CM_HB]       = peer_cm_evt_hb,
	[ST__CM_SENDHB]   = peer_cm_evt_sendhb,
	[ST__CM_MAX]      = NULL
};

static dt_int peer_cm_timeout_run(t_user *user)
{
	dt_assert(user && user_session(user) && "bug");
	dt_assert(user_cm_status(user) > ST__CM_MIN && user_cm_status(user) <= ST__CM_SENDHB);

	if (peer_cm_timeout_handler[user_cm_status(user)])
		return peer_cm_timeout_handler[user_cm_status(user)](user);

	return T_OK;
}


static dt_int peer_cm_evt_run(t_user *user, dt_int flgs)
{
	dt_assert(user && user_session(user) && "bug");

	dt_assert(user_cm_status(user) > ST__CM_MIN && user_cm_status(user) <= ST__CM_SENDHB);

	if (peer_cm_evt_handler[user_cm_status(user)])
		return peer_cm_evt_handler[user_cm_status(user)](user, flgs);

	return T_OK;
}

static dt_int peer_gen_uuid(t_user *user, dt_char *buf, dt_int len)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	snprintf(buf, len, "%08lx-%ld-%08lx-%p-%08lx-%08lx-%ld",
		tv.tv_usec, tv.tv_usec, random(), user, random(), tv.tv_sec, tv.tv_sec);

	return T_OK;
}


static dt_int peer_cm_hb_set(t_user *user)
{
#pragma pack(push, 1)
typedef struct {
	unsigned short	len;
	unsigned short  ver;
	unsigned short  cmd;
	
	struct {
		char	lan_ip[4];
		unsigned short	lan_udp_port;
		unsigned short	lan_tcp_port;

		char	pub_ip[4];
		unsigned short	pub_udp_port;
		unsigned short	upnp_tcp_port;

		char	device[32];
	} device;

	char	id[32];
	char	username[32];
	char	uuid[64];
	char	rsrv[32];
}t_p2p_reg;
#pragma pack(pop)

	t_p2p_reg *p2p = (t_p2p_reg *)buf_wr(&user->cm_buf);

	memset(p2p, 0, sizeof(t_p2p_reg));

	p2p->len = htons(sizeof(t_p2p_reg));
	p2p->ver = htons(1);
	p2p->cmd = htons(MSG_CM_P2P_REG);

	if (!session_uuid(user_session(user))) {
		peer_gen_uuid(user, p2p->uuid, 64);
		session_uuid_set(user_session(user), p2p->uuid);
	} else {
		snprintf(p2p->uuid, 64, "%s", session_uuid(user_session(user)));
	}
	snprintf(p2p->id,   32, "%s", p2p->uuid);

	fwd_buffer_seek_wr(&user->cm_buf, sizeof(t_p2p_reg), FWD_BUFFER_SEEK_CUR);

	return T_OK;
}

static dt_int peer_cm_timeout_register(t_user *user)
{
	dt_assert(user && user_session(user) && "bug");

	t_logger_trace(user_session(user), user, "[cm] timeout for register");

	return t_session_changestatus(user_session(user), ST_SESSION_ERROR);
}

static dt_int peer_cm_timeout_hb(t_user *user)
{
	t_logger_trace(user_session(user), user, "[cm] time to send hb");

	peer_cm_hb_set(user);

	user_cm_timeout_set(user, T_TIMEOUT_CM_SEND_HB, peer_cm_timeout_cb);
	user_cm_event_set(user, FWD_EVT_WRITE|FWD_EVT_EDGE_TRIGGER, peer_cm_evt_cb);

	return t_peer_cm_change_status(user, ST__CM_SENDHB);
}

static dt_int peer_cm_timeout_sendhb(t_user *user)
{
	dt_assert(user && user_session(user) && "bug");

	t_logger_trace(user_session(user), user, "[cm] timeout for send hb msg");

	return t_session_changestatus(user_session(user), ST_SESSION_ERROR);
}

static dt_int peer_cm_evt_init(t_user *user, dt_int flgs)
{
	dt_assert(user && user_session(user) && "bug");
	dt_assert(user == session_peer(user_session(user)) && "bug");

	fwd_sock_setaddr(&user->cm, t_main_cm_server());
	fwd_sock_setport(&user->cm, t_main_cm_port());

	if (t_user_cm_connect(user) != OK)
		return t_session_changestatus(user_session(user), ST_SESSION_ERROR);

	peer_cm_hb_set(user);

	user_cm_timeout_set(user, T_TIMEOUT_CONNECT_CM, peer_cm_timeout_cb);
	user_cm_event_set(user, FWD_EVT_WRITE|FWD_EVT_EDGE_TRIGGER, peer_cm_evt_cb);

	return t_peer_cm_change_status(user, ST__CM_REGISTER);
}

static dt_int peer_cm_evt_register(t_user *user, dt_int flgs)
{
	dt_int ret;
	if (flgs & FWD_EVT_WRITE) {
		ret = fwd_sock_send(&user->cm, &user->cm_buf);

		if (ret <= 0)
			return t_session_changestatus(user_session(user), ST_SESSION_ERROR);

		if (buf_len(&user->cm_buf) <= 0) {
			t_peer_cm_change_status(user, ST__CM_HB);
			fwd_buffer_refresh(&user->cm_buf);

			dt_assert(user_peer(user) && user_peer(user) == session_self(user_session(user)));
			t_self_cm_change_status(user_peer(user), ST__CM_INIT);
		}

		return T_OK;
	}

	dt_assert(0 && "should not be here");
	return T_ERR;
}

static dt_int peer_cm_evt_hb(t_user *user, dt_int flgs)
{
	dt_int ret;
	dt_int again;

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

static dt_int peer_cm_evt_sendhb(t_user *user, dt_int flgs)
{
	dt_int ret;

	if (flgs & FWD_EVT_WRITE) {
		ret = fwd_sock_send(&user->cm, &user->cm_buf);

		if (ret <= 0)
			return t_session_changestatus(user_session(user), ST_SESSION_ERROR);

		if (buf_len(&user->cm_buf) <= 0) {
			t_peer_cm_change_status(user, ST__CM_HB);
			fwd_buffer_refresh(&user->cm_buf);
		}

		return T_OK;
	}

	dt_assert(0 && "should not be here for now");
	return T_ERR;
}


static dt_void peer_cm_timeout_cb(t_timeout *to)
{
	t_user *user = dt_container_of(to, t_user, cm_to);

	peer_cm_timeout_run(user);
}

static dt_void peer_cm_evt_cb(t_event_fd *evt, dt_int flgs)
{
	t_user *user = dt_container_of(evt, t_user, cm_evt);

	t_logger_trace(user_session(user), user,
			"[cm] evt, error?%s, flg: 0x%x", evt->error?"yes":"no", flgs);

	if (evt->error){
		t_logger_warn(user_session(user), user, "[cm]event error: %s", t_lasterrstr());
		t_session_changestatus(user_session(user), ST_SESSION_ERROR);
	} else
		peer_cm_evt_run(user, flgs);
}


static dt_int peer_cm_st_init(t_user *user)
{
	return peer_cm_evt_run(user, 0);
}

static dt_int peer_cm_st_hb(t_user *user)
{
	user_cm_timeout_set(user, T_TIMEOUT_CM_HB, peer_cm_timeout_cb);
	user_cm_event_set(user, FWD_EVT_READ|FWD_EVT_EDGE_TRIGGER, peer_cm_evt_cb);

	return T_OK;
}


dt_int t_peer_cm_change_status(t_user *user, t_cm_status st)
{
	dt_int ret;

	dt_assert(user && user_session(user) && "bug");
	dt_assert(user == session_peer(user_session(user)));

	t_logger_trace(user_session(user), user, "[cm] change status [%s --> %s]",
				t_cm_status_str(user_cm_status(user)), t_cm_status_str(st));

	ret = T_OK;
	user->cm_st = st;

	switch(st) {

	case ST__CM_INIT:
		ret = peer_cm_st_init(user);
		break;

	case ST__CM_REGISTER:
		break;

	case ST__CM_HB:
		ret = peer_cm_st_hb(user);
		break;

	case ST__CM_SENDHB:
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
//static dt_void peer_fwd_timeout_cb(t_timeout *to);
static dt_void peer_fwd_evt_cb(t_event_fd *evt, dt_int flgs);

static dt_int peer_fwd_est(t_user *user, dt_int flgs);
static dt_int peer_fwd_hb(t_user *user, dt_int flgs);
static dt_int peer_fwd_sendfile(t_user *user, dt_int flgs);


static user_event_func   peer_fwd_handler[] = 
{
	[ST_FWD_EST]      = peer_fwd_est,
	[ST_FWD_HB]       = peer_fwd_hb,
	[ST_FWD_SENDFILE] = peer_fwd_sendfile,
	[ST_FWD_MAX]      = NULL
};


static dt_int peer_fwd_evt_run(t_user *user, dt_int flgs)
{
	dt_assert(user && user_session(user) && "bug");
	dt_assert(user == session_peer(user_session(user)));

	if (peer_fwd_handler[user_fwd_status(user)])
		return peer_fwd_handler[user_fwd_status(user)](user, flgs);

	dt_assert(0 && "should not be here for now.");
	return T_ERR;
}

dt_int t_peer_fwd_established(t_user *user, dt_int flgs)
{
	return peer_fwd_evt_run(user, flgs);
}

static dt_int peer_fwd_msg_hello(t_user *user, dt_int len, dt_char *val)
{
	t_logger_trace(user_session(user), user, "heart beat");

	fwd_buffer_pack(&user->fwd_out, T_MSG_HELLO, 0, NULL, NULL);
	user_fwd_event_set(user, FWD_EVT_WRITE|FWD_EVT_EDGE_TRIGGER, peer_fwd_evt_cb);
	return T_OK;
}

static dt_int peer_fwd_msg_file(t_user *user, dt_int len, dt_char *val)
{
	t_logger_trace(user_session(user), user, "user recved len: %d", len);
	fwd_buffer_pack(&user->fwd_out, T_MSG_FILE, 0, NULL, NULL);
	user_fwd_event_set(user, FWD_EVT_WRITE|FWD_EVT_EDGE_TRIGGER, peer_fwd_evt_cb);
	return T_OK;
}

static dt_int peer_fwd_handle_request(t_user *user)
{
	dt_int       ret;
	fwd_request  req;
	dt_int       len;
	dt_char	     *val = NULL;

	ret = fwd_buffer_unpack(&user->fwd_in, &req, &len, &val);

	if (ret == T_OK) {

		fwd_buffer_refresh(&user->fwd_in);
		switch(req.cmd) {

		case T_MSG_HELLO:
			peer_fwd_msg_hello(user, len, val);
			break;

		case T_MSG_FILE:
			peer_fwd_msg_file(user, len, val);
			break;

		default:
			break;
		}

		dt_free(val);
	}

	return ret;
}

static dt_int peer_fwd_read(t_user *user)
{
	dt_int ret;
	dt_int again;

	again = NO;

rd_again:
	ret = fwd_sock_read(&user->fwd, &user->fwd_in);

	t_logger_trace(user_session(user), user, "[est] read %d bytes", ret);

	if (ret <= 0) {
		if (ret == -EAGAIN)
			return T_OK;
		return t_session_changestatus(user_session(user), ST_SESSION_ERROR);
	}

	if (buf_left(&user->fwd_in) <= 0)
		again = YES;

	fwd_trans_read(&user->fwd_trans, ret);
	fwd_trans_read(&user_session(user)->trans, ret);

	switch(peer_fwd_handle_request(user)) {

	case T_ERR:
		return t_session_changestatus(user_session(user), ST_SESSION_ERROR);

	default:
		break;
	}

	if (again)
		goto rd_again;

	return T_OK;
}

static dt_int peer_fwd_write(t_user *user)
{
	dt_int ret;

	ret = fwd_sock_send(&user->fwd, &user->fwd_out);

	if (ret <= 0)
		return t_session_changestatus(user_session(user), ST_SESSION_ERROR);

	fwd_trans_write(&user->fwd_trans, ret);
	fwd_trans_write(&user_session(user)->trans, ret);

	if (buf_len(&user->fwd_out) <= 0) {
		fwd_buffer_refresh(&user->fwd_out);
		user_fwd_event_set(user, FWD_EVT_READ|FWD_EVT_EDGE_TRIGGER, peer_fwd_evt_cb);
	}

	return T_OK;
}

static dt_int peer_fwd_est(t_user *user, dt_int flgs)
{
	if (flgs & FWD_EVT_READ)
		return peer_fwd_read(user);


	if (flgs & FWD_EVT_WRITE)
		return peer_fwd_write(user);

	dt_assert(0 && "should not be here for now");

	return T_ERR;
}

static dt_int peer_fwd_hb(t_user *user, dt_int flgs)
{
	dt_assert(0 && "should not be here for now");

	return T_ERR;
}

static dt_int peer_fwd_sendfile(t_user *user, dt_int flgs)
{
	dt_assert(0 && "should not be here for now");

	return T_ERR;
}

/*
static dt_void peer_fwd_timeout_cb(t_timeout *to)
{
	t_user *user = dt_container_of(to, t_user, fwd_to);

	t_logger_info(user_session(user), user, 
		"peer fwd timeout triger, not handle for now, just shutdown session");
}
*/

static dt_void peer_fwd_evt_cb(t_event_fd *evt, dt_int flgs)
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
		peer_fwd_evt_run(user, flgs);
}
