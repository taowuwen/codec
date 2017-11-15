
#include "audit.h"
#include "ems_core.h"
#include "ems_client.h"
#include "class.h"
#include "ctrl.h"
#include "ctrl_msg.h"



#define EMS_CMD_TIMEOUT	3000

static ems_int ctrl_evt_run(audit_ctrl *ctrl, ems_session *sess, ems_uint flg);

static ems_int ctrl_start(audit_ctrl *ctrl, ems_session *sess, ems_uint flg);
static ems_int ctrl_init(audit_ctrl *ctrl, ems_session *sess, ems_uint flg);
static ems_int ctrl_stop(audit_ctrl *ctrl, ems_session *sess, ems_uint flg);
static ems_int ctrl_normal(audit_ctrl *ctrl, ems_session *sess, ems_uint flg);

typedef ems_int (*ctrl_evt_func)(audit_ctrl *ctrl, ems_session *sess, ems_uint flg);
static ctrl_evt_func ctrl_handler[] = 
{
	[st_start]   = ctrl_start,
	[st_init]    = ctrl_init,
	[st_normal]  = ctrl_normal,
	[st_stop]    = ctrl_stop
};

static ems_void evt_cmd_cb    (ems_session *sess, ems_int st, ems_int flg);

static ems_int 
ctrl_cmd_handle(audit_ctrl *ctrl, ems_session *sess, json_object *req, ems_uint msgid)
{
	ems_l_trace("<< \033[01;33m req: %s \033[00m>>",
			req?json_object_to_json_string(req): "no arg");

	switch(msgid & 0x0000ffff) {

	/* start of 16wifi */
	case MSG_E6WIFI_FTP:
		return msg_e6wifi_ftp(ctrl, sess, req);

	case MSG_E6WIFI_CITYCODE:
		return msg_e6wifi_citycode(ctrl, sess, req);

	case MSG_USERINFO:
		return msg_userinfo(ctrl, sess, req);

	case MSG_SETUSER:
		return msg_setuser(ctrl, sess, req);

	case MSG_DELUSER:
		return msg_deluser(ctrl, sess, req);

	case MSG_MODULE_CTRL:
		return msg_mod_ctrl(ctrl, sess, req);

	case MSG_MODULE_INFO:
		return msg_mod_info(ctrl, sess, req);

	case MSG_MODULE_SET:
		return msg_mod_set(ctrl, sess, req);

	case MSG_MODULE_GET:
		return msg_mod_get(ctrl, sess, req);

	/* end of 16wifi */

	default:
		break;
	}

	return EMS_ERR;
}

static ems_int
ctrl_cmd_process_one(audit_ctrl *ctrl, ems_session *sess, ems_request *req)
{
	ems_int      rtn;
	json_object *root = NULL;

	ems_assert(req && ctrl && sess);
	ems_assert(req->len >= SIZE_REQUEST);
	ems_assert(buf_len(&sess->buf_in) >= req->len);

	if (req->len >= (SIZE_REQUEST + INTSIZE))
	{
		ems_int     len;
		ems_char    *p, ch;

		p = (ems_char *)(buf_rd(&sess->buf_in) + SIZE_REQUEST);
		getword(p, len);

		if (len > req->len - SIZE_REQUEST - INTSIZE)
			return EMS_ERR;

		ch = p[len]; // backup
		p[len] = '\0';
		root = ems_json_tokener_parse(p);
		p[len] = ch; // restore
	}

	rtn = ctrl_cmd_handle(ctrl, sess, root, req->tag.val);

	if (root)
		json_object_put(root);

	core_pack_rsp(sess,    req->tag.val,    rtn);
	sess_event_set(sess,   EMS_EVT_WRITE,   evt_cmd_cb);

	ems_buffer_seek_rd(&sess->buf_in, req->len, EMS_BUFFER_SEEK_CUR);
	ems_buffer_refresh(&sess->buf_in);

	return EMS_OK;
}

static ems_int
ctrl_cmd_process(audit_ctrl *ctrl, ems_session *sess)
{
	ems_request  req;

	if (buf_len(&sess->buf_in) < SIZE_REQUEST) 
		return EMS_CONTINUE;

	ems_buffer_prefetch(&sess->buf_in, (ems_char *)req.val, SIZE_REQUEST);

	req.tag.val = ntohl(req.tag.val);
	req.len     = ntohl(req.len);

	ems_l_trace("[ctrl]tag: 0x%.8X, msgid: 0x%.2X, len: 0x%.4X",
					req.tag.val, req.tag.val & 0x0000ffff, req.len);

	if (req.len >= buf_size(&sess->buf_in))
		return EMS_BUFFER_INSUFFICIENT;

	if (buf_len(&sess->buf_in) < req.len)
		return EMS_CONTINUE;

	return ctrl_cmd_process_one(ctrl, sess, &req);
}


