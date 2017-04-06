
#ifdef USE_EMS_SERVER

#include "ems_core.h"
#include "ems_client.h"
#include "app.h"
#include "ems_fw.h"

struct _ems_client_s 
{
	ems_core    *core;
	ems_session *sess;
	ems_status   st;
	ems_void    *ctx;

	ems_uint     upt;
	ems_int      retry;
	ems_uint     flg;
	ems_str      ticket;
};

#define EMS_TIMEOUT_CONNECT	10000
#define EMS_TIMEOUT_ERROR_WAIT	10000
#define EMS_TIMEOUT_REGISTER	10000
#define EMS_TIMEOUT_DEFAULT	10000
#define EMS_TIMEOUT_FOR_HB	10000
#define EMS_TIMEOUT_HB		5000


static ems_void cl_timeout_cb(ems_session *sess, ems_timeout *to);
static ems_void cl_evt_cb    (ems_session *sess, ems_int st, ems_int flg);

static ems_int cl_init(ems_client *cl, ems_session *sess, ems_uint flg);
static ems_int cl_connect(ems_client *cl, ems_session *sess, ems_uint flg);
static ems_int cl_stopped(ems_client *cl, ems_session *sess, ems_uint flg);
static ems_int cl_normal(ems_client *cl, ems_session *sess, ems_uint flg);
static ems_int cl_hb(ems_client *cl, ems_session *sess, ems_uint flg);
static ems_int cl_reg(ems_client *cl, ems_session *sess, ems_uint flg);
static ems_int cl_applist(ems_client *cl, ems_session *sess, ems_uint flg);
static ems_int cl_download(ems_client *cl, ems_session *sess, ems_uint flg);
static ems_int cl_install(ems_client *cl, ems_session *sess, ems_uint flg);
static ems_int cl_err(ems_client *cl, ems_session *sess, ems_uint flg);

typedef ems_int (*cl_evt_func)(ems_client *cl, ems_session *sess, ems_uint flg);
static cl_evt_func ctrl_handler[] = 
{
	[st_init]    = cl_init,
	[st_stopped] = cl_stopped,
	[st_normal]  = cl_normal,
	[st_hb]      = cl_hb,
	[st_reg]     = cl_reg,
	[st_applist] = cl_applist,
	[st_download]= cl_download,
	[st_install] = cl_install,
	[st_err]     = cl_err,
	[st_connect] = cl_connect,
	[st_max]     = NULL
};

typedef ems_int (*cl_timeout_func)(ems_client *fl, ems_session *sess);

static ems_int cl_to_normal(ems_client *cl, ems_session *sess);
static ems_int cl_to_hb(ems_client *cl, ems_session *sess);
static ems_int cl_to_reg(ems_client *cl, ems_session *sess);
static ems_int cl_to_applist(ems_client *cl, ems_session *sess);
static ems_int cl_to_download(ems_client *cl, ems_session *sess);
static ems_int cl_to_install(ems_client *cl, ems_session *sess);
static ems_int cl_to_err(ems_client *cl, ems_session *sess);
static ems_int cl_to_connect(ems_client *cl, ems_session *sess);

static cl_timeout_func timeout_handler[] =
{
	[st_init]    = NULL,
	[st_stopped] = NULL,
	[st_normal]  = cl_to_normal,
	[st_hb]      = cl_to_hb,
	[st_reg]     = cl_to_reg,
	[st_applist] = cl_to_applist,
	[st_download]= cl_to_download,
	[st_install] = cl_to_install,
	[st_err]     = cl_to_err,
	[st_connect] = cl_to_connect,
	[st_max]     = NULL
};

typedef ems_int (*cl_evt_rsp_cb)(ems_client *, ems_session *sess, ems_response *rsp, json_object *root);

static ems_void cl_timeout_cb(ems_session *sess, ems_timeout *to)
{
	ems_client *cl = (ems_client *)sess_cbarg(sess);

	ems_assert(cl->st > st_min && cl->st < st_max);

	ems_assert(timeout_handler[cl->st]);

	if (timeout_handler[cl->st])
		timeout_handler[cl->st](cl, sess);
}

