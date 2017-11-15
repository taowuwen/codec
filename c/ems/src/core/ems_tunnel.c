
#include "ems_core.h"
#include "ems_client.h"
#include "ems_tunnel.h"


static ems_void tunnel_evt_cb(ems_session *sess, ems_int err, ems_int flg);
static ems_void tunnel_timeout_cb(ems_session *sess, ems_timeout *to);
typedef ems_int (*tunnel_recv_cb)(ems_tunnel *tunnel, ems_session *sess, ems_tunnel_pkg *pkg);

static ems_int tunnel_start(ems_tunnel *tunnel, ems_session *sess, ems_uint flg);
static ems_int tunnel_connect(ems_tunnel *tunnel, ems_session *sess, ems_uint flg);
static ems_int tunnel_reg  (ems_tunnel *tunnel, ems_session *sess, ems_uint flg);
static ems_int tunnel_normal(ems_tunnel *tunnel, ems_session *sess, ems_uint flg);
static ems_int tunnel_hb   (ems_tunnel *tunnel, ems_session *sess, ems_uint flg);
static ems_int tunnel_stopped(ems_tunnel *tunnel, ems_session *sess, ems_uint flg);
static ems_int tunnel_err  (ems_tunnel *tunnel, ems_session *sess, ems_uint flg);

typedef ems_int (*tunnel_evt_func)(ems_tunnel *tunnel, ems_session *sess, ems_uint flg);
static tunnel_evt_func tunnel_evt_handler[] = 
{
	[st_start]   = tunnel_start,
	[st_connect] = tunnel_connect,
	[st_reg]     = tunnel_reg,
	[st_normal]  = tunnel_normal,
	[st_hb]      = tunnel_hb, 
	[st_err]     = tunnel_err,
	[st_stopped] = tunnel_stopped
};

static ems_int tunnel_to_connect(ems_tunnel *tunnel, ems_session *sess, ems_timeout *to);
static ems_int tunnel_to_reg(ems_tunnel *tunnel, ems_session *sess, ems_timeout *to);
static ems_int tunnel_to_normal(ems_tunnel *tunnel, ems_session *sess, ems_timeout *to);
static ems_int tunnel_to_hb(ems_tunnel *tunnel, ems_session *sess, ems_timeout *to);
static ems_int tunnel_to_err(ems_tunnel *tunnel, ems_session *sess, ems_timeout *to);

typedef ems_int (*tunnel_timeout_func)(ems_tunnel *tunnel, ems_session *sess, ems_timeout *to);
static tunnel_timeout_func tunnel_timeout_handler[] = 
{
	[st_start]   = NULL,
	[st_connect] = tunnel_to_connect,
	[st_reg]     = tunnel_to_reg,
	[st_normal]  = tunnel_to_normal,
	[st_hb]      = tunnel_to_hb, 
	[st_err]     = tunnel_to_err,
	[st_stopped] = NULL
};

#define EMS_256K	262144
#define EMS_512K	524288

extern ems_int tunnel_session_restore_speed(tunnel_session *tsess);

static ems_void tunnel_notify_all_tsess(ems_tunnel *tunnel)
{
	ems_queue      *p, *q;
	tunnel_session *tsess = NULL;

	ems_queue_foreach_safe(&tunnel->list_sess, p, q) {
		tsess = ems_container_of(p, tunnel_session, entry);
		tunnel_session_restore_speed(tsess);
	}
}

static ems_int tunnel_send_msg(ems_tunnel *tunnel, ems_session *sess, ems_uint flg)
{
	ems_int ret;

	ems_assert(ems_flag_like(flg, EMS_EVT_WRITE));

	ret = sess_send(sess, &sess->buf);
	if (ret <= 0) {
		switch(ret) {

		case -EAGAIN:
			break;

		default:
			return ret;
		}
	}

	ems_l_trace("[tunnel %d] *send out* buf(len: %d, left: %d, size: %d", 
		ems_sock_fd(&sess->sock), buf_len(&sess->buf), buf_left(&sess->buf), buf_size(&sess->buf));

	if (   ems_flag_like(tunnel->flg, FLG_TUNNEL_NEED_NOTIFY) 
	   && (buf_len(&sess->buf) < EMS_256K)) {
		ems_flag_unset(tunnel->flg, FLG_TUNNEL_NEED_NOTIFY);

		ems_buffer_refresh(&sess->buf);
		tunnel_notify_all_tsess(tunnel);
	}

	if (buf_len(&sess->buf) <= 0) {
		sess_event_set(sess, EMS_EVT_READ, tunnel_evt_cb);
		ems_buffer_clear(&sess->buf);
	}

	return EMS_OK;
}