static ems_int
ctrl_cmd_read(audit_ctrl *ctrl, ems_session *sess, ems_int flg)
{
	ems_int ret, again;

	ems_assert(ems_flag_like(flg, EMS_EVT_READ));

	again = EMS_YES;
recv_again:
	ems_buffer_refresh(&sess->buf_in);
	ret = sess_recv(sess, &sess->buf_in);

	if (ret <= 0) {
		switch (ret) {

		case -EAGAIN:
			again = EMS_NO;
			break;

		default:
			return EMS_ERR;
		}
	}

	switch (ctrl_cmd_process(ctrl, sess)) {
		
	case EMS_CONTINUE:
	case EMS_OK:
		break;

	case EMS_ERR:
	default:
		return EMS_ERR;
		break;
	}

	if (again)
		goto recv_again;

	return EMS_OK;
}

static ems_int
ctrl_cmd_write(audit_ctrl *ctrl, ems_session *sess, ems_int flg)
{
	ems_int ret;

	ems_assert(ems_flag_like(flg, EMS_EVT_WRITE));

	ret = sess_send(sess, &sess->buf);

	if (ret <= 0) {
		switch(ret) {
		case -EAGAIN:
			break;

		default:
			return EMS_ERR;
		}
	}

	if (buf_len(&sess->buf) <= 0) {
		sess_event_set(sess,   EMS_EVT_READ,    evt_cmd_cb);
		ems_buffer_refresh(&sess->buf);
	}

	return EMS_OK;
}


static ems_void evt_cmd_cb(ems_session *sess, ems_int err, ems_int flg)
{
	audit_ctrl *ctrl = (audit_ctrl *)sess_cbarg(sess);

	ems_l_trace("sess(%d), err? %s, flg: %x",
			ems_sock_fd(&sess->sock),
			err?"yes":"no",
			flg);
	if (err)
		goto err_out;

	do {
		if (ems_flag_like(flg, EMS_EVT_READ) && 
		    ctrl_cmd_read(ctrl, sess, flg)) 
		{
			goto err_out;
		}

		if (ems_flag_like(flg, EMS_EVT_WRITE) &&
		    ctrl_cmd_write(ctrl, sess, flg))
		{
			goto err_out;
		}
	} while (0);

	return;
err_out:
	ems_queue_remove(&sess->entry);
	ems_session_shutdown_and_destroy(sess);
}

#if 0
static ems_void timeout_cmd_cb(ems_session *sess, ems_timeout *to)
{
	ems_l_trace("[ctrl]timeout for cmd sess(%d): %s:%d",
			ems_sock_fd(&sess->sock),
			ems_sock_addr(&sess->sock),
			ems_sock_port(&sess->sock));

	ems_queue_remove(&sess->entry);
	ems_session_shutdown_and_destroy(sess);
}
#endif

static ems_int 
ctrl_sess_in(audit_ctrl *ctrl, ems_int sockfd, ems_cchar *addr, ems_int port)
{
	ems_session  *sess = NULL;

	sess = ems_session_new();
	if (!sess)
		return EMS_ERR;

	ems_sock_setaddr(&sess->sock, addr);
	ems_sock_setport(&sess->sock, port);
	ems_sock_setfd(&sess->sock, sockfd);

	ems_queue_insert_tail(&ctrl->cmd, &sess->entry);

	sess_cbarg_set(sess, ctrl);
	sess_event_set(sess, EMS_EVT_READ, evt_cmd_cb);

	return EMS_OK;
}

static ems_int 
ctrl_accept_next(audit_ctrl *ctrl, ems_int sock)
{
	ems_int       ret;
	socklen_t     len;
	struct sockaddr_in addr;

	len = sizeof(addr);
	memset(&addr, 0, sizeof(addr));

	ret = accept(sock, (struct sockaddr *)&addr, &len);

	if (ret < 0) {
		switch(ems_lasterr()) {
		
		case EINTR:
		case ECONNABORTED:
			return EMS_YES;

		case EAGAIN:
			return EMS_NO;

		default:
			ems_l_trace("[ctrl]accept error: %s", ems_lasterrmsg());
			return NO;
		}
	}

	ems_l_trace("[ctrl] NEW connection (%d) from: %s:%d in", 
				ret, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
	fcntl(sock, F_SETFD, fcntl(sock, F_GETFD) | FD_CLOEXEC);
	{
		struct linger lg;

		lg.l_onoff  = 1;
		lg.l_linger = 2;
		setsockopt(ret, SOL_SOCKET, SO_LINGER, (ems_cchar *)&lg, sizeof(lg));
	}

	if (ctrl_sess_in(ctrl, ret, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port)))
	{
		close(ret);
	}

	return EMS_YES;
}


