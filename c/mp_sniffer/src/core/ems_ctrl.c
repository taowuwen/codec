
#include "ems_core.h"

/* 
   start --> connect ---> reg --> normal 
    |         |	           |	    |
    |         |	           |	    |
    |         |	           |	    |
    V	      V	           |	    |
   stop<-----err <---------+--------+
 */


#define EMS_CMD_TIMEOUT	3000

#define MSG_REGISTER_SNIFFER	"register_sniffer"
#define MSG_CAPTURE_START	"capture_start"
#define MSG_CAPTURE_STOP	"capture_stop"
#define MSG_STOP_ALL		"stop_all"
#define MSG_CHANNEL_UPDATE	"channel_update"

static ems_void evt_ctrl_cb(ems_session *sess, ems_int err, ems_int flg);
static ems_int ctrl_evt_run(ems_ctrl *ctrl, ems_session *sess, ems_uint flg);

static ems_int ctrl_start(ems_ctrl *ctrl, ems_session *sess, ems_uint flg);
static ems_int ctrl_stopped(ems_ctrl *ctrl, ems_session *sess, ems_uint flg);
static ems_int ctrl_normal(ems_ctrl *ctrl, ems_session *sess, ems_uint flg);
static ems_int ctrl_err(ems_ctrl *ctrl, ems_session *sess, ems_uint flg);
static ems_int ctrl_connect(ems_ctrl *ctrl, ems_session *sess, ems_uint flg);
static ems_int ctrl_reg(ems_ctrl *ctrl, ems_session *sess, ems_uint flg);

typedef ems_int (*ctrl_evt_func)(ems_ctrl *ctrl, ems_session *sess, ems_uint flg);

static ctrl_evt_func ctrl_handler[] = 
{
	[st_start]   = ctrl_start,
	[st_connect] = ctrl_connect,
	[st_reg]     = ctrl_reg,
	[st_normal]  = ctrl_normal,
	[st_err]     = ctrl_err,
	[st_stopped] = ctrl_stopped,
	[st_max]     = NULL
};

static ems_int 
ctrl_handle_msg_from_ac(ems_ctrl *ctrl, ems_session *sess, json_object *req)
{
	ems_int ret;
	ems_str method;
	ems_assert(ctrl->st == st_normal);

	str_init(&method);
	ems_json_get_string_def(req, "method",  &method, NULL);

	if (str_len(&method) <= 0) {
		str_uninit(&method);
		return EMS_OK;
	}

	ret = EMS_ERR;
	if (!strcmp(str_text(&method), MSG_CAPTURE_START)) {
		ret = capture_handle_msg_start(ctrl->_core->capture, req);

	} else if (!strcmp(str_text(&method), MSG_CAPTURE_STOP)) {
		ret = capture_handle_msg_stop(ctrl->_core->capture, req);

	} else if (!strcmp(str_text(&method), MSG_STOP_ALL)) {
		ret = capture_handle_msg_stopall(ctrl->_core->capture, req);

	} else
		ems_l_trace("did not handle method: %s", str_text(&method));

	str_uninit(&method);

	return ret;
}


static ems_int 
apcfg_msg_register(ems_ctrl *ctrl, ems_session *sess, json_object *req)
{
	ems_int st;

	ems_json_get_int_def(req, "status", st, -1);

	if (st == EMS_OK) {
		sess_timeout_cancel(sess);
		return ctrl_change_status(ctrl, st_normal);
	}

	ems_l_error("[ctrl] register failed, status code = %d", st);
	return ctrl_change_status(ctrl, st_err);
}

static ems_int 
apcfg_msg_channel_update(ems_ctrl *ctrl, ems_session *sess, json_object *req)
{
	ems_int st;
	ems_str inf;
	ems_int channel;

	ems_json_get_int_def(req, "status", st, -1);

	if (st == EMS_OK)
		return EMS_OK;

	str_init(&inf);

	ems_json_get_int_def(req, "channel", channel, -1);
	ems_json_get_string_def(req, "interface", &inf, NULL);

	ems_l_error("[ctrl]channel update failed(%d: %s, %d)", 
			st, str_text(&inf), channel);

	cap_omit_channel_update(
		capture_get_cap(ctrl->_core->capture, str_text(&inf)), channel);

	str_uninit(&inf);

	return 0;
}