static ems_int tunnel_process_rsp(ems_tunnel *tunnel, ems_session *sess, tunnel_recv_cb cb)
{
	ems_int         ret;
	ems_tunnel_pkg *pkg;

	if (buf_len(&sess->buf_in) <= SIZE_TUNNEL_HL) {
		return EMS_CONTINUE;
	}

	pkg = (ems_tunnel_pkg *)buf_rd(&sess->buf_in);

	if (ntohs(pkg->len) > EMS_BUFFER_2K) {
		ems_l_trace("[tunnel] invalid rsp header length, (%d > %d)", 
				ntohs(pkg->len), EMS_BUFFER_2K);
		ems_printhex(buf_rd(&sess->buf_in), SIZE_TUNNEL_HL);

		ems_buffer_clear(&sess->buf_in);
		return EMS_ERR;
	}

	if (buf_len(&sess->buf_in) < ntohs(pkg->len) + SIZE_TUNNEL_HL) {
		/*
		   maybe auto increate here, not now
		 */
		return EMS_CONTINUE;
	}

	pkg->flgty = ntohs(pkg->flgty);
	pkg->len   = ntohs(pkg->len);
	pkg->id    = ntohl(pkg->id);

	if (cb) 
		ret = cb(tunnel, sess, pkg);
	else
		ret = EMS_OK;
	ems_buffer_seek_rd(&sess->buf_in, pkg->len + SIZE_TUNNEL_HL, EMS_BUFFER_SEEK_CUR);
	ems_buffer_refresh(&sess->buf_in);

	return ret;
}

static ems_int
tunnel_recv_msg(ems_tunnel *tunnel, ems_session *sess, tunnel_recv_cb h)
{
	ems_int ret, again;

	again = EMS_YES;
recv_again:
	ret = sess_recv(sess, &sess->buf_in);
	if (ret <= 0) {
		switch (ret) {
		case -EAGAIN:
			again = EMS_NO;
			break;
		default:
			if (buf_len(&sess->buf_in) > 0) 
				tunnel_process_rsp(tunnel, sess, h);
			/* shutdown current sesson*/
			return EMS_ERR;
		}
	}

	do {
		ret = tunnel_process_rsp(tunnel, sess, h);

		switch (ret) {
		case EMS_BUFFER_INSUFFICIENT:
		case EMS_ERR:
			return EMS_ERR;

		case EMS_SLOW_DOWN:
			again = EMS_NO;
			ems_l_trace("[tunnel]upload too fast, tunnel session(%d)", ems_sock_fd(&sess->sock));
			sess_event_cancel(sess);
			if (buf_len(&sess->buf) > 0)
				sess_event_set(sess, EMS_EVT_WRITE, tunnel_evt_cb);

			break;

		case EMS_OK:
		case EMS_CONTINUE:
		default:
			break;
		}

	} while (ret != EMS_CONTINUE);

	if (again)
		goto recv_again;

	return EMS_OK;
}

