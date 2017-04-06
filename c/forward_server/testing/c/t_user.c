
#include "t_main.h"
#include "t_user.h"
#include "t_session.h"
#include "t_logger.h"
#include "fwd_sock.h"
#include "fwd_buffer.h"
#include "t_worker.h"
#include "json.h"
#include "fwd_transinfo.h"
#include "t_status.h"
#include "t_peer.h"
#include "t_self.h"


static dt_void user_fwd_timeout_cb(t_timeout *to);
static dt_void user_fwd_evt_cb(t_event_fd *evt, dt_int flgs);


static dt_int user_fwd_timeout_conn(t_user *user);
static dt_int user_fwd_timeout_reg(t_user *user);

static user_timeout_func user_fwd_timeout_handler[] = 
{
	[ST_FWD_INIT] = NULL,
	[ST_FWD_CONN] = user_fwd_timeout_conn,
	[ST_FWD_REG]  = user_fwd_timeout_reg,
	[ST_FWD_EST]  = NULL,
	[ST_FWD_HB]   = NULL,
	[ST_FWD_REREG]= NULL,
	[ST_FWD_MAX]  = NULL
};

static dt_int user_fwd_evt_init(t_user *user, dt_int flgs);
static dt_int user_fwd_evt_conn(t_user *user, dt_int flgs);
static dt_int user_fwd_evt_reg (t_user *user, dt_int flgs);
static dt_int user_fwd_evt_est (t_user *user, dt_int flgs);
static dt_int user_fwd_evt_hb  (t_user *user, dt_int flgs);

static user_event_func user_fwd_evt_handler[] = 
{
	[ST_FWD_INIT] = user_fwd_evt_init,
	[ST_FWD_CONN] = user_fwd_evt_conn,
	[ST_FWD_REG]  = user_fwd_evt_reg,
	[ST_FWD_EST]  = user_fwd_evt_est,
	[ST_FWD_HB]   = user_fwd_evt_hb,
	[ST_FWD_REREG]= NULL,
	[ST_FWD_MAX]  = NULL
};


static dt_int user_fwd_timeout_run(t_user *user)
{
	dt_assert(user && user_session(user) && "bugs");

	if(user_fwd_timeout_handler[user_fwd_status(user)])
		return user_fwd_timeout_handler[user_fwd_status(user)](user);

	dt_assert(0 && "should not be here for now");
	return T_OK;
}


static dt_int user_fwd_evt_run(t_user *user, dt_int flgs)
{
	dt_assert(user && user_session(user) && "bugs");

	if(user_fwd_evt_handler[user_fwd_status(user)])
		return user_fwd_evt_handler[user_fwd_status(user)](user, flgs);

	dt_assert(0 && "should not be here for now");

	return T_OK;
}