static ems_int 
ctrl_handle_msg_from_apcfg(ems_ctrl *ctrl, ems_session *sess, json_object *req)
{
	ems_str method;
	ems_assert(ctrl->st == st_normal || ctrl->st == st_reg);

	str_init(&method);
	ems_json_get_string_def(req, "method",  &method, NULL);

	if (str_len(&method) <= 0) {
		str_uninit(&method);
		return EMS_OK;
	}

	if (!strcmp(str_text(&method), MSG_REGISTER_SNIFFER)) {
		ems_assert(ctrl->st == st_reg);
		if (ctrl->st == st_reg)
			apcfg_msg_register(ctrl, sess, req);

	} else if (!strcmp(str_text(&method), MSG_CHANNEL_UPDATE)) {
		apcfg_msg_channel_update(ctrl ,sess, req);
	}
	else
		ems_l_trace("unknown method: %s", str_text(&method));


	str_uninit(&method);

	return EMS_OK;
}

static ems_int 
ctrl_cmd_handle(ems_ctrl *ctrl, ems_session *sess, json_object *req, ems_uint tag)
{
	ems_l_trace("[ctrl]<< \033[01;33m req: %s \033[00m>>",
			req?json_object_to_json_string(req): "no arg");

	switch(tag) {
	case MSG_FROM_AC:
		return ctrl_handle_msg_from_ac(ctrl, sess, req);

	case MSG_FROM_APCFG:
		return ctrl_handle_msg_from_apcfg(ctrl, sess, req);

	default:
		ems_l_trace("[ctrl], did not handle for now...%x", tag);
		break;
	}

	return EMS_ERR;
}

static ems_int
ctrl_process_msg(ems_ctrl *ctrl, ems_session *sess)
{
	ems_request  req;
	json_object *jobj = NULL;

	while (buf_len(&sess->buf_in) >= SIZE_REQUEST) {

		ems_buffer_prefetch(&sess->buf_in, (ems_char *)req.val, SIZE_REQUEST);

		req.tag = ntohs(req.tag);
		req.len = ntohs(req.len);
		ems_assert(req.len >= SIZE_REQUEST);

		ems_l_trace("[ctrl]tag: 0x%.4X, len: 0x%.4X [buf(size: %d, len: %d, left: %d)]",
					req.tag, req.len,
					buf_size(&sess->buf_in), buf_len(&sess->buf_in), buf_left(&sess->buf_in));
		if (req.len < SIZE_REQUEST)
			return EMS_ERR;

		if (req.len >= buf_size(&sess->buf_in))
			return EMS_BUFFER_INSUFFICIENT;

		if (buf_len(&sess->buf_in) < req.len)
			return EMS_CONTINUE;

		if (req.len > SIZE_REQUEST) {
			ems_char *p;

			p = (ems_char *)(buf_rd(&sess->buf_in) + SIZE_REQUEST);
			jobj = ems_json_tokener_parse_ex(p, req.len - SIZE_REQUEST);
		}

		ctrl_cmd_handle(ctrl, sess, jobj, req.tag);

		if (jobj) {
			json_object_put(jobj);
			jobj = NULL;
		}

		ems_buffer_seek_rd(&sess->buf_in, req.len, EMS_BUFFER_SEEK_CUR);
	}

	ems_buffer_refresh(&sess->buf_in);

	return EMS_CONTINUE;
}

static ems_void timeout_ctrl_cb(ems_session *sess, ems_timeout *to)
{
	ems_ctrl  *ctrl = (ems_ctrl *)sess_cbarg(sess);

	ems_assert(ctrl != NULL);

	switch (ctrl->st) {
	case st_reg:
	case st_connect:
		ems_l_warn("sniffer register error: register timeout");
		ctrl_change_status(ctrl, st_err);
		break;

	default:
		ems_assert(0); /* never show up this line */
		break;
	}

}

static ems_void evt_ctrl_cb(ems_session *sess, ems_int err, ems_int flg)
{
	ems_ctrl  *ctrl = (ems_ctrl *)sess_cbarg(sess);

	ems_assert(ctrl);
	if (err) {
		ems_l_error("[ctrl] lost peer, shutdown sniffer");
		ctrl_change_status(ctrl, st_err);
		return;
	}

	ctrl_evt_run(ctrl, sess, flg);
}