ems_int tunnel_session_connect(ems_session *sess)
{
	ems_int    fd, ret;
	socklen_t  len;
	struct sockaddr_in addr;
	ems_sock   *sock = &sess->sock;

	ems_assert(sess);

	memset(&addr, 0, sizeof(addr));
	if (ems_gethostbyname(ems_sock_addr(sock), &addr) != OK) {
		ems_l_info("[tunnel]gethostbyename failed %s : %s", 
				ems_sock_addr(sock), ems_lasterrmsg());
		return EMS_ERR;
	}

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd <= 0)
		return EMS_ERR;

	ems_l_trace("[tunnel/session] sess(%d) try to connect to: %s(%s): %d...",
			fd, 
			ems_sock_addr(sock), 
			inet_ntoa(addr.sin_addr), 
			ems_sock_port(sock));

	addr.sin_family = AF_INET;
	addr.sin_port   = htons(ems_sock_port(sock));

	ems_setnonblocking(fd, YES);
	len = sizeof(struct sockaddr_in);
	ret = connect(fd, (struct sockaddr *)&addr, len);

	switch(ret) {
	case 0:
		ems_flag_set(sess->flg, EMS_FLG_ONLINE);
		break;

	case -1:
		ret = ems_lasterr();
		ems_flag_unset(sess->flg, EMS_FLG_ONLINE);

		if (ret != EINPROGRESS) {
			close(fd);
			ems_l_info("[tunnel] connect to: %s:%d: failed: %s",
					ems_sock_addr(sock), 
					ems_sock_port(sock), 
					ems_geterrmsg(ret));
			return EMS_ERR;

		}
	default:
		break;
	}

	ems_sock_setfd(sock, fd);
	return EMS_OK;
}


static ems_int tunnel_start(ems_tunnel *tunnel, ems_session *sess, ems_uint flg)
{
	if (!tunnel->sess) {
		tunnel->sess = ems_session_new();

		if (!tunnel->sess)
			return EMS_ERR;

		ems_buffer_increase(&tunnel->sess->buf_in,  EMS_BUFFER_2K);
	}

	sess = tunnel->sess;
	ems_buffer_clear(&sess->buf_in);
	ems_buffer_clear(&sess->buf_out);
	sess_cbarg_set(sess, tunnel);

	tunnel->id = random() & 0x0000ffff;
	ems_sock_setaddr(&sess->sock, str_text(&tunnel->addr));
	ems_sock_setport(&sess->sock, tunnel->port);

	if (tunnel_session_connect(sess) != EMS_OK)
		return tunnel_change_status(tunnel, st_err);

	if (ems_flag_like(sess->flg, EMS_FLG_ONLINE))
		return tunnel_change_status(tunnel, st_reg);

	sess_event_set(sess, EMS_EVT_READ|EMS_EVT_WRITE, tunnel_evt_cb);
	sess_timeout_set_sorted(sess, 30000, tunnel_timeout_cb);

	return tunnel_change_status(tunnel, st_connect);
}

static ems_int tunnel_connect(ems_tunnel *tunnel, ems_session *sess, ems_uint flg)
{
	ems_int    err;
	socklen_t len;

	len = sizeof(err);
	err = 0;
	getsockopt(ems_sock_fd(&sess->sock), SOL_SOCKET, SO_ERROR, (ems_char *)&err, &len);

	if (err ) {
		errno = err;
		ems_l_info("[tunnel]sess(%d) connect to %s:%d failed, %s",
				ems_sock_fd(&sess->sock),
				ems_sock_addr(&sess->sock),
				ems_sock_port(&sess->sock),
				ems_geterrmsg(err));

		return tunnel_change_status(tunnel, st_err);
	}

	ems_l_trace("[tunnel]sess(%d) established with %s:%d",
			ems_sock_fd(&sess->sock),
			ems_sock_addr(&sess->sock),
			ems_sock_port(&sess->sock));

	ems_flag_set(sess->flg, EMS_FLG_ONLINE);
	sess_event_set(sess, EMS_EVT_READ, tunnel_evt_cb);

	return tunnel_change_status(tunnel, st_reg);
}

static json_object *tunnel_parse_rsp(ems_tunnel_pkg *pkg)
{
	struct json_tokener *tok;
	struct json_object  *jobj;
	enum   json_tokener_error jerr = json_tokener_success;

	tok = json_tokener_new();
	if (!tok)
		return NULL;

	jobj = json_tokener_parse_ex(tok, pkg->val, pkg->len);

	json_tokener_free(tok);

	if (!(jobj && (jerr == json_tokener_success))) {
		ems_l_warn("[tunnel rsp] parse err: %s", json_tokener_error_desc(jerr));

		if (jobj) json_object_put(jobj);
		return NULL;
	}

	return jobj;
}

