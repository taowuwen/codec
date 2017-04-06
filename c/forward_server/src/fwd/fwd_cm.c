
#include "fwd_main.h"
#include "fwd_sock.h"
#include "fwd_buffer.h"
#include "fwd_cm.h"
#include "json.h"

typedef dt_int (*cm_func_st)(fwd_cm *cm, dt_int flgs);
typedef dt_int (*cm_func_timeout)(fwd_cm *);
typedef dt_int (*cm_func_content_cb)(dt_char *buf, dt_int len, dt_cchar *arg);

static dt_void cm_event_cb(fwd_event_fd *evt, dt_int flgs);
static dt_void cm_timeout_cb(fwd_timeout *to);
static  dt_int fwd_cm_connect(fwd_cm *cm);

static dt_int cm_status(fwd_cm *cm);
static dt_int cm_st_init(fwd_cm *cm, dt_int flgs);
static dt_int cm_st_est(fwd_cm *cm, dt_int flgs);
static dt_int cm_st_err(fwd_cm *cm, dt_int flgs);
static dt_int cm_st_stop(fwd_cm *cm, dt_int flgs);
static dt_int cm_st_max(fwd_cm *cm, dt_int flgs);

static dt_int cm_timeout_est(fwd_cm *cm);
static dt_int cm_timeout_err(fwd_cm *cm);

static dt_int cm_ticket_rsp(dt_char *buf, dt_int len, dt_cchar *arg);


static cm_func_st cm_evt_handler[] = {
	[ST_CM_INIT] = cm_st_init,
	[ST_CM_EST]  = cm_st_est,
	[ST_CM_ERR]  = cm_st_err,
	[ST_CM_STOP] = cm_st_stop,
	[ST_CM_MAX]  = cm_st_max
};

static cm_func_timeout cm_timeout_handler[] = {
	[ST_CM_INIT] = NULL,
	[ST_CM_EST]  = cm_timeout_est,
	[ST_CM_ERR]  = cm_timeout_err,
	[ST_CM_STOP] = NULL,
	[ST_CM_MAX]  = NULL
};


#define cm_event_run(cm, flgs)    cm_evt_handler[cm_status(cm)](cm, flgs)

#define cm_timeout_run(cm)        do {					\
		if (cm_timeout_handler[cm_status(cm)])			\
			cm_timeout_handler[cm_status(cm)](cm);		\
} while (0)

static dt_cchar *cm_statusstr(fwd_cm_status st)
{
	switch(st) {
	case ST_CM_INIT:
		return "INIT";

	case ST_CM_EST:
		return "ESTABLISHED";

	case ST_CM_ERR:
		return "ERROR";
	
	case ST_CM_STOP:
		return "STOP";

	case ST_CM_MAX:
		return "SHOULD NOT BE HERE";

	case ST_CM_MIN:
		return "UNKNOWN";
	}

	return "*** never show up this --- bugs ***";
}


static dt_int cm_changestatus(fwd_cm *cm, fwd_cm_status st)
{
	log_trace("[cm]change status: %s -> %s",
		cm_statusstr(cm->st), cm_statusstr(st));
	return cm->st = st;
}

static dt_int 
cm_set_reginfo(dt_char *buf, dt_int len, dt_cchar *arg)
{
	json_object *obj;

	obj = json_object_new_object();

	json_object_object_add(obj, "fwd_addr", json_object_new_string(fwd_main_bind_addr()));
	json_object_object_add(obj, "fwd_port", json_object_new_int(fwd_main_bind_port()));
	json_object_object_add(obj, "force_takeover", json_object_new_int(0));

	snprintf(buf, len, "%s", json_object_to_json_string(obj));

	json_object_put(obj);

	return FWD_OK;
}

static dt_int 
cm_send_req(fwd_cm *cm, dt_uint tag, cm_func_content_cb fillinfo, dt_cchar *arg)
{
	fwd_buffer_pack(&cm->buf_out, (dt_int)tag, 0, fillinfo, arg);
	return fwd_main_event_set(&cm->event,
			      FWD_EVT_WRITE | FWD_EVT_READ | FWD_EVT_EDGE_TRIGGER,
			      cm_event_cb);
}

