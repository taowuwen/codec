
#include "ems_core.h"
#include "ems_client.h"
#include "app.h"
#include "app_nic.h"
#include "ems_cmd.h"
#include "ems_radius.h"
#include "ems_ctrl.h"


#define EMS_CMD_TIMEOUT	3000

static ems_int ctrl_start(ems_ctrl *ctrl, ems_session *sess, ems_uint flg);
static ems_int ctrl_stopped(ems_ctrl *ctrl, ems_session *sess, ems_uint flg);
static ems_int ctrl_normal(ems_ctrl *ctrl, ems_session *sess, ems_uint flg);
static ems_int ctrl_evt_run(ems_ctrl *ctrl, ems_session *sess, ems_uint flg);

typedef ems_int (*ctrl_evt_func)(ems_ctrl *ctrl, ems_session *sess, ems_uint flg);
static ctrl_evt_func ctrl_handler[] = 
{
	[st_start]   = ctrl_start,
	[st_stopped] = ctrl_stopped,
	[st_normal]  = ctrl_normal,
	[st_max]     = NULL
};

static ems_void evt_cmd_cb    (ems_session *sess, ems_int st, ems_int flg);
static ems_void timeout_cmd_cb(ems_session *sess, ems_timeout *to);

extern ems_int ems_cmd_c     (ems_core *core, ems_session *sess, json_object *req);
extern ems_int ems_cmd_ctrl  (ems_core *core, ems_session *sess, json_object *req);
extern ems_int ems_cmd_status(ems_core *core, ems_session *sess, json_object *req);
extern ems_int ems_cmd_bwlist(ems_core *core, ems_session *sess, json_object *req);

extern ems_int ems_cmd_fw(ems_core *core, ems_session *sess, json_object *req);
extern ems_int ems_cmd_user(ems_core *core, ems_session *sess, json_object *req);

extern ems_int ems_cmd_network(ems_core *core, ems_session *sess, json_object *req);
extern ems_int ems_cmd_config(ems_core *core, ems_session *sess, json_object *req);
#ifdef EMS_LOGGER_FILE
extern ems_int ems_cmd_log(ems_core *core, ems_session *sess, json_object *req);
#endif
extern ems_int ems_cmd_lan(ems_core *core, ems_session *sess, json_object *req);

static ems_int 
ctrl_cmd_handle(ems_ctrl *ctrl, ems_session *sess, json_object *req, ems_uint msgid)
{
	ems_l_trace("[ctrl]<< \033[01;33m req: %s \033[00m>>",
			req?json_object_to_json_string(req): "no arg");

	switch(msgid & 0x0000ffff) {

	case CMD_EMS_C:
		return ems_cmd_c(emscorer(), sess, req);

	case CMD_EMS_STATUS:
		return ems_cmd_status(emscorer(), sess, req);

	case CMD_EMS_BWLIST:
		return ems_cmd_bwlist(emscorer(), sess, req);

	case CMD_EMS_FW:
		return ems_cmd_fw(emscorer(), sess, req);

	case CMD_EMS_USER:
		return ems_cmd_user(emscorer(), sess, req);

	case CMD_EMS_NETWORK:
		return ems_cmd_network(emscorer(), sess, req);

	case CMD_EMS_CONFIG:
		return ems_cmd_config(emscorer(), sess, req);

	case CMD_EMS_LAN:
		return ems_cmd_lan(emscorer(), sess, req);

#ifdef EMS_LOGGER_FILE
	case CMD_EMS_LOG:
		return ems_cmd_log(emscorer(), sess, req);
#endif

	default:
		break;
	}

	return EMS_ERR;
}


static ems_int
ctrl_cmd_process_one(ems_ctrl *ctrl, ems_session *sess, ems_request *req)
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

		if (len > req->len)
			return EMS_ERR;

		ch = p[len]; // backup
		p[len] = '\0';
		root = ems_json_tokener_parse(p);
		p[len] = ch; // restore
	}

	rtn = ctrl_cmd_handle(ctrl, sess, root, req->tag.msg);

	if (root)
		json_object_put(root);

	core_pack_rsp(sess,    req->tag.val,    rtn);
	sess_event_set(sess,   EMS_EVT_WRITE,   evt_cmd_cb);
	sess_timeout_set_sorted(sess, EMS_CMD_TIMEOUT, timeout_cmd_cb);

	ems_buffer_seek_rd(&sess->buf_in, req->len, EMS_BUFFER_SEEK_CUR);
	ems_buffer_refresh(&sess->buf_in);

	return EMS_OK;
}

#include "ems_http.h"
extern ems_int http_web_redirect_to_mgmt(ems_http *http, ems_session *sess);