static ems_int 
tunnel_dispatch_to_tunnel(ems_tunnel *tunnel, ems_session *sess, ems_tunnel_pkg *pkg)
{
	json_object  *root;
	ems_int       err;

	ems_l_info("[tunnel %d] (ty: 0x%x, len: 0x%x, id: 0x%x, tunnel->id: 0x%x)",
		ems_sock_fd(&sess->sock), pkg->flgty, pkg->len, pkg->id, tunnel->id);
	if (pkg->id != tunnel->id)
		return EMS_OK;

	if (ems_flag_like(pkg->flgty, PH_RST))
		goto err_out;

	root = tunnel_parse_rsp(pkg);
	if (!root)
		goto err_out;

	ems_l_info("[tunnel] %s:%d, response: %s",
			ems_sock_addr(&sess->sock), 
			ems_sock_port(&sess->sock),
			json_object_to_json_string(root));

	/* for now, does not matter of register or heartbeat, check error code only*/
	ems_json_get_int_def(root, "error", err, 0);
	json_object_put(root);

	if (err != 0)
		goto err_out;

	if (tunnel->st != st_normal) {
		sess_timeout_set_sorted(sess, tunnel->hb * 1000, tunnel_timeout_cb);
		return tunnel_change_status(tunnel, st_normal);
	}

	return EMS_OK;

err_out:
	return tunnel_change_status(tunnel, st_err);
}

/*
   1. find web session
   	no? create session,
		change session status
   2. unpack msgs info session	
 */

static ems_int 
tunnel_dispatch_to_web(ems_tunnel *tunnel, ems_session *sess, ems_tunnel_pkg *pkg)
{
	tunnel_session *tsess = NULL;

	tsess = tunnel_session_find(&tunnel->hash_sess, pkg->id);

	if (ems_flag_like(pkg->flgty, PH_RST)) {
		ems_l_info("[tsess RST %d] (ty: 0x%x, len: 0x%x, id: 0x%x, tunnel->id: 0x%x)",
			tunnel->st, pkg->flgty, pkg->len, pkg->id, tunnel->id);

		if (tsess)
			return tsess_change_status(tsess, st_stopped);
	}

	if (!tsess) {
		tsess = tunnel_session_new(tunnel, pkg->id);
		if (!tsess) 
			goto web_err;

		ems_hash_insert(&tunnel->hash_sess, &tsess->hash);
		ems_queue_insert_tail(&tunnel->list_sess, &tsess->entry);

		if (EMS_OK != tsess_change_status(tsess, st_start)) {
			tsess_change_status(tsess, st_stopped);
			tsess = NULL;
			goto web_err;
		}
	}

	return tunnel_unpackmsg(pkg, tsess);

web_err:
	tunnel_packmsg(tunnel, MSG_WEB_DOWN, pkg->id, 0, NULL);
	return EMS_OK;
}

static ems_int tunnel_dispatcher(ems_tunnel *tunnel, ems_session *sess, ems_tunnel_pkg *pkg)
{
	ems_l_trace("[tunnel %d] (ty: 0x%x, len: 0x%x, id: 0x%x, tunnel->id: 0x%x)",
			ems_sock_fd(&sess->sock), pkg->flgty, pkg->len, pkg->id, tunnel->id);

	switch(pkg->flgty & 0x0fff) {
	case MSG_TYPE_WEB:
		return tunnel_dispatch_to_web(tunnel, sess, pkg);

	case MSG_TYPE_TUNNEL:
		return tunnel_dispatch_to_tunnel(tunnel, sess, pkg);

	default:
		break;
	}

	return EMS_OK;
}

static ems_int tunnel_reg(ems_tunnel *tunnel, ems_session *sess, ems_uint flg)
{
	if (ems_flag_like(flg, EMS_EVT_READ)) {
#define tunnel_reg_cb	tunnel_dispatcher
		if (tunnel_recv_msg(tunnel, sess, tunnel_reg_cb) != EMS_OK) 
			return tunnel_change_status(tunnel, st_err);
	}

	if (ems_flag_like(flg, EMS_EVT_WRITE)) {
		if (tunnel_send_msg(tunnel, sess, flg) != EMS_OK)
			return tunnel_change_status(tunnel, st_err);
	}

	return EMS_OK;
}