static dt_int 
cm_send_rsp(fwd_cm *cm, dt_uint tag, cm_func_content_cb fillinfo, dt_int st, dt_cchar *arg)
{
	fwd_buffer_pack(&cm->buf_out, tag, st, fillinfo, arg);
	return fwd_main_event_set(&cm->event,
			      FWD_EVT_WRITE | FWD_EVT_READ | FWD_EVT_EDGE_TRIGGER,
			      cm_event_cb);
}

static dt_int cm_st_init(fwd_cm *cm, dt_int flgs)
{
	dt_int    err;
	socklen_t len;

	len = sizeof(err);
	err = 0;
	getsockopt(fwd_sock_fd(&cm->sock), SOL_SOCKET, SO_ERROR, (dt_char *)&err, &len);

	if (err) {
		errno = err;
		log_err("[cm]connect failed to %s:%d: %s",
				fwd_sock_addr(&cm->sock),
				fwd_sock_port(&cm->sock),
				fwd_getlasterrstr(err));

		cm_changestatus(cm, ST_CM_ERR);
		return cm_event_run(cm, 0);
	} else {
		log_info("[cm] established with CM server : %s:%d, session: %d",
				fwd_sock_addr(&cm->sock),
				fwd_sock_port(&cm->sock),
				fwd_sock_fd(&cm->sock)
			);
		cm_send_req(cm, MSG_CM_REG, cm_set_reginfo, NULL);
		cm_changestatus(cm, ST_CM_EST);
		return cm_event_run(cm, flgs);
	}

	return FWD_OK;
}

static dt_int msg_ticket_parse(dt_cchar *buf, dt_str *ticket)
{
	struct json_object *root, *obj;

	root = json_tokener_parse(buf);

	if (is_error(root)) {
		log_warn("[cm] error parsing json: %s: %s", buf, 
			json_tokener_errors[-(unsigned long)root]);
		return FWD_ERR_INVALID_ARG;
	}

	log_trace("[cm]ticket json: %s", json_object_to_json_string(root));

	obj = json_object_object_get(root, "ticket");
	if (obj)
		str_set(ticket, json_object_get_string(obj));

	json_object_put(root);

	return FWD_OK;
}

/*
{"ticket": "string"}
*/
static dt_int
cm_msg_ticket_new(fwd_cm *cm, fwd_response *rsp, dt_cchar *val)
{
	dt_int  ret;
	dt_str  ticket;

	ret = FWD_OK;
	str_init(&ticket);
	do {
		if (msg_ticket_parse(val, &ticket) != FWD_OK) {
			ret = FWD_ERR_INVALID_ARG;
			break;
		}

		log_trace("[cm]ticket: %s", str_text(&ticket)?str_text(&ticket): "NULL");

		if (str_text(&ticket) == NULL ) {
			ret = FWD_ERR_INVALID_ARG;
			break;
		}

		ret = fwd_main_ticket_in(str_text(&ticket));
	} while (0);


	if (str_text(&ticket))
		ret = cm_send_rsp(cm, MSG_CM_TICKET_NEW, cm_ticket_rsp, ret, str_text(&ticket));
	else
		ret = cm_send_rsp(cm, MSG_CM_TICKET_NEW, NULL, ret, NULL);

	str_uninit(&ticket);

	return ret;
}


static dt_int cm_handle_msg(fwd_cm *cm)
{
	fwd_response  rsp;
	dt_char      *val;
	dt_int        ret;

	val = NULL;

	ret = fwd_buffer_unpack(&cm->buf_in, (fwd_request *)&rsp, NULL, &val);

	if (ret == FWD_OK) {
		fwd_buffer_refresh(&cm->buf_in);

		switch(rsp.cmd) {

		case MSG_CM_REG:
		case MSG_CM_HEARTBEAT:
		case MSG_CM_TICKET_FIRED:
			break;

		case MSG_CM_TICKET_NEW:
			cm_msg_ticket_new(cm, &rsp, val);
			break;

		default:
			dt_assert(0 && "bug");
			break;
		}
	}

	if (val)
		dt_free(val);

	return ret;
}