static ems_int ctrl_do_conect(ems_session *sess)
{
	ems_int    fd, ret;
	socklen_t  len;
	struct sockaddr_un addr;
	ems_sock  *sock = &sess->sock;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(fd < 0) {
		ems_l_warn("[ctrl] create socket failed: %s", ems_geterrmsg(errno));
		return EMS_ERR;
	}

	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", ems_sock_addr(sock));

	ems_setnonblocking(fd, YES);

	len = sizeof(struct sockaddr_un);
	ret = connect(fd, (struct sockaddr *)&addr, len);
	switch(ret) {
	case 0:
		ems_flag_set(sess->flg, FLG_ONLINE);
		break;

	case -1:
		ret = ems_lasterr();
		ems_flag_unset(sess->flg, FLG_ONLINE);

		if (ret != EINPROGRESS) {
			close(fd);
			ems_l_warn("[ctrl] connect to: %s failed: %s",
					ems_sock_addr(sock), ems_geterrmsg(ret));
			return EMS_ERR;

		}
	default:
		break;
	}

	ems_sock_setfd(sock, fd);

	return EMS_OK;
}

static ems_int ctrl_start(ems_ctrl *ctrl, ems_session *sess, ems_uint flg)
{
	if (!ctrl->sess) {
		ctrl->sess = ems_session_new();
		if (!ctrl->sess) {
			ctrl_change_status(ctrl, st_stopped);
			return EMS_ERR;
		}
		sess_cbarg_set(ctrl->sess, ctrl);
	}

	sess = ctrl->sess;

	ems_buffer_clear(&sess->buf);
	ems_buffer_clear(&sess->buf_in);

	ems_sock_setaddr(&sess->sock, "/tmp/sniffer_apcfg_sock");
	ems_sock_setport(&sess->sock, 0);

	if (ctrl_do_conect(sess) != EMS_OK) {
		ctrl_change_status(ctrl, st_stopped);
		return EMS_ERR;
	}

	if (ems_flag_like(sess->flg, FLG_ONLINE))
		return ctrl_change_status(ctrl, st_reg);

	sess_event_set(sess, EMS_EVT_READ, evt_ctrl_cb);
	return ctrl_change_status(ctrl, st_connect);
}

static ems_int ctrl_connect(ems_ctrl *ctrl, ems_session *sess, ems_uint flg)
{
	ems_int    err;
	socklen_t len;

	len = sizeof(err);
	err = 0;
	getsockopt(ems_sock_fd(&sess->sock), SOL_SOCKET, SO_ERROR, (ems_char *)&err, &len);

	if (err) {
		errno = err;
		ems_l_trace("[ctrl]sess(%d) connect to %s failed, %s",
				ems_sock_fd(&sess->sock), ems_sock_addr(&sess->sock), ems_geterrmsg(err));

		return ctrl_change_status(ctrl, st_err);
	}

	ems_l_trace("[ctrl]sess(%d) established with %s",
			ems_sock_fd(&sess->sock),
			ems_sock_addr(&sess->sock));

	ems_flag_set(sess->flg, FLG_ONLINE);

	sess_event_cancel(sess);
	return ctrl_change_status(ctrl, st_reg);
}



static ems_int ctrl_do_read(ems_ctrl *ctrl, ems_session *sess, ems_uint flg)
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
			/* no matter what we got here, we need no care now,
			   just return EMS_ERR;
			 */
			return EMS_ERR;
		}
	}

	switch (ctrl_process_msg(ctrl, sess)) {
		
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

static ems_int ctrl_do_send(ems_ctrl *ctrl, ems_session *sess, ems_uint flg)
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
		sess_event_set(sess, EMS_EVT_READ, evt_ctrl_cb);
		ems_buffer_refresh(&sess->buf);
	}

	return EMS_OK;
}