static ems_int tunnel_normal(ems_tunnel *tunnel, ems_session *sess, ems_uint flg)
{
	if (ems_flag_like(flg, EMS_EVT_READ)) {
#define tunnel_normal_cb	tunnel_dispatcher
		if (tunnel_recv_msg(tunnel, sess, tunnel_normal_cb) != EMS_OK) 
			return tunnel_change_status(tunnel, st_err);
	}

	if (ems_flag_like(flg, EMS_EVT_WRITE)) {
		if (tunnel_send_msg(tunnel, sess, flg) != EMS_OK)
			return tunnel_change_status(tunnel, st_err);
	}

	return EMS_OK;
}

static ems_int tunnel_hb(ems_tunnel *tunnel, ems_session *sess, ems_uint flg)
{
	if (ems_flag_like(flg, EMS_EVT_READ)) {
#define tunnel_hb_cb	tunnel_dispatcher
		if (tunnel_recv_msg(tunnel, sess, tunnel_hb_cb) != EMS_OK) 
			return tunnel_change_status(tunnel, st_err);
	}

	if (ems_flag_like(flg, EMS_EVT_WRITE)) {
		if (tunnel_send_msg(tunnel, sess, flg) != EMS_OK)
			return tunnel_change_status(tunnel, st_err);
	}

	return EMS_OK;
}

#ifdef DEBUG

ems_int tunnel_session_hash_cb(ems_hash_fd *fd, ems_void *arg)
{
	ems_assert(0 && "never show up this line");
	ems_l_error("[tunnel]never show up this line");
	return EMS_ERR;
}


ems_int tunnel_session_empty(ems_tunnel *tunnel)
{
	if (ems_hash_walk(&tunnel->hash_sess, tunnel_session_hash_cb, NULL) != EMS_OK)
		return EMS_NO;

	if (!ems_queue_empty(&tunnel->list_sess))
		return EMS_NO;

	return EMS_YES;
}

#endif

static ems_int tunnel_stop_all(ems_tunnel *tunnel, ems_session *sess)
{
	if (sess) {
		/* cancel all events */
		sess_timeout_cancel(sess);
		sess_event_cancel(sess);
	}

	{
		/* destroy sessions */
		ems_queue      *p, *q;
		tunnel_session *tsess = NULL;

		ems_queue_foreach_safe(&tunnel->list_sess, p, q) {
			tsess = ems_container_of(p, tunnel_session, entry);
			tsess_change_status(tsess, st_stopped);
		}
	}

	ems_assert(tunnel_session_empty(tunnel));

	/* clear hash */
//	ems_hash_clean(&tunnel->hash_sess);

	return EMS_OK;
}

static ems_int tunnel_stopped(ems_tunnel *tunnel, ems_session *sess, ems_uint flg)
{
	/* clear all and stop all */
	tunnel_stop_all(tunnel, tunnel->sess);

	if (tunnel->sess) {
		ems_session_shutdown_and_destroy(tunnel->sess);
		tunnel->sess = NULL;
	}

	return EMS_OK;
}

static ems_int tunnel_err(ems_tunnel *tunnel, ems_session *sess, ems_uint flg)
{
	ems_assert(tunnel && tunnel->sess);

	sess = tunnel->sess;

	/* clear and stop all */
	tunnel_stop_all(tunnel, sess);

	ems_assert(sess);
	if (sess) {
		ems_l_trace("[tunnel %d] %s:%d shutdown", 
				ems_sock_fd(&sess->sock), 
				ems_sock_addr(&sess->sock), 
				ems_sock_port(&sess->sock));
		ems_sock_close(&sess->sock);
		sess_timeout_set_sorted(sess, 5000, tunnel_timeout_cb);

		if (buf_size(&sess->buf) >=  EMS_BUFFER_16k * 4) {
			ems_buffer_uninit(&sess->buf);
			ems_buffer_init(&sess->buf, EMS_BUFFER_4k);
		}
	}

	return EMS_OK;
}

