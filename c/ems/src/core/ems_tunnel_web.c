
#include "ems_core.h"
#include "ems_client.h"
#include "ems_tunnel.h"

#define TSESS_TIMEOUT_FLOW	120000
#define TSESS_TIMEOUT_CONNECT	15000

static ems_void tsess_evt_cb(ems_session *sess, ems_int err, ems_int flg);
static ems_void tsess_timeout_cb(ems_session *sess, ems_timeout *to);

static ems_int tsess_start(tunnel_session *tsess, ems_session *sess, ems_uint flg);
static ems_int tsess_connect(tunnel_session *tsess, ems_session *sess, ems_uint flg);
static ems_int tsess_normal(tunnel_session *tsess, ems_session *sess, ems_uint flg);
static ems_int tsess_err  (tunnel_session *tsess, ems_session *sess, ems_uint flg);
static ems_int tsess_stopped(tunnel_session *tsess, ems_session *sess, ems_uint flg);

typedef ems_int (*tsess_evt_func)(tunnel_session *tsess, ems_session *sess, ems_uint flg);
static tsess_evt_func tsess_evt_handler[] = 
{
	[st_start]   = tsess_start,
	[st_connect] = tsess_connect,
	[st_normal]  = tsess_normal,
	[st_err]     = tsess_err,
	[st_stopped] = tsess_stopped
};

static ems_int tsess_to_connect(tunnel_session *tsess, ems_session *sess, ems_timeout *to);
static ems_int tsess_to_normal(tunnel_session *tsess, ems_session *sess, ems_timeout *to);

typedef ems_int (*tsess_timeout_func)(tunnel_session *tsess, ems_session *sess, ems_timeout *to);
static tsess_timeout_func tsess_timeout_handler[] = 
{
	[st_start]   = NULL,
	[st_connect] = tsess_to_connect,
	[st_normal]  = tsess_to_normal,
	[st_err]     = NULL,
	[st_stopped] = NULL
};

static ems_int tsess_evt_run(tunnel_session *tsess, ems_session *sess, ems_uint flg)
{
	ems_assert(tsess_evt_handler[tsess->st] != NULL);

	return tsess_evt_handler[tsess->st](tsess, sess, flg);
}

static ems_int tsess_start(tunnel_session *tsess, ems_session *sess, ems_uint flg)
{
	if (!tsess->sess) {
		tsess->sess = ems_session_new();
		if (!tsess->sess)
			return EMS_ERR;
	}

	sess = tsess->sess;
	ems_buffer_clear(&sess->buf_in);
	ems_buffer_clear(&sess->buf_out);
	sess_cbarg_set(sess, tsess);

	ems_sock_setaddr(&sess->sock, "127.0.0.1");
	ems_sock_setport(&sess->sock, 80);

	if (tunnel_session_connect(sess) != EMS_OK)
		return tsess_change_status(tsess, st_err);

	if (ems_flag_like(sess->flg, EMS_FLG_ONLINE))
		return tsess_change_status(tsess, st_normal);

	sess_event_set(sess, EMS_EVT_READ | EMS_EVT_WRITE, tsess_evt_cb);
	sess_timeout_set_sorted(sess, TSESS_TIMEOUT_CONNECT, tsess_timeout_cb);

	return tsess_change_status(tsess, st_connect);
}

static ems_int tsess_connect(tunnel_session *tsess, ems_session *sess, ems_uint flg)
{
	ems_int    err;
	socklen_t len;

	len = sizeof(err);
	err = 0;
	getsockopt(ems_sock_fd(&sess->sock), SOL_SOCKET, SO_ERROR, (ems_char *)&err, &len);

	if (err ) {
		errno = err;
		ems_l_info("[tsess]sess(%d) connect to %s:%d failed, %s",
				ems_sock_fd(&sess->sock),
				ems_sock_addr(&sess->sock),
				ems_sock_port(&sess->sock),
				ems_geterrmsg(err));

		return tsess_change_status(tsess, st_err);
	}

	ems_l_trace("[tsess]sess(%d) established with %s:%d",
			ems_sock_fd(&sess->sock),
			ems_sock_addr(&sess->sock),
			ems_sock_port(&sess->sock));

	ems_flag_set(sess->flg, EMS_FLG_ONLINE);
	sess_event_cancel(sess);

	ems_l_trace("[tsess %d] %s:%d, buf length: %d, left: %d, total: %d",
			ems_sock_fd(&sess->sock),
			ems_sock_addr(&sess->sock),
			ems_sock_port(&sess->sock),
			buf_len(&sess->buf), buf_left(&sess->buf), buf_size(&sess->buf));
	if (buf_len(&sess->buf) > 0)
		sess_event_set(sess, EMS_EVT_READ | EMS_EVT_WRITE, tsess_evt_cb);
	else
		sess_event_set(sess, EMS_EVT_READ, tsess_evt_cb);

	sess_timeout_set(sess, TSESS_TIMEOUT_FLOW, tsess_timeout_cb);

	return tsess_change_status(tsess, st_normal);
}