static ems_int ctrl_reg(ems_ctrl *ctrl, ems_session *sess, ems_uint flg)
{
	if (ems_flag_like(flg, EMS_EVT_READ))
		if (ctrl_do_read(ctrl, sess, flg) != EMS_OK)
			goto err_out;

	if (ems_flag_like(flg, EMS_EVT_WRITE))
		if (ctrl_do_send(ctrl, sess, flg) != EMS_OK)
			goto err_out;

	return EMS_OK;

err_out:
	ems_l_trace("[ctrl] register failed");
	return ctrl_change_status(ctrl, st_err);
}

static ems_int ctrl_normal(ems_ctrl *ctrl, ems_session *sess, ems_uint flg)
{
	if (ems_flag_like(flg, EMS_EVT_READ))
		if (ctrl_do_read(ctrl, sess, flg) != EMS_OK)
			goto err_out;

	if (ems_flag_like(flg, EMS_EVT_WRITE))
		if (ctrl_do_send(ctrl, sess, flg) != EMS_OK)
			goto err_out;

	return EMS_OK;
err_out:
	ems_l_trace("[ctrl] normal failed");
	return ctrl_change_status(ctrl, st_err);
}

static ems_int ctrl_err(ems_ctrl *ctrl, ems_session *sess, ems_uint flg)
{
	output_change_status(ctrl->_core->output, st_stopped);
	capture_change_status(ctrl->_core->capture, st_stopped);

	return ctrl_change_status(ctrl, st_stopped);
}


static ems_int ctrl_stopped(ems_ctrl *ctrl, ems_session *sess, ems_uint flg)
{
	ems_assert(ctrl);

	if (ctrl->sess) {
		ems_session_shutdown_and_destroy(ctrl->sess);
		ctrl->sess = NULL;
	}

	ems_event_end(eventer());

	return EMS_OK;
}

static ems_int 
ctrl_evt_run(ems_ctrl *ctrl, ems_session *sess, ems_uint flg)
{
	ems_assert(ctrl->st > st_min && ctrl->st < st_max);

	if (ctrl_handler[ctrl->st])
		return ctrl_handler[ctrl->st](ctrl, sess, flg);

	return EMS_OK;
}


/*
   send APCFG reg msg
 */
static ems_int
ctrl_status_goes_into_reg(ems_ctrl *ctrl, ems_session *sess, ems_uint flg)
{
	json_object *jobj = NULL;
	ems_int      cap;

	jobj = json_object_new_object();

	json_object_object_add(jobj, "method", json_object_new_string(MSG_REGISTER_SNIFFER));
/* 
   +--------+   
    000000SRL
   +--------+   
    S: provide service while capturing : be 0
    R: Radio capture
    L: Local Capture

    000000011 = 0x03 = 3
 */
	cap = 0x03;
	json_object_object_add(jobj, "capability", json_object_new_int(cap));

	sess = ctrl->sess;

	sess_request_set(sess, jobj);

	core_pack_req(sess, MSG_EVT_SEND_TO_APCFG);
	sess_event_set(sess, EMS_EVT_WRITE, evt_ctrl_cb);
	sess_timeout_set_sorted(sess, 3000, timeout_ctrl_cb);

	return EMS_OK;
}

ems_int ctrl_change_status(ems_ctrl *ctrl, ems_status st)
{
	ems_l_trace("[ctrl] change status from %s into %s", 
			ems_status_str(ctrl->st), ems_status_str(st));
	if (ctrl->st == st)
		return EMS_OK;

	ctrl->st = st;

	switch(st) {
	case st_start:
	case st_stopped:
	case st_err:
		return ctrl_evt_run(ctrl, NULL, 0);
		break;

	case st_reg:
		return ctrl_status_goes_into_reg(ctrl, NULL, 0);
	default:
		break;
	}

	return EMS_OK;
}

ems_int ctrl_send_msg(ems_ctrl *ctrl, ems_uint tag, json_object *req)
{
	ems_session *sess;
	ems_assert(ctrl && req != NULL);

	if (!req)
		return EMS_ERR;

	if ((ctrl->st == st_err) || (ctrl->st == st_stopped)) {
		json_object_put(req);
		return EMS_OK;
	}

	sess = ctrl->sess;

	sess_request_set(sess, req);
	core_pack_req(sess, tag);
	sess_event_set(sess, EMS_EVT_WRITE | EMS_EVT_READ, evt_ctrl_cb);

	return EMS_OK;
}