static ems_int cl_evt_run(ems_client *cl, ems_session *sess, ems_int flg)
{
	ems_assert(cl && cl->st > st_min && cl->st < st_max);

	ems_assert(ctrl_handler[cl->st]);

	return ctrl_handler[cl->st](cl, sess, flg);
}

static ems_void cl_evt_cb(ems_session *sess, ems_int err, ems_int flg)
{
	ems_client *cl = (ems_client *)sess_cbarg(sess);

	ems_assert(cl->st > st_min && cl->st < st_max);

	if (err) {
		ems_l_trace("[clnt] evt err, sess: %d %s:%d",
				ems_sock_fd(&sess->sock),
				ems_sock_addr(&sess->sock),
				ems_sock_port(&sess->sock)
				);
		cl_change_status(cl, st_err);
		return;
	}

	cl_evt_run(cl, sess, flg);
}

static  ems_int cl_do_connect(ems_session *sess)
{
	ems_int    fd, ret;
	socklen_t  len;
	struct sockaddr_in addr;
	ems_sock   *sock = &sess->sock;

	ems_assert(sess);
	
	memset(&addr, 0, sizeof(addr));
	if (ems_gethostbyname(ems_sock_addr(sock), &addr) != OK) {
		ems_l_trace("gethostbyename failed %s : %s", 
				ems_sock_addr(sock), ems_lasterrmsg());
		return EMS_ERR;
	}

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd <= 0)
		return EMS_ERR;

	ems_l_trace("[clnt] sess(%d) try to connect to: %s(%s): %d...",
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
			ems_l_trace("[clnt] connect to: %s:%d: failed: %s",
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

static ems_int cl_init(ems_client *cl, ems_session *sess, ems_uint flg)
{
	if (!cl->sess) {
		cl->sess = ems_session_new();
		if (!cl->sess)
			return EMS_ERR;

		sess_cbarg_set(cl->sess, cl);
	}

	sess = cl->sess;
	ems_buffer_clear(&sess->buf);
	ems_buffer_clear(&sess->buf_in);

	ems_sock_setaddr(&sess->sock,          cfg_get(emscfg(), CFG_ems_s_addr));
	ems_sock_setport(&sess->sock, ems_atoi(cfg_get(emscfg(), CFG_ems_s_port)));

	if (cl_do_connect(sess) != EMS_OK)
		return cl_change_status(cl, st_err);

	if (ems_flag_like(sess->flg, EMS_FLG_ONLINE))
		return cl_change_status(cl, st_reg);

	sess_event_set(sess,   EMS_EVT_READ|EMS_EVT_WRITE, cl_evt_cb);
	sess_timeout_set(sess, EMS_TIMEOUT_CONNECT,        cl_timeout_cb);

	return cl_change_status(cl, st_connect);
}

static ems_int cl_connect(ems_client *cl, ems_session *sess, ems_uint flg)
{
	ems_int    err;
	socklen_t len;

	len = sizeof(err);
	err = 0;
	getsockopt(ems_sock_fd(&sess->sock), SOL_SOCKET, SO_ERROR, (ems_char *)&err, &len);

	if (err ) {
		errno = err;
		ems_l_trace("[clnt]sess(%d) connect to %s:%d failed, %s",
				ems_sock_fd(&sess->sock),
				ems_sock_addr(&sess->sock),
				ems_sock_port(&sess->sock),
				ems_geterrmsg(err));
		return cl_change_status(cl, st_err);
	}

	ems_l_trace("[clnt]sess(%d) established with %s:%d",
			ems_sock_fd(&sess->sock),
			ems_sock_addr(&sess->sock),
			ems_sock_port(&sess->sock));

	ems_flag_set(sess->flg, EMS_FLG_ONLINE);

	if (ems_flag_like(cl->flg, FLG_CLIENT_ONLINE))
		return cl_change_status(cl, st_hb);

	return cl_change_status(cl, st_reg);
}

static ems_int cl_stopped(ems_client *cl, ems_session *sess, ems_uint flg)
{
	if (cl->sess) {
		ems_session_shutdown_and_destroy(cl->sess);
		cl->sess = NULL;
	}

	return EMS_OK;
}

static ems_int 
cl_process_rsp(ems_client *cl, ems_session *sess, ems_response *rsp, cl_evt_rsp_cb h)
{
	struct json_object *root;
	ems_int      rtn = EMS_OK;

	ems_assert(cl && sess && rsp);

	root = NULL;
	if (rsp->len > SIZE_RESPONSE) {
		ems_int     len;
		ems_char    *p, ch;

		ems_assert(buf_len(&sess->buf_in) >= rsp->len);

		p = (ems_char *)(buf_rd(&sess->buf_in) + SIZE_RESPONSE);
		getword(p, len);

		ch = p[len]; // backup
		p[len] = '\0';
		root = ems_json_tokener_parse(p);
		p[len] = ch; // restore
	}
#ifdef DEBUG
	{
		ems_cchar *ctx = root?json_object_to_json_string(root):"no ctx";

		ems_l_trace("\033[01;34m[clnt]<rsp tag: 0x%x, st: %d ctx: %s> \033[00m",
					rsp->tag.val, rsp->st, strlen(ctx)> 0x200?"**too long**":ctx);
	}
#endif


	if (h)
		rtn = h(cl, sess, rsp, root);
	else
		rtn = EMS_ERR;

	if (root)
		json_object_put(root);

	ems_buffer_seek_rd(&sess->buf_in, rsp->len, EMS_BUFFER_SEEK_CUR);
	ems_buffer_refresh(&sess->buf_in);
	return rtn;
}


static ems_int cl_preprocess(ems_client *cl, ems_session *sess, cl_evt_rsp_cb h)
{
	ems_response rsp;

	ems_assert(cl && sess && h);

	if (buf_len(&sess->buf_in) < SIZE_RESPONSE) 
		return EMS_CONTINUE;

	ems_buffer_prefetch(&sess->buf_in, (ems_char *)rsp.val, SIZE_RESPONSE);

	rsp.tag.val = ntohl(rsp.tag.val);
	rsp.len     = ntohl(rsp.len);
	rsp.st      = ntohl(rsp.st);

	if (rsp.len >= buf_size(&sess->buf_in)) {
		if (ems_buffer_increase(&sess->buf_in, rsp.len) != EMS_OK)
			return EMS_BUFFER_INSUFFICIENT;
	}

	if (buf_len(&sess->buf_in) < rsp.len)
		return EMS_CONTINUE;

	return cl_process_rsp(cl, sess, &rsp, h);
}


static ems_int 
cl_recv_handle(ems_client *cl, ems_session *sess, cl_evt_rsp_cb h)
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
				cl_preprocess(cl, sess, h);
			return EMS_ERR;
		}
	}

	do {
		ret = cl_preprocess(cl, sess, h);

		switch (ret) {
		case EMS_BUFFER_INSUFFICIENT:
		case EMS_ERR:
			return EMS_ERR;

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

static ems_int cl_send_msg(ems_client *cl, ems_session *sess, ems_uint flg)
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

	if (buf_len(&sess->buf) <= 0) {
		sess_event_set(sess, EMS_EVT_READ, cl_evt_cb);
		sess_timeout_cancel(sess);
		ems_buffer_refresh(&sess->buf);
	}

	return EMS_OK;
}


static ems_int cl_normal(ems_client *cl, ems_session *sess, ems_uint flg)
{
	ems_assert(cl->sess != NULL);

	sess = cl->sess;

	if (sess) {
		sess_event_cancel(sess);
		ems_l_trace("[clnt]shutdown session(%d) with [%s: %d]", 
				ems_sock_fd(&sess->sock),
				ems_sock_addr(&sess->sock),
				ems_sock_port(&sess->sock));
		ems_sock_close(&sess->sock);
		sess_timeout_set(sess, EMS_TIMEOUT_FOR_HB, cl_timeout_cb);
	}

	return EMS_OK;
}

static ems_int 
cl_msg_rsp_hb(ems_client *cl, ems_session *sess, ems_response *rsp, json_object *root)
{
	ems_int upt;

	ems_json_get_int_def(root, "modify_id", upt,  0);
	if (upt != cl->upt) {
		ems_l_trace("server's upt update: from %u into %u", cl->upt, upt);
		cl->upt = upt;
		return cl_change_status(cl, st_applist);
	}

	return cl_change_status(cl, st_normal);
}

static ems_int cl_hb(ems_client *cl, ems_session *sess, ems_uint flg)
{
	if (ems_flag_like(flg, EMS_EVT_READ)) {
		if (cl_recv_handle(cl, sess, cl_msg_rsp_hb) != EMS_OK) 
		{
			return cl_change_status(cl, st_err);
		}
	}

	if (ems_flag_like(flg, EMS_EVT_WRITE)) {
		if (cl_send_msg(cl, sess, flg)  != EMS_OK)
			return cl_change_status(cl, st_err);
	}

	return EMS_OK;
}

static ems_int 
cl_msg_rsp_reg(ems_client *cl, ems_session *sess, ems_response *rsp, json_object *root)
{
	if ((rsp->st == EMS_OK) && root) {
		ems_json_get_string_def(root, "ticket",    &cl->ticket, NULL);
		ems_json_get_int_def(root,    "modify_id",  cl->upt,    0);

		ems_assert(str_len(&cl->ticket) > 0);
		if (str_len(&cl->ticket) > 0) {
			ems_flag_set(cl->flg, FLG_CLIENT_ONLINE);
			return cl_change_status(cl, st_applist);
		}
	}

	return cl_change_status(cl, st_err);
}

static ems_int cl_reg(ems_client *cl, ems_session *sess, ems_uint flg)
{
	if (ems_flag_like(flg, EMS_EVT_READ)) {
		if (cl_recv_handle(cl, sess, cl_msg_rsp_reg) != EMS_OK) 
		{
			return cl_change_status(cl, st_err);
		}
	}

	if (ems_flag_like(flg, EMS_EVT_WRITE)) {
		if (cl_send_msg(cl, sess, flg)  != EMS_OK)
			return cl_change_status(cl, st_err);
	}

	return EMS_OK;
}

static ems_int 
cl_msg_rsp_applist(ems_client *cl, ems_session *sess, ems_response *rsp, json_object *root)
{
	ems_applist_process(root);
	/*
	   check need to download 
	 */

	return cl_change_status(cl, st_normal);
}

static ems_int cl_applist(ems_client *cl, ems_session *sess, ems_uint flg)
{
	if (ems_flag_like(flg, EMS_EVT_READ)) {
		if (cl_recv_handle(cl, sess, cl_msg_rsp_applist) != EMS_OK) 
		{
			return cl_change_status(cl, st_err);
		}
	}

	if (ems_flag_like(flg, EMS_EVT_WRITE)) {
		if (cl_send_msg(cl, sess, flg)  != EMS_OK)
			return cl_change_status(cl, st_err);
	}

	return EMS_OK;
}

static ems_int cl_download(ems_client *cl, ems_session *sess, ems_uint flg)
{
	return EMS_OK;
}

static ems_int cl_install(ems_client *cl, ems_session *sess, ems_uint flg)
{
	return EMS_OK;
}

static ems_int cl_err(ems_client *cl, ems_session *sess, ems_uint flg)
{
	ems_assert(cl->sess != NULL);

	sess = cl->sess;

	if (sess) {
		sess_event_cancel(sess);
		ems_l_trace("[clnt]shutdown session(%d) with [%s: %d]", 
				ems_sock_fd(&sess->sock),
				ems_sock_addr(&sess->sock),
				ems_sock_port(&sess->sock));
		ems_sock_close(&sess->sock);
		sess_timeout_set(sess, EMS_TIMEOUT_ERROR_WAIT, cl_timeout_cb);
	}

	ems_flag_unset(cl->flg, FLG_CLIENT_ONLINE);
	cl->upt   = -1;

	return EMS_OK;
}

static ems_int cl_to_connect(ems_client *cl, ems_session *sess)
{
	ems_l_trace("[clnt]sess(%d) connect to %s:%d timeout",
			ems_sock_fd(&sess->sock),
			ems_sock_addr(&sess->sock),
			ems_sock_port(&sess->sock));

	return cl_change_status(cl, st_err);
}

static ems_int cl_to_normal(ems_client *cl, ems_session *sess)
{
	ems_l_trace("[clnt]sess(%d) time for hb ", ems_sock_fd(&sess->sock));
#ifdef DEBUG
	if (!cl->retry)
		return cl_change_status(cl, st_err);
#endif
	ems_buffer_clear(&sess->buf);
	ems_buffer_clear(&sess->buf_in);

	if (cl_do_connect(sess) != EMS_OK)
		return cl_change_status(cl, st_err);

	if (ems_flag_like(sess->flg, EMS_FLG_ONLINE))
		return cl_change_status(cl, st_hb);

	sess_event_set(sess,   EMS_EVT_READ|EMS_EVT_WRITE, cl_evt_cb);
	sess_timeout_set(sess, EMS_TIMEOUT_CONNECT,        cl_timeout_cb);

	return cl_change_status(cl, st_connect);
}

static ems_int cl_to_hb(ems_client *cl, ems_session *sess)
{
	ems_l_trace("[clnt]sess(%d) send hb timeout to %s:%d ",
			ems_sock_fd(&sess->sock),
			ems_sock_addr(&sess->sock),
			ems_sock_port(&sess->sock));

	return cl_change_status(cl, st_err);
}

static ems_int cl_to_reg(ems_client *cl, ems_session *sess)
{
	ems_l_trace("[clnt]sess(%d) reg to %s:%d timeout",
			ems_sock_fd(&sess->sock),
			ems_sock_addr(&sess->sock),
			ems_sock_port(&sess->sock));

	return cl_change_status(cl, st_err);
}

static ems_int cl_to_applist(ems_client *cl, ems_session *sess)
{
	return EMS_OK;
}

static ems_int cl_to_download(ems_client *cl, ems_session *sess)
{
	return EMS_OK;
}

static ems_int cl_to_install(ems_client *cl, ems_session *sess)
{
	return EMS_OK;
}

static ems_int cl_to_err(ems_client *cl, ems_session *sess)
{
	ems_l_trace("[clnt] time to retry to connect server");
	return cl_change_status(cl, st_init);
}

#ifdef DEBUG
ems_cchar *ems_status_str(ems_status st)
{
	switch(st) {
	case st_init:
		return "init";
	case st_normal:
		return "normal";
	case st_hb:
		return "hb";
	case st_reg:
		return "reg";
	case st_applist:
		return "applist";
	case st_download:
		return "download";
	case st_install:
		return "install";
	case st_err:
		return "err";
	case st_stopped:
		return "stopped";
	case st_connect:
		return "connect";

	case st_auth:
		return "auth";

	case st_acct:
		return "acct";

	case st_acct_stop:
		return "acct_stop";
	default:
		break;
	}

	return "unknown";
}
#endif


static ems_int cl_st_into_reg(ems_client *cl)
{
	ems_session *sess = cl->sess;
	json_object *req;

	ems_assert(cl->sess != NULL);

	req = json_object_new_object();
	json_object_object_add(req, "code", json_object_new_string(cfg_get(emscfg(), CFG_ems_sn)));
	json_object_object_add(req, "mac",  json_object_new_string(cfg_get(emscfg(), CFG_ems_mac)));

	sess_request_set(sess, req);
	core_pack_req(sess,    AC_MSG_REGISTER);
	sess_event_set(sess,   EMS_EVT_WRITE,        cl_evt_cb);
	sess_timeout_set(sess, EMS_TIMEOUT_REGISTER, cl_timeout_cb);

	return EMS_OK;
}

static ems_int cl_st_into_hb(ems_client *cl)
{
	ems_session *sess = cl->sess;
	json_object *req;

	ems_assert(cl->sess != NULL);

	req = json_object_new_object();
	json_object_object_add(req, "ticket", json_object_new_string(str_text(&cl->ticket)));

	sess_request_set(sess, req);
	core_pack_req(sess,    AC_MSG_HB);
	sess_event_set(sess,   EMS_EVT_WRITE,        cl_evt_cb);
	sess_timeout_set(sess, EMS_TIMEOUT_REGISTER, cl_timeout_cb);

	return EMS_OK;
}

static ems_int cl_st_into_applist(ems_client *cl)
{
	ems_session *sess = cl->sess;
	json_object *req;

	ems_assert(cl->sess != NULL);

	req = json_object_new_object();
	json_object_object_add(req, "ticket", json_object_new_string(str_text(&cl->ticket)));

	sess_request_set(sess, req);
	core_pack_req(sess,    AC_MSG_APPLIST);
	sess_event_set(sess,   EMS_EVT_WRITE,        cl_evt_cb);
	sess_timeout_set(sess, EMS_TIMEOUT_REGISTER, cl_timeout_cb);

	return EMS_OK;
}


static ems_int cl_change_status(ems_client *cl, ems_status st)
{
	ems_l_trace("[clnt] change status: %s ---> %s",
			ems_status_str(cl->st), ems_status_str(st));

	cl->st = st;

	switch(cl->st) {
	case st_init:
	case st_stopped:
	case st_err:
		return cl_evt_run(cl, NULL, 0);
		break;

	case st_reg:
		return cl_st_into_reg(cl);
		break;

	case st_applist:
		return cl_st_into_applist(cl);

	case st_normal:
		cl->retry = 5;
		return cl_evt_run(cl, NULL, 0);

	case st_hb:
		return cl_st_into_hb(cl);

	default:
		break;
	}

	return EMS_OK;
}



ems_int clnt_init(ems_core *core)
{
	ems_client *cl = core->clnt;

	memset(cl, 0, sizeof(ems_client));

	cl->core = core;
	cl->st   = st_stopped;
	cl->sess = NULL;

	cl->upt   = -1;
	cl->retry = 0;
	cl->flg   = 0;
	str_init(&cl->ticket);

	return EMS_OK;
}

ems_int clnt_uninit(ems_core *core)
{
	ems_client *cl = core->clnt;
	ems_assert(core != NULL);

	ems_assert(cl->st == st_stopped);

	if (cl->sess) {
		ems_session_shutdown_and_destroy(cl->sess);
		cl->sess = NULL;
	}

	cl->core = NULL;
	cl->st = st_stopped;

	str_uninit(&cl->ticket);

	return EMS_OK;
}

ems_int clnt_run(ems_core *core, ems_int run)
{
	ems_client *cl = core->clnt;
	ems_assert(core != NULL && cl);

	if (run) {
		if (cl->st != st_stopped) 
			return EMS_OK;

		ems_app_modules_run(EMS_YES);
		ems_flag_set(core->flg, FLG_RUN);
		return cl_change_status(cl, st_init);
	} else {

		if (cl->st == st_stopped)
			return EMS_OK;

		ems_app_modules_run(EMS_NO);
		ems_flag_unset(core->flg, FLG_RUN);
		return cl_change_status(cl, st_stopped);
	}

	return EMS_OK;
}

ems_client *clnt_new()
{
	return (ems_client *)ems_malloc(sizeof(ems_client));
}

ems_void  clnt_destroy(ems_client *clnt)
{
	if (clnt)
		ems_free(clnt);
}

ems_int ems_cmd_status_fill_ems(ems_core *core, json_object *root)
{
	ems_client  *cl = core->clnt;
	json_object *rsp = NULL;

	if (!cl)
		return EMS_ERR;

	rsp = json_object_new_object();
	json_object_object_add(rsp,  "status", json_object_new_int(cl->st));
	json_object_object_add(root, "ems_c", rsp);

	return EMS_OK;
}

#endif