static ems_int tsess_process_rsp(tunnel_session *tsess, ems_session *sess)
{
	ems_int ret;
	if (!tsess->tunnel)
		return EMS_ERR;

	if (buf_len(&sess->buf_in) <= 0) 
		return EMS_CONTINUE;

	ret = tunnel_packmsg(tsess->tunnel,
			MSG_TYPE_WEB,
			tsess->id,
			buf_len(&sess->buf_in),
			(ems_uchar *)buf_rd(&sess->buf_in));

	ems_buffer_clear(&sess->buf_in);

	return ret;
}

static ems_int
tsess_recv_msg(tunnel_session *tsess, ems_session *sess, ems_uint flg)
{
	ems_int ret, again;

	again = EMS_YES;
recv_again:
	ret = sess_recv(sess, &sess->buf_in);

	ems_l_trace("[tsess (%d) %s:%d --> 0x%0x] ret: %d recved: %d bytes(buf left: %d, total: %d)",
			ems_sock_fd(&sess->sock), ems_sock_addr(&sess->sock), ems_sock_port(&sess->sock), 
			tsess->id, 
			ret, buf_len(&sess->buf_in),
			buf_left(&sess->buf_in), buf_size(&sess->buf_in));

	if (ret <= 0) {
		switch (ret) {
		case -EAGAIN:
			again = EMS_NO;
			break;
		default:
			tsess_process_rsp(tsess, sess);
			return EMS_ERR;
		}
	}

	ret = tsess_process_rsp(tsess, sess);

	switch (ret) {
	case EMS_SLOW_DOWN:
		ems_l_trace("[tsess] localhost too fast, slow down tunnel session(%d)", ems_sock_fd(&sess->sock));
		sess_event_cancel(sess);
		if (buf_len(&sess->buf) > 0)
			sess_event_set(sess, EMS_EVT_WRITE, tsess_evt_cb);
		again = EMS_NO;
		break;

	default:
		break;
	}

	if (again)
		goto recv_again;

	return EMS_OK;
}

extern ems_int tunnel_restore_speed(ems_tunnel *tunnel);

static ems_int tsess_send_msg(tunnel_session *tsess, ems_session *sess, ems_uint flg)
{
	ems_int ret;

	ems_assert(ems_flag_like(flg, EMS_EVT_WRITE) && buf_len(&sess->buf) > 0);

	ret = sess_send(sess, &sess->buf);

	ems_l_trace("[tsess %s:%d --> 0x%0x] sent: %d bytes, left %d, buf size: %d",
			ems_sock_addr(&sess->sock), ems_sock_port(&sess->sock), 
			tsess->id, 
			ret, buf_len(&sess->buf), buf_size(&sess->buf));
	if (ret <= 0) {
		switch(ret) {

		case -EAGAIN:
			break;

		default:
			return ret;
		}
	}


	ems_buffer_refresh(&sess->buf);

	if (ems_flag_like(sess->flg, FLG_TUNNEL_NEED_NOTIFY) &&
			buf_len(&sess->buf) <= EMS_BUFFER_2K) {
		ems_l_trace("[tsess %d] retore tunnel speed");
		tunnel_restore_speed(tsess->tunnel);
	}

	if (buf_len(&sess->buf) <= 0) {
		sess_event_set(sess, EMS_EVT_READ, tsess_evt_cb);
	}

	return EMS_OK;
}


static ems_int tsess_normal(tunnel_session *tsess, ems_session *sess, ems_uint flg)
{
	sess_timeout_set(sess, TSESS_TIMEOUT_FLOW, tsess_timeout_cb);

	if (ems_flag_like(flg, EMS_EVT_READ)) {
		if (tsess_recv_msg(tsess, sess, flg) != EMS_OK) 
			return tsess_change_status(tsess, st_err);
	}

	if (ems_flag_like(flg, EMS_EVT_WRITE)) {
		if (tsess_send_msg(tsess, sess, flg) != EMS_OK)
			return tsess_change_status(tsess, st_err);
	}

	return EMS_OK;
}

static ems_int tsess_err (tunnel_session *tsess, ems_session *sess, ems_uint flg)
{
	ems_assert(tsess && tsess->tunnel);
	if (tsess->tunnel)
		tunnel_packmsg(tsess->tunnel, MSG_WEB_DOWN, tsess->id, 0, NULL);

	return EMS_OK;
}

static ems_int tsess_stopped(tunnel_session *tsess, ems_session *sess, ems_uint flg)
{
	if (tsess->sess) {
		ems_session_shutdown_and_destroy(tsess->sess);
		tsess->sess = NULL;
	}

	ems_hash_remove(&tsess->hash);
	ems_queue_remove(&tsess->entry);
	tsess->tunnel = NULL;

	ems_free(tsess);

	return EMS_OK;
}

static ems_int tsess_to_connect(tunnel_session *tsess, ems_session *sess, ems_timeout *to)
{
	ems_l_info("[tsess %s:%d -->0x%x] connect timeout",
			ems_sock_addr(&sess->sock), ems_sock_port(&sess->sock),
			tsess->id);

	return tsess_change_status(tsess, st_err);
}