static ems_int tunnel_evt_run(ems_tunnel *tunnel, ems_session *sess, ems_uint flg)
{
	ems_assert(tunnel_evt_handler[tunnel->st] != NULL);

	return tunnel_evt_handler[tunnel->st](tunnel, sess, flg);
}

static ems_int tunnel_to_connect(ems_tunnel *tunnel, ems_session *sess, ems_timeout *to)
{
	ems_l_info("[tunnel], connect to %s:%d timeout", 
			ems_sock_addr(&sess->sock), ems_sock_port(&sess->sock));

	return tunnel_change_status(tunnel, st_err);
}

static ems_int tunnel_to_reg(ems_tunnel *tunnel, ems_session *sess, ems_timeout *to)
{
	ems_l_info("[tunnel], register to %s:%d timeout", 
			ems_sock_addr(&sess->sock), ems_sock_port(&sess->sock));

	return tunnel_change_status(tunnel, st_err);
}

static ems_int tunnel_to_normal(ems_tunnel *tunnel, ems_session *sess, ems_timeout *to)
{
	return tunnel_change_status(tunnel, st_hb);
}

static ems_int tunnel_to_hb(ems_tunnel *tunnel, ems_session *sess, ems_timeout *to)
{
	ems_l_info("[tunnel], heartbeat to %s:%d timeout", 
			ems_sock_addr(&sess->sock), ems_sock_port(&sess->sock));

	return tunnel_change_status(tunnel, st_err);
}

static ems_int tunnel_to_err(ems_tunnel *tunnel, ems_session *sess, ems_timeout *to)
{
	return tunnel_change_status(tunnel, st_start);
}

static ems_int 
tunnel_timeout_run(ems_tunnel *tunnel, ems_session *sess, ems_timeout *to)
{
	ems_assert(tunnel_timeout_handler[tunnel->st] != NULL);

	if (tunnel_timeout_handler[tunnel->st])
		return tunnel_timeout_handler[tunnel->st](tunnel, sess, to);

	return EMS_OK;
}

static ems_void tunnel_evt_cb(ems_session *sess, ems_int err, ems_int flg)
{
	ems_tunnel *tunnel = (ems_tunnel *)sess_cbarg(sess);

	ems_assert(tunnel->st > st_min && tunnel->st < st_max);

	if (err) {
		ems_l_info("[tunnel] evt err, sess: %d %s",
				ems_sock_fd(&sess->sock),
				ems_sock_addr(&sess->sock));
		tunnel_change_status(tunnel, st_err);
		return;
	}

	tunnel_evt_run(tunnel, sess, flg);
}

static ems_void tunnel_timeout_cb(ems_session *sess, ems_timeout *to)
{
	ems_tunnel *tunnel = (ems_tunnel *)sess_cbarg(sess);

	ems_assert(tunnel->st > st_min && tunnel->st < st_max);

	tunnel_timeout_run(tunnel, sess, to);
}

static ems_int tunnel_status_into_reg(ems_tunnel *tunnel)
{
	ems_cchar    *ctx;
	json_object *jobj;
	ems_session  *sess = tunnel->sess;

	ems_assert(tunnel && sess);

	jobj = json_object_new_object();
	json_object_object_add(jobj, "method", json_object_new_string("register"));
	json_object_object_add(jobj, "sn",     json_object_new_string(core_sn()));

	ctx = json_object_to_json_string(jobj);

	ems_l_info("[tunnel] %s:%d --> %s",
		ems_sock_addr(&sess->sock), ems_sock_port(&sess->sock), ctx);

	tunnel_packmsg(tunnel, MSG_TYPE_TUNNEL, tunnel->id, (ems_short)ems_strlen(ctx), (ems_uchar *)ctx);

	json_object_put(jobj);
	sess_timeout_set_sorted(tunnel->sess, 30000, tunnel_timeout_cb);

	return EMS_OK;
}