static dt_int cm_st_est(fwd_cm *cm, dt_int flgs)
{
	dt_int ret;

	if (flgs & FWD_EVT_WRITE) {
		ret = fwd_sock_send(&cm->sock, &cm->buf_out);

		if (ret == FWD_ERR) {
			cm_changestatus(cm, ST_CM_ERR);
			return cm_event_run(cm, 0);
		}

		if (buf_len(&cm->buf_out) == 0) {
			log_trace("[cm]session [%d] finished write: %d bytes, event ***>>>> READ",
							fwd_sock_fd(&cm->sock), ret);
			fwd_main_event_set(&cm->event,
					FWD_EVT_READ | FWD_EVT_EDGE_TRIGGER,
					cm_event_cb);

			fwd_buffer_refresh(&cm->buf_out);
		}
		/*
		did not finished send, try next event
		*/
		return FWD_OK;
	}

	if (flgs & FWD_EVT_READ) {
		dt_int  retry;
again:
		ret = fwd_sock_read(&cm->sock, &cm->buf_in);

		if (ret <= 0 ) {
			if (ret == -EAGAIN)
				return FWD_OK;
			cm_changestatus(cm, ST_CM_ERR);
			return cm_event_run(cm, 0);
		}

		retry = YES;

		if (buf_left(&cm->buf_in) > 0)
			retry = NO;

		while (buf_len(&cm->buf_in) >= sizeof(fwd_request)) {
			if (cm_handle_msg(cm) != FWD_OK)
				break;
		}

		if (retry)
			goto again;
	}

	return FWD_OK;
}

static dt_int cm_st_err(fwd_cm *cm, dt_int flgs)
{
	log_warn("[cm] disconnected with CM server(%d)", fwd_sock_fd(&cm->sock));

	fwd_main_event_cancel(&cm->event);
	fwd_sock_close(&cm->sock);
	fwd_buffer_clear(&cm->buf_in);
	fwd_buffer_clear(&cm->buf_out);
	
	fwd_main_timeout_set(&cm->timeout,
				FWD_TIMEOUT_RETRY,
				cm_timeout_cb,
				FWD_TIMEOUT_TAIL);

	return 0;
}

static dt_int cm_st_stop(fwd_cm *cm, dt_int flgs)
{
	log_info("[cm] stopping ....");

	fwd_sock_close(&cm->sock);
	fwd_buffer_clear(&cm->buf_in);
	fwd_buffer_clear(&cm->buf_out);
	fwd_main_event_cancel(&cm->event);
	fwd_main_timeout_cancel(&cm->timeout);

	log_info("[cm] stopped ....");

	return FWD_OK;
}

static dt_int cm_st_max(fwd_cm *cm, dt_int flgs)
{
	dt_assert(0 && "bugs");
	return FWD_ERR;
}

static dt_void
cm_event_cb(fwd_event_fd *evt, dt_int flgs)
{
	fwd_cm *cm = dt_container_of(evt, fwd_cm, event);

	log_trace("[cm][%d] got event: flags: 0x%x, error? %s",
					evt->fd,
					flgs, evt->error?"yes":"no");
	if (evt->error)
		cm_changestatus(cm, ST_CM_ERR);

	cm_event_run(cm, flgs);
}


static dt_int cm_timeout_est(fwd_cm *cm)
{
	cm_send_req(cm, MSG_CM_HEARTBEAT, NULL, NULL);

	fwd_main_timeout_set(&cm->timeout,
				FWD_TIMEOUT_HEARTBEAT,
				cm_timeout_cb,
				FWD_TIMEOUT_TAIL);

	return FWD_OK;
}

static dt_int cm_timeout_err(fwd_cm *cm)
{
	log_warn("[cm]retry to connect to : %s:%d",
			fwd_sock_addr(&cm->sock), fwd_sock_port(&cm->sock));

	if (fwd_cm_connect(cm) != FWD_OK) {
		fwd_main_timeout_set(&cm->timeout,
					FWD_TIMEOUT_RETRY,
					cm_timeout_cb,
					FWD_TIMEOUT_TAIL);
		return FWD_ERR;
	}

	return FWD_OK;
}

static dt_void
cm_timeout_cb(fwd_timeout *to)
{
	fwd_cm *cm = dt_container_of(to, fwd_cm, timeout);

	cm_timeout_run(cm);
}

static dt_int cm_status(fwd_cm *cm)
{
	if ((cm->st > ST_CM_MIN) && (cm->st <= ST_CM_MAX))
		return cm->st;

	return ST_CM_MAX;
}