static ems_int
ctrl_cmd_process(ems_ctrl *ctrl, ems_session *sess)
{
	ems_request  req;

	if (ems_flag_like(sess->flg, FLG_SESSION_IS_WEB))
	{
		ems_buffer_clear(&sess->buf_in);
		return EMS_CONTINUE;
	}

	if (http_msg_is_web(buf_rd(&sess->buf_in))) {
		ems_assert(ems_flag_unlike(emscorer()->flg, FLG_NETWORK_READY));
		ems_flag_set(sess->flg, FLG_SESSION_IS_WEB);
		ems_buffer_clear(core_buffer());
		ems_buffer_clear(&sess->buf_in);
		return http_web_redirect_to_mgmt(NULL, sess);
	}


	if (buf_len(&sess->buf_in) < SIZE_REQUEST) 
		return EMS_CONTINUE;

	ems_buffer_prefetch(&sess->buf_in, (ems_char *)req.val, SIZE_REQUEST);

	req.tag.val = ntohl(req.tag.val);
	req.len     = ntohl(req.len);

	ems_l_trace("[ctrl]tag: 0x%.8X, msgid: 0x%.2X, len: 0x%.4X",
					req.tag.val, req.tag.msg, req.len);

	if (req.len >= buf_size(&sess->buf_in))
		return EMS_BUFFER_INSUFFICIENT;

	if (buf_len(&sess->buf_in) < req.len)
		return EMS_CONTINUE;

	return ctrl_cmd_process_one(ctrl, sess, &req);
}


static ems_int
ctrl_cmd_read(ems_ctrl *ctrl, ems_session *sess, ems_int flg)
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
ctrl_cmd_write(ems_ctrl *ctrl, ems_session *sess, ems_int flg)
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

		if (ems_flag_like(sess->flg, SESSION_FLAG_DIE_AFTER_SEND)) 
		{
			return EMS_ERR;
		}

		sess_event_set(sess,   EMS_EVT_READ,    evt_cmd_cb);
		sess_timeout_set_sorted(sess, EMS_CMD_TIMEOUT, timeout_cmd_cb);
		ems_buffer_refresh(&sess->buf);
	}

	return EMS_OK;
}


static ems_void evt_cmd_cb(ems_session *sess, ems_int err, ems_int flg)
{
	ems_ctrl *ctrl = (ems_ctrl *)sess_cbarg(sess);

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
	ems_l_trace("[ctrl] session(%d): %s:%d die",
			ems_sock_fd(&sess->sock),
			ems_sock_addr(&sess->sock),
			ems_sock_port(&sess->sock));
	ems_queue_remove(&sess->entry);
	ems_session_shutdown_and_destroy(sess);
}

static ems_void timeout_cmd_cb(ems_session *sess, ems_timeout *to)
{
	ems_l_trace("[ctrl]timeout for cmd sess(%d): %s:%d",
			ems_sock_fd(&sess->sock),
			ems_sock_addr(&sess->sock),
			ems_sock_port(&sess->sock));

	ems_queue_remove(&sess->entry);
	ems_session_shutdown_and_destroy(sess);
}

static ems_int 
ctrl_sess_in(ems_ctrl *ctrl, ems_int sockfd, ems_cchar *addr, ems_int port)
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
	sess_timeout_set_sorted(sess, EMS_CMD_TIMEOUT, timeout_cmd_cb);

	return EMS_OK;
}

static ems_int 
ctrl_accept_next(ems_ctrl *ctrl, ems_int sock)
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
	ems_ctrl  *ctrl = (ems_ctrl *)sess_cbarg(sess);

	ems_assert(ctrl);
	if (err) {
		ems_assert(0 && "should never be here");
		ctrl_change_status(ctrl, st_stopped);
		/* restart */
		ctrl_change_status(ctrl, st_start);
		return;
	}

	ctrl_evt_run(ctrl, sess, flg);
}

static ems_int ctrl_start(ems_ctrl *ctrl, ems_session *sess, ems_uint flg)
{
	if (!ctrl->sess) {
		ctrl->sess = ems_session_new();
		if (!ctrl->sess)
			return ctrl_change_status(ctrl, st_stopped);

		ems_sock_setaddr(&ctrl->sess->sock, EMS_ADDR);
		ems_sock_setport(&ctrl->sess->sock, EMS_PORT);
	}

	sess = ctrl->sess;

	if (ems_sock_be_server(&sess->sock) != EMS_OK)
		return ctrl_change_status(ctrl, st_stopped);

	sess_cbarg_set(sess, ctrl);
	sess_event_set(sess, EMS_EVT_READ, evt_ctrl_cb);

	return ctrl_change_status(ctrl, st_normal);
}

static ems_int ctrl_stopped(ems_ctrl *ctrl, ems_session *sess, ems_uint flg)
{
	ems_assert(ctrl);

	if (ctrl->sess) {
		ems_session_shutdown_and_destroy(ctrl->sess);
		ctrl->sess = NULL;
	}

	ems_queue_clear(&ctrl->cmd, ems_session, entry, ems_session_shutdown_and_destroy);

	return EMS_OK;
}

static ems_int ctrl_normal(ems_ctrl *ctrl, ems_session *sess, ems_uint flg)
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
ctrl_evt_run(ems_ctrl *ctrl, ems_session *sess, ems_uint flg)
{
	ems_assert(ctrl->st >= st_start && ctrl->st <= st_normal);

	if (ctrl_handler[ctrl->st])
		return ctrl_handler[ctrl->st](ctrl, sess, flg);

	return EMS_OK;
}

ems_int ctrl_change_status(ems_ctrl *ctrl, ems_status st)
{
	ems_l_trace("[ctrl] change status: %s ===> %s", 
			ems_status_str(ctrl->st),
			ems_status_str(st));

	ctrl->st = st;

	switch(st) {
	case st_start:
	case st_stopped:
		return ctrl_evt_run(ctrl, NULL, 0);
		break;

	default:
		break;
	}

	return EMS_OK;
}