static ems_int tunnel_status_into_hb(ems_tunnel *tunnel)
{
	ems_cchar    *ctx;
	json_object *jobj;
	ems_session  *sess = tunnel->sess;

	ems_assert(tunnel && sess);

	jobj = json_object_new_object();
	json_object_object_add(jobj, "method", json_object_new_string("heartbeat"));

	ctx = json_object_to_json_string(jobj);

	ems_l_info("[tunnel] %s:%d --> %s",
		ems_sock_addr(&sess->sock), ems_sock_port(&sess->sock), ctx);

	tunnel_packmsg(tunnel, MSG_TYPE_TUNNEL, tunnel->id, (ems_short)ems_strlen(ctx), (ems_uchar *)ctx);

	json_object_put(jobj);

	sess_timeout_set_sorted(sess, 30000, tunnel_timeout_cb);

	return EMS_OK;
}

ems_int tunnel_change_status(ems_tunnel *tunnel, ems_status st)
{
	ems_l_trace("[tunnel] change  status: %s >> %s",
			ems_status_str(tunnel->st), ems_status_str(st));

	tunnel->st = st;

	switch(tunnel->st) {
	case st_start:
	case st_stopped:
	case st_err:
		return tunnel_evt_run(tunnel, NULL, 0);

	case st_reg:
		return tunnel_status_into_reg(tunnel);

	case st_hb:
		return tunnel_status_into_hb(tunnel);

	default:
		break;
	}
	return EMS_OK;
}

/*
   pkg size <= 512k ---> auto inc
 */
ems_int tunnel_packmsg(ems_tunnel *tunnel, ems_short ty, ems_uint id, ems_short len, ems_uchar *val)
{
	ems_int         ret;
	ems_session    *sess = tunnel->sess;
	ems_tunnel_pkg *pkg = NULL;

	ems_assert(len <= EMS_BUFFER_2K);

	ems_l_trace("[tunnel (%d) %s:%d]packmsg: <0x%04x, %d, 0x%x> buf(len: %d, left: %d, size: %d)", 
			ems_sock_fd(&sess->sock), ems_sock_addr(&sess->sock), ems_sock_port(&sess->sock), 
			ty, len, id,
			buf_len(&sess->buf),
			buf_left(&sess->buf),
			buf_size(&sess->buf));

	if (buf_left(&sess->buf) <= SIZE_TUNNEL_HL + len) {
		if (buf_size(&sess->buf) >= EMS_512K)
			return EMS_ERR;

		ems_buffer_increase(&sess->buf, EMS_BUFFER_2K);
	}

	ret = EMS_OK;

	if (buf_len(&sess->buf) > EMS_256K) {
		ems_flag_set(tunnel->flg, FLG_TUNNEL_NEED_NOTIFY);

		ems_l_trace("[tunnel] localhost too fast  buf(len: %d, size: %d)",
				buf_len(&sess->buf), buf_size(&sess->buf));
		ret = EMS_SLOW_DOWN;
	}

	pkg = (ems_tunnel_pkg *)buf_wr(&sess->buf);
	pkg->flgty = htons(ty);
	pkg->id    = htonl(id);

	if (len > 0 && val) {
		pkg->len   = htons(len);
		memcpy(pkg->val, val, len);
	} else
		pkg->len   = htons(0);

	ems_buffer_seek_wr(&sess->buf, SIZE_TUNNEL_HL + len, EMS_BUFFER_SEEK_CUR);
	sess_event_set(sess, EMS_EVT_READ|EMS_EVT_WRITE, tunnel_evt_cb);

	return ret;
}

ems_int tunnel_restore_speed(ems_tunnel *tunnel)
{
	ems_int      evt;
	ems_session *sess;
	ems_assert(tunnel && tunnel->sess);

	sess = tunnel->sess;
	if (sess) {
		evt = EMS_EVT_READ;
		if (buf_len(&sess->buf) > 0)
			evt |= EMS_EVT_WRITE;

		ems_l_trace("[tunnel] restore tunnel session(%d, evt: %d, buf_len: %d) spped", 
				ems_sock_fd(&sess->sock), evt, buf_len(&sess->buf));

		sess_event_set(sess, evt, tunnel_evt_cb);
	}

	return EMS_OK;
}