static dt_int 
fwd_cm_setevent(fwd_cm *cm)
{
	cm->event.fd = fwd_sock_fd(&cm->sock);
	fwd_main_event_set(&cm->event,
			      FWD_EVT_WRITE|FWD_EVT_READ|FWD_EVT_EDGE_TRIGGER,
			      cm_event_cb);

	fwd_main_timeout_set(&cm->timeout,
				FWD_TIMEOUT_HEARTBEAT,
				cm_timeout_cb,
				FWD_TIMEOUT_TAIL);

	return FWD_OK;
}

dt_int fwd_cm_init(fwd_cm *cm)
{
	dt_assert(cm != NULL);

	memset(cm, 0, sizeof(fwd_cm));

	fwd_sock_init(  &cm->sock);
	fwd_buffer_init(&cm->buf_in,  FWD_BUFFER_4k);
	fwd_buffer_init(&cm->buf_out, FWD_BUFFER_4k);

	return FWD_OK;
}

dt_int fwd_cm_uninit(fwd_cm *cm)
{
	if (cm) {
		fwd_cm_stop(cm);
		fwd_buffer_uninit(&cm->buf_in);
		fwd_buffer_uninit(&cm->buf_out);
		fwd_sock_clear(   &cm->sock);
		return FWD_OK;
	}

	return FWD_ERR;
}

static  dt_int fwd_cm_connect(fwd_cm *cm)
{
	dt_int             ret;
	dt_int             fd;
	socklen_t          len;
	struct sockaddr_in addr;

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd <= 0)
		return FWD_ERR;
	
	memset(&addr, 0, sizeof(addr));
	if (dt_gethostbyname(fwd_sock_addr(&cm->sock), &addr) != OK) {
		log_info("[cm]get host: %s failed: %s", 
				fwd_sock_addr(&cm->sock), fwd_lasterrstr());
		close(fd);
		return FWD_ERR;
	}

	addr.sin_family = AF_INET;
	addr.sin_port   = htons(fwd_sock_port(&cm->sock));

	dt_setnonblocking(fd, YES);
	len = sizeof(struct sockaddr_in);
	ret = connect(fd, (struct sockaddr *)&addr, len);

	switch(ret) {

	case 0:
		cm_send_req(cm, MSG_CM_REG, cm_set_reginfo, NULL);
		cm_changestatus(cm, ST_CM_EST);
		break;

	case -1:
		ret = fwd_lasterr();

		switch(ret) {
			
		case EINPROGRESS:
			cm_changestatus(cm, ST_CM_INIT);
			break;
		default:
			cm_changestatus(cm, ST_CM_ERR);
			close(fd);
			log_err("[cm]connect failed: %s", fwd_getlasterrstr(ret));
			return FWD_ERR;
		}
		break;
	}

	fwd_sock_setfd(&cm->sock, fd);

	return fwd_cm_setevent(cm);
}

dt_int fwd_cm_start(fwd_cm *cm)
{
	dt_assert(cm != NULL);
	dt_assert(fwd_strlen(fwd_sock_addr(&cm->sock)) > 0 && fwd_sock_port(&cm->sock) > 0);

	cm->st = ST_CM_MIN;
	if (fwd_cm_connect(cm) != FWD_OK) {
		cm->st = ST_CM_STOP;
		return FWD_ERR;	
	}

	return FWD_OK;
}

dt_int fwd_cm_stop(fwd_cm *cm)
{
	cm->st = ST_CM_STOP;

	return cm_event_run(cm, 0);
}

static dt_int
cm_ticket_rsp(dt_char *buf, dt_int len, dt_cchar *arg)
{
	json_object *obj;

	obj = json_object_new_object();

	json_object_object_add(obj, "ticket", json_object_new_string(arg));

	if (strlen(json_object_to_json_string(obj)) > len -1) {
		json_object_put(obj);
		return FWD_ERR;
	}

	snprintf(buf, len, "%s", json_object_to_json_string(obj));
	json_object_put(obj);

	return FWD_OK;
}

dt_int fwd_cm_ticket_fired(fwd_cm *cm, dt_cchar *ticket)
{
	return cm_send_req(cm, MSG_CM_TICKET_FIRED, cm_ticket_rsp, ticket);
}