static  dt_int t_user_connect_fwd(t_user *user)
{
	dt_int 	  ret, fd;
	socklen_t len;
	struct sockaddr_in addr;

	dt_assert(fwd_sock_addr(&user->fwd));
	fd = socket(AF_INET, SOCK_STREAM, 0);

	if (fd <= 0)
		return T_ERR;

	dt_setnonblocking(fd, YES);
	memset(&addr, 0, sizeof(addr));

#if 0
	addr.sin_family = AF_INET;
	addr.sin_port   = htons(fwd_sock_port(&user->fwd));
	addr.sin_addr.s_addr = inet_addr(fwd_sock_addr(&user->fwd));
#endif
	memcpy(&addr, t_main_cm_addr(), sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port   = htons(fwd_sock_port(&user->fwd));
	len = sizeof(struct sockaddr_in);

	ret = connect(fd, (struct sockaddr *)&addr, len);

	switch(ret) {

	case 0:
		/* success here*/
		fwd_sock_setfd(&user->fwd, fd);
		return t_user_fwd_change_status(user, ST_FWD_REG);
		break;

	default:
		ret = fwd_lasterr();
		switch(ret) {

		case EINPROGRESS:
			break;

		default:
			close(fd);
			t_logger_info(user_session(user), user, 
				"[fwd] connect to: %s:%d failed: %s", 
				fwd_sock_addr(&user->fwd), fwd_sock_port(&user->fwd), t_lasterrstr());
					
			return T_ERR;
		}
	}

	fwd_sock_setfd(&user->fwd, fd);
	user_fwd_timeout_set(user, T_TIMEOUT_CONNECT_FWD, user_fwd_timeout_cb);
	user_fwd_event_set(user, FWD_EVT_WRITE|FWD_EVT_EDGE_TRIGGER, user_fwd_evt_cb);

	return t_user_fwd_change_status(user, ST_FWD_CONN);
}

static dt_int user_fwd_connect_rsp(t_user *user)
{
	dt_int    err;
	socklen_t len;

	len = sizeof(err);
	err = 0;

	getsockopt(fwd_sock_fd(&user->fwd), SOL_SOCKET, SO_ERROR, (dt_char *)&err, &len);

	if (err) {
		t_logger_warn(user_session(user), user, "[fwd]connect to: %s:%d failed: %s",
					fwd_sock_addr(&user->fwd), 
					fwd_sock_port(&user->fwd), 
					fwd_getlasterrstr(err));
		return T_ERR;
	}
	
	t_logger_trace(user_session(user), user, "[fwd]established with %s:%d",
				fwd_sock_addr(&user->fwd), 
				fwd_sock_port(&user->fwd));
	return T_OK;
}


static dt_int user_fwd_timeout_conn(t_user *user)
{
	dt_assert(user && user_session(user));

	t_logger_info(user_session(user), user, "[fwd]connect to : %s:%d timeout", 
			fwd_sock_addr(&user->fwd), fwd_sock_port(&user->fwd));

	t_main_failed_inc();
	return t_user_fwd_change_status(user, ST_FWD_REREG);
//	return t_session_changestatus(user_session(user), ST_SESSION_ERROR);
}

static dt_int user_fwd_timeout_reg(t_user *user)
{
	dt_assert(user && user_session(user));

	t_logger_info(user_session(user), user, "[fwd]request to : %s:%d timeout", 
			fwd_sock_addr(&user->fwd), fwd_sock_port(&user->fwd));

	t_main_failed_inc();
	return t_user_fwd_change_status(user, ST_FWD_REREG);
//	return t_session_changestatus(user_session(user), ST_SESSION_ERROR);
}

static dt_int user_fwd_evt_init(t_user *user, dt_int flgs)
{
	dt_assert(user && user_session(user) && "bug");
	dt_assert(user && user_peer(user) && user_peer(user_peer(user)) == user);

	user->fwd_buf_len = -1;
	if (t_user_connect_fwd(user) == T_ERR) {
		t_main_failed_inc();
		return t_user_fwd_change_status(user, ST_FWD_REREG);
	//	return t_session_changestatus(user_session(user), ST_SESSION_ERROR);
	}

	return T_OK;
}

static dt_int user_fwd_evt_conn(t_user *user, dt_int flgs)
{
	dt_assert(user && user_session(user) && "bug");

	if (!(flgs & FWD_EVT_WRITE))
		return T_OK;

	if (user_fwd_connect_rsp(user) != T_OK) {
		t_main_failed_inc();
		return t_user_fwd_change_status(user, ST_FWD_REREG);
	//	return t_session_changestatus(user_session(user), ST_SESSION_ERROR);
	}

	user_fwd_timeout_cancel(user);
	user_fwd_event_cancel(user);

	return t_user_fwd_change_status(user, ST_FWD_REG);
}

static dt_int user_fwd_handle_rsp(t_user *user)
{
//	t_logger_trace(user_session(user), user, "recved: %s", buf_rd(&user->fwd_buf));

//	fwd_trans_read(&user->fwd_trans, buf_len(&user->fwd_buf));
	fwd_trans_read(&user_session(user)->trans, buf_len(&user->fwd_buf));

	if (user->fwd_buf_len < 0) {
		user->fwd_buf_len = 0;
#if 0
		{
#define HTTPOK "HTTP/1.1 200"
#define MARK "Content-Length: "
			dt_char  *cl;
			dt_int    len;
			dt_char   buf[256] = {0};

			len = buf_len(&user->fwd_buf) <= 256? buf_len(&user->fwd_buf): 255;

			memcpy(buf, buf_rd(&user->fwd_buf), len);

			/*
			 * check header
			 * */
			if (strncasecmp(HTTPOK,  buf, strlen(HTTPOK))) {
				cl = strstr(buf_rd(&user->fwd_buf), "\r\n");
				if (cl)
					*cl = '\0';
				t_logger_info(user_session(user), user, "http response error: %s", buf_rd(&user->fwd_buf));
				return T_ERR;
			}

			/*
			 * check length
			 * */
			cl = strstr(buf, MARK);
			if (cl) {
				cl += strlen(MARK);
				user->fwd_buf_len = strtoll(cl, NULL, 10);

				if (!t_test(T_FLG_RECV_ALL) 
					&& user->fwd_buf_len > (FWD_BUFFER_16k * 16))
				{
					user->fwd_buf_len = FWD_BUFFER_16k * 16;
				}
			}

		}
#endif
	}

	user->fwd_buf_len -= buf_len(&user->fwd_buf);
	fwd_buffer_drain(&user->fwd_buf, buf_len(&user->fwd_buf));
	fwd_buffer_refresh(&user->fwd_buf);

	return T_OK;

#if 0
	dt_int       ret;
	fwd_response rsp;

	ret = fwd_buffer_unpack(&user->fwd_buf, (fwd_request *)&rsp, NULL, NULL);

	if (ret == T_OK) {
		if (rsp.st != 0) {
			t_logger_info(user_session(user), user, "fwd server rsp error code : %d", rsp.st);
			return T_ERR;
		}
	}


	return ret;
#endif
}


static dt_int user_fwd_evt_reg (t_user *user, dt_int flgs)
{
	dt_int ret;

	if (flgs & FWD_EVT_WRITE) {
		ret = fwd_sock_send(&user->fwd, &user->fwd_buf);

		if (ret <= 0) {
			t_logger_info(user_session(user), user, "send request failed: %d", buf_len(&user->fwd_buf));
			t_main_failed_inc();
			return t_user_fwd_change_status(user, ST_FWD_REREG);
		}

		if (buf_len(&user->fwd_buf) <= 0) {
			user_fwd_event_set(user, FWD_EVT_READ|FWD_EVT_EDGE_TRIGGER, user_fwd_evt_cb);
			fwd_buffer_refresh(&user->fwd_buf);
		}

		//fwd_trans_write(&user->fwd_trans, ret);
		fwd_trans_write(&user_session(user)->trans, ret);

		return T_OK;
	}

	if (flgs & FWD_EVT_READ) {
		ret = fwd_sock_read(&user->fwd, &user->fwd_buf);

		if (ret <= 0) {
			if (ret == -EAGAIN)
				return T_OK;
			t_logger_info(user_session(user), user, "recv failed reponse");
			t_main_failed_inc();
			return t_user_fwd_change_status(user, ST_FWD_REREG);
			//return t_session_changestatus(user_session(user), ST_SESSION_ERROR);
		}

		switch(user_fwd_handle_rsp(user)) {

		case T_OK:

			if (user->fwd_buf_len <= 0) {
				t_main_success_inc();
				t_user_fwd_change_status(user, ST_FWD_REREG);
			}
#if 0
			user_fwd_timeout_cancel(user);
			user_fwd_event_cancel(user);
			t_user_fwd_change_status(user, ST_FWD_EST);
#endif
			break;
		case T_ERR:
			t_main_failed_inc();
			t_user_fwd_change_status(user, ST_FWD_REREG);
		//	return t_session_changestatus(user_session(user), ST_SESSION_ERROR);
		default:
			break;
		}

		return T_OK;
	}

	return T_OK;

}

static dt_int user_fwd_evt_est (t_user *user, dt_int flgs)
{
	if (!user_is_self(user))
		return t_peer_fwd_established(user, flgs);

	return  t_self_fwd_established(user, flgs);
}

static dt_int user_fwd_evt_hb  (t_user *user, dt_int flgs)
{
	dt_assert(0 && "should not be here");
	return T_OK;
}

dt_int t_user_cm_connect(t_user *user)
{
	dt_int    fd;
	socklen_t len;
	struct sockaddr_in addr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);

	if (fd <=0 )
		return T_ERR;

	memcpy(&addr, t_main_cm_addr(), sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port   = htons(fwd_sock_port(&user->cm));

	len = sizeof(struct sockaddr_in);

	if (connect(fd, (struct sockaddr *)&addr, len) == -1) {
		t_logger_warn(user_session(user), user, "connect failed: %s", t_lasterrstr());
		close(fd);
		return T_ERR;
	}

	fwd_sock_setfd(&user->cm, fd);

	return T_OK;
}

dt_int t_user_cm_handle_msg(t_user *user)
{
#pragma pack(push, 1)
typedef struct {
	unsigned short	len;
	unsigned short  ver;
	unsigned short  cmd;

	char 		fwd_ipv6[16];
	unsigned int	fwd_ip;
	unsigned short	fwd_port;
	char		fwd_uuid[32];
	char		ticket[16];
	char		err;
	char		rsrv[29];
} t_ticket_rsp;
#pragma pack(pop)

	t_ticket_rsp *rsp = NULL;
	dt_char buf[128] = {0};

	dt_assert(user && user_session(user) && "bug?");

	rsp = (t_ticket_rsp *)buf_rd(&user->cm_buf);

	if (buf_len(&user->cm_buf) < ntohs(rsp->len))
		return FWD_CONTINUE;

	if (ntohs(rsp->len) != sizeof(t_ticket_rsp)) {
		t_logger_warn(user_session(user), user, 
			"[cm]invalid msg header, len: %d , should be: %d", 
			ntohs(rsp->len), sizeof(t_ticket_rsp));

		return T_ERR;
	}

	rsp->len     = ntohs(rsp->len);
	rsp->ver     = ntohs(rsp->ver);
	rsp->cmd     = ntohs(rsp->cmd);
//	rsp->fwd_ip  = ntohl(rsp->fwd_ip); /* do not to ntohl */
	rsp->fwd_port= ntohs(rsp->fwd_port);

	fwd_buffer_seek_rd(&user->cm_buf, rsp->len, FWD_BUFFER_SEEK_CUR);

	if (rsp->err == 0) {
		memcpy(buf, rsp->ticket, 16);
		user_ticket_set(user, buf);
	} else {
		t_logger_warn(user_session(user), user, 
			"request fwd failed for: %s , stcode: %d",
			session_uuid(user_session(user)), rsp->err);

		dt_assert(user == session_self(user_session(user)));

		if (user_is_self(user) && user_retry(user))
			return FWD_CONTINUE;

		return T_ERR;
	}

	fwd_sock_setaddr(&user->fwd, inet_ntoa(*(struct in_addr *)&rsp->fwd_ip));
	fwd_sock_setport(&user->fwd, rsp->fwd_port);

	t_logger_trace(user_session(user), user, 
		"err: %d, ticket rsp: %s, fwd_ip: %s, fwd_port: %d", 
		rsp->err, buf, inet_ntoa(*(struct in_addr *)&rsp->fwd_ip), rsp->fwd_port);


	if (fwd_strlen(fwd_sock_addr(&user->fwd)) <= 0 || fwd_sock_port(&user->fwd) <= 0) 
	{
		t_logger_warn(user_session(user), user, "invalid sock address or port: %s:%d",
				fwd_sock_addr(&user->fwd), fwd_sock_port(&user->fwd));
		return T_ERR;
	}

	return T_OK;
}

static dt_int user_fwd_st_init(t_user *user)
{
	return user_fwd_evt_run(user, 0);
}


#if 0
static dt_int user_ticket_set_cb(dt_char *buf, dt_int len, dt_cchar *arg)
{
	json_object *obj;

	obj = json_object_new_object();

	json_object_object_add(obj, "ticket", json_object_new_string(arg));

	snprintf(buf, len, "%s", json_object_to_json_string(obj));

	json_object_put(obj);

	return T_OK;
}
#endif

static dt_int user_fwd_st_reg(t_user *user)
{
	//fwd_buffer_pack(&user->fwd_buf, MSG_FWD_REQUEST, 0, user_ticket_set_cb, user_ticket(user));
	{
		dt_int       len;
		dt_char      *wr;
		wr = buf_wr(&user->fwd_buf);

//"GET /redirect.php HTTP/1.1\r\n"
//"GET /fuqqtest/test.jpg HTTP/1.1\r\n"
//"GET /index.php HTTP/1.1\r\n"
//"GET /indexs.html HTTP/1.1\r\n"
//"GET /index.php?m=homepage&a=index&user=gongzonghang HTTP/1.1\r\n"
//"GET /%s HTTP/1.1\r\n"
//"User-Agent: Wget/1.14 (linux-gnu)\r\n"
//"Accept: */*\r\n"
//"Host: %s\r\n"
//"Connection: Keep-Alive\r\n\r\n",
		len = snprintf(wr, buf_left(&user->fwd_buf), "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
		fwd_buffer_seek_wr(&user->fwd_buf, len, FWD_BUFFER_SEEK_CUR);
	}


	user_fwd_timeout_set(user, T_TIMEOUT_FWD_SENDMSG, user_fwd_timeout_cb);
	user_fwd_event_set(user, FWD_EVT_WRITE|FWD_EVT_EDGE_TRIGGER, user_fwd_evt_cb);

	return T_OK;
}

static dt_int user_fwd_st_rereg(t_user *user)
{
	user_fwd_timeout_cancel(user);
	user_fwd_event_cancel(user);
	if (user->fwd_retry) {
		user->fwd_retry--;
		t_logger_trace(user_session(user), user, "left: %d", user->fwd_retry);
		fwd_sock_close(&user->fwd);
		return t_user_fwd_change_status(user, ST_FWD_INIT);
	} else {
		return t_session_changestatus(user_session(user), ST_SESSION_ERROR);
	}
}

static dt_int user_fwd_st_est(t_user *user)
{
	t_logger_info(user_session(user), user, "user established with, could send datas to peer");

	if (user_is_self(user))
		return user_fwd_evt_run(user, 0);
	else
		user_fwd_event_set(user, FWD_EVT_READ|FWD_EVT_EDGE_TRIGGER, user_fwd_evt_cb);

	return T_OK;
}

dt_int t_user_init(t_user *user)
{
	dt_assert(user && "bug");

	memset(user, 0, sizeof(t_user));

	user_attach(user, NULL);
	user_peer_set(user, NULL);

	fwd_sock_init(&user->cm);
	fwd_timeout_init(&user->cm_to);
	fwd_event_fd_init(&user->cm_evt);
	fwd_buffer_init(&user->cm_buf, FWD_BUFFER_8k);
	user->cm_st = ST_CM_MIN;

	fwd_sock_init(&user->fwd);
	fwd_timeout_init(&user->fwd_to);
	fwd_event_fd_init(&user->fwd_evt);
	fwd_buffer_init(&user->fwd_buf, FWD_BUFFER_16k);
	fwd_buffer_init(&user->fwd_out, FWD_BUFFER_16k);
	user->fwd_st = ST_FWD_MIN;
	fwd_trans_init(&user->fwd_trans);

	str_init(&user->ticket);
	str_init(&user->fl.marks);
	str_init(&user->fwd_arg);

	user->fwd_retry = 0;
	user->fwd_buf_len = -1;

	return T_OK;
}

dt_void t_user_uninit(t_user *user)
{
	dt_assert(user && "bug???");

	if (user_session(user)) {
		user_cm_timeout_cancel(user);
		user_cm_event_cancel(user);

		user_fwd_timeout_cancel(user);
		user_fwd_event_cancel(user);

		user_detach(user);
	}

	user_peer_set(user, NULL);

	fwd_buffer_uninit(&user->cm_buf);
	fwd_buffer_uninit(&user->fwd_buf);
	fwd_buffer_uninit(&user->fwd_out);

	fwd_sock_close(&user->cm);
	fwd_sock_close(&user->fwd);

	fwd_sock_clear(&user->cm);
	fwd_sock_clear(&user->fwd);

	str_clear(&user->ticket);
	str_clear(&user->fwd_arg);
	fwd_trans_uninit(&user->fwd_trans);

	user->cm_st  = ST_CM_MAX;
	user->fwd_st = ST_FWD_MAX; 

	str_clear(&user->fl.marks);
}

t_user  *t_user_create()
{
	t_user *user = NULL;

	user = (t_user *)dt_malloc(sizeof(t_user));

	if (user) {
		if (t_user_init(user) != T_OK) {
			dt_assert(0 && "bug?");
			dt_free(user);
			user = NULL;
		}
	}

	return user;
}

dt_void t_user_destroy(t_user *user)
{
	if (user) {
		t_user_uninit(user);
		dt_free(user);
	}
}

static dt_void user_fwd_timeout_cb(t_timeout *to)
{
	t_user *user = dt_container_of(to, t_user, fwd_to);
	user_fwd_timeout_run(user);
}

static dt_void user_fwd_evt_cb(t_event_fd *evt, dt_int flgs)
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
		user_fwd_evt_run(user, flgs);
}

dt_int t_user_timeout_set(t_user *user, t_timeout *to, dt_uint delay, user_timeout_func_cb cb)
{
	t_session *sess;

	dt_assert(user && user_session(user) && "bug");

	sess = user_session(user);

	return fwd_timeout_set(worker_event(session_base(sess)), to, delay, cb, 0);
}

dt_int t_user_timeout_cancel(t_timeout *to)
{
	return fwd_timeout_cancel(to);
}

dt_int t_user_event_set(t_user *user, t_event_fd *evt, dt_int flgs, user_event_func_cb cb)
{
	t_session *sess;

	dt_assert(user && user_session(user) && "bug");
	sess = user_session(user);

	return fwd_event_add(worker_event(session_base(sess)), evt, flgs, cb);
}

dt_int t_user_event_cancel(t_user *user, t_event_fd *evt)
{
	t_session *sess;

	dt_assert(user && user_session(user) && "bug");

	sess = user_session(user);
	return fwd_event_del(worker_event(session_base(sess)), evt);
}



dt_int t_user_fwd_change_status(t_user *user, t_fwd_status st)
{
	dt_int ret;
	dt_assert(user && user_session(user) && "bug");

	t_logger_trace(user_session(user), user, "[fwd] change status [%s --> %s]",
				t_fwd_status_str(user_fwd_status(user)), t_fwd_status_str(st));

	ret = T_OK;
	user->fwd_st = st;

	switch(st) {

	case ST_FWD_INIT:
		ret = user_fwd_st_init(user);
		break;

	case ST_FWD_REG:
		ret = user_fwd_st_reg(user);
		break;

	case ST_FWD_EST:
		ret = user_fwd_st_est(user);
		break;

	case ST_FWD_CONN:
	case ST_FWD_HB:
		break;

	case ST_FWD_REREG:
		ret = user_fwd_st_rereg(user);
		break;

	default:
		dt_assert(0 && "bug?");
		break;
	}

	return ret;
}