static ems_int tsess_to_normal(tunnel_session *tsess, ems_session *sess, ems_timeout *to)
{
	ems_l_info("[tsess %s:%d -->0x%x] timeout for stream flow.. ",
			ems_sock_addr(&sess->sock), ems_sock_port(&sess->sock),
			tsess->id);

	return tsess_change_status(tsess, st_err);
}

static ems_int 
tsess_timeout_run(tunnel_session *tsess, ems_session *sess, ems_timeout *to)
{
	ems_assert(tsess_timeout_handler[tsess->st] != NULL);

	if (tsess_timeout_handler[tsess->st])
		return tsess_timeout_handler[tsess->st](tsess, sess, to);

	return EMS_OK;
}

static ems_void tsess_evt_cb(ems_session *sess, ems_int err, ems_int flg)
{
	tunnel_session *tsess = (tunnel_session *)sess_cbarg(sess);

	ems_assert(tsess->st > st_min && tsess->st < st_max);

	if (err) {
		ems_l_trace("[tsess] evt err, sess: %d %s",
				ems_sock_fd(&sess->sock),
				ems_sock_addr(&sess->sock));
		tunnel_session_change_status(tsess, st_err);
		return;
	}

	tsess_evt_run(tsess, sess, flg);
}

static ems_void tsess_timeout_cb(ems_session *sess, ems_timeout *to)
{
	tunnel_session *tsess = (tunnel_session *)sess_cbarg(sess);

	ems_assert(tsess->st > st_min && tsess->st < st_max);

	tsess_timeout_run(tsess, sess, to);
}

tunnel_session *tunnel_session_new(ems_tunnel *tunnel, ems_uint id)
{
	tunnel_session *tsess = NULL;

	tsess = (tunnel_session *)ems_malloc(sizeof(tunnel_session));
	if (tsess) {
		memset(tsess, 0, sizeof(tunnel_session));

		ems_hash_fd_init(&tsess->hash);
		ems_hash_fd_set_key(&tsess->hash, ems_itoa((ems_int)id));
		tsess->id = id;
		tsess->tunnel = tunnel;
		tsess->st = st_stopped;
		tsess->sess = NULL;
	}

	return tsess;
}

tunnel_session *tunnel_session_find(ems_hash *hash, ems_uint id)
{
	ems_hash_fd    *h;
	tunnel_session *tsess = NULL;

	h = ems_hash_find(hash, ems_itoa(id));
	if (h) {
		tsess = ems_container_of(h, tunnel_session, hash);
	}

	return tsess;
}

ems_int tunnel_unpackmsg(ems_tunnel_pkg *pkg, tunnel_session *tsess)
{
	ems_int      ret;
	ems_session *sess = tsess->sess;
	ems_buffer  *buf  = NULL;
	ems_assert(pkg && tsess && tsess->sess);

	buf = &sess->buf_out;
	ret = EMS_OK;

	if (buf_left(buf) <= pkg->len) {
		if (buf_size(buf) > EMS_BUFFER_16k ) {
			ems_l_warn("[tsess(%d)--> 0x%x] buffer insufficient(len: %d: left: %d size:%d), drop",
				ems_sock_fd(&sess->sock), tsess->id, buf_len(buf), buf_left(buf), buf_size(buf));

			return EMS_ERR;
		}

		ems_buffer_increase(buf, pkg->len);
	}

	if (buf_len(buf) > EMS_BUFFER_2K) {
		ems_l_trace("[tsess %d] send too fast from tunnel, slow it down");
		ret = EMS_SLOW_DOWN;
		ems_flag_set(sess->flg, FLG_TUNNEL_NEED_NOTIFY);
	}

	ems_buffer_write(buf, pkg->val, pkg->len);
	sess_event_set(sess, EMS_EVT_WRITE, tsess_evt_cb);

	return ret;
}

ems_int tunnel_session_change_status(tunnel_session *tsess, ems_status st)
{
	ems_l_trace("[tsess] change  status: %s >> %s",
			ems_status_str(tsess->st), ems_status_str(st));

	tsess->st = st;

	switch(tsess->st) {
	case st_start:
	case st_stopped:
	case st_err:
		return tsess_evt_run(tsess, NULL, 0);

	default:
		break;
	}

	return EMS_OK;
}

ems_int tunnel_session_restore_speed(tunnel_session *tsess)
{
	ems_int      evt;
	ems_session *sess;
	ems_assert(tsess && tsess->sess);

	sess = tsess->sess;
	if (sess) {
		evt = EMS_EVT_READ;
		if (buf_len(&sess->buf) > 0)
			evt |= EMS_EVT_WRITE;

		ems_l_trace("[tsess] restore tunnel session(%d, evt: %d, buf_len: %d) speed", 
				ems_sock_fd(&sess->sock), evt, buf_len(&sess->buf));

		sess_event_set(sess, evt, tsess_evt_cb);
	}

	return EMS_OK;
}