static ems_void evt_ctrl_cb(ems_session *sess, ems_int err, ems_int flg)
{
	audit_ctrl  *ctrl = (audit_ctrl *)sess_cbarg(sess);

	ems_assert(ctrl);
	if (err) {
		ems_assert(0 && "should never be here");

		ctrl->sess = NULL;
		ems_session_shutdown_and_destroy(sess);

		/* now just retry */
		ctrl_change_status(ctrl, st_init);
		return;
	}

	ctrl_evt_run(ctrl, sess, flg);
}


static ems_int ctrl_start(audit_ctrl *ctrl, ems_session *sess, ems_uint flg)
{
	if (audit_sendmsg(0, mod_net, A_AUDIT_START, NULL) != EMS_OK) {
		ctrl_change_status(ctrl, st_stop);
		return EMS_ERR;
	}

	if (audit_sendmsg(0, mod_out, A_AUDIT_START, NULL) != EMS_OK) {
		ctrl_change_status(ctrl, st_stop);
		return EMS_ERR;
	}

	return ctrl_change_status(ctrl, st_init);
}

static ems_int ctrl_init(audit_ctrl *ctrl, ems_session *sess, ems_uint flg)
{
	ems_cchar *val = NULL;

	if (!ctrl->sess) {
		ctrl->sess = ems_session_new();
		if (!ctrl->sess)
			return ctrl_change_status(ctrl, st_stop);

		val = cfg_get(emscfg(), CFG_ctrl_bind_addr);
		if (val) 
			ems_sock_setaddr(&ctrl->sess->sock, val);
		else
			ems_sock_setaddr(&ctrl->sess->sock, AUDIT_ADDR);

		val = cfg_get(emscfg(), CFG_ctrl_bind_port);
		if (val)
			ems_sock_setport(&ctrl->sess->sock, ems_atoi(val));
		else
			ems_sock_setport(&ctrl->sess->sock, AUDIT_PORT);
	}

	sess = ctrl->sess;

	if (ems_sock_be_server(&sess->sock) != EMS_OK) {
		ctrl_change_status(ctrl, st_stop);
		return EMS_ERR;
	}

	sess_cbarg_set(sess, ctrl);
	sess_event_set(sess, EMS_EVT_READ, evt_ctrl_cb);

	return ctrl_change_status(ctrl, st_normal);
}

static ems_int ctrl_stop(audit_ctrl *ctrl, ems_session *sess, ems_uint flg)
{
	ems_assert(ctrl);

	audit_sendmsg(0, mod_net, A_AUDIT_STOP, NULL);
	audit_sendmsg(0, mod_out, A_AUDIT_STOP, NULL);
	if (ctrl->sess) {
		ems_session_shutdown_and_destroy(ctrl->sess);
		ctrl->sess = NULL;
	}

	ems_queue_clear(&ctrl->cmd, ems_session, entry, ems_session_shutdown_and_destroy);

	return EMS_OK;
}

static ems_int ctrl_normal(audit_ctrl *ctrl, ems_session *sess, ems_uint flg)
{
	ems_int    next;

	ems_assert(ems_flag_like(flg, EMS_EVT_READ));

	if (ems_flag_unlike(flg, EMS_EVT_READ))
		return EMS_OK;

	do {
		next = EMS_NO;
		next = ctrl_accept_next(ctrl, ems_sock_fd(&sess->sock));
	} while (next);

	return EMS_OK;
}

static ems_int 
ctrl_evt_run(audit_ctrl *ctrl, ems_session *sess, ems_uint flg)
{
	ems_assert(ctrl->st >= st_start && ctrl->st <= st_stop);

	if (ctrl_handler[ctrl->st])
		return ctrl_handler[ctrl->st](ctrl, sess, flg);

	return EMS_OK;
}

ems_int ctrl_change_status(audit_ctrl *ctrl, audit_status st)
{
	ems_l_trace("[ctrl] change status: %s ===> %s", 
			audit_status_str(ctrl->st),
			audit_status_str(st));

	ctrl->st = st;

	switch(st) {
	case st_start:
	case st_init:
	case st_stop:
		return ctrl_evt_run(ctrl, NULL, 0);
		break;

	default:
		break;
	}

	return EMS_OK;
}
