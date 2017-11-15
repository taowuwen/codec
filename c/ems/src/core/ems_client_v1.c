
#ifndef USE_EMS_SERVER

#include "ems_core.h"
#include "ems_client.h"
#include "app.h"
#include "app_nic.h"
#include "ems_fw.h"
#include "ems_cmd.h"
#include "ems_uci.h"

#define EMS_TIMEOUT_SEND	10000
#define EMS_TIMEOUT_CONNECT	30000


#define ERR_DEVICE_INACTIVE	0x999
#define ERR_CONNECT_DC_FAILED	0x1000
#define ERR_DOWNLOAD_APPCONFIG	0x1001
#define ERR_RESPONSE_ERROR	0x1002

#define ERR_CONNECT_NM_FAILED		0x2000
#define ERR_DOWNLOAD_CONFIG_FAILED	0x2001
#define ERR_SETUPFILE_MISSING		0x2002
#define ERR_INSTALL_FAILED		0x2003

#define SSID_CFG_PREFIX		"ssid_"

static ems_void cl_timeout_cb(ems_session *sess, ems_timeout *to);
static ems_void cl_evt_cb    (ems_session *sess, ems_int st, ems_int flg);

static ems_int cl_start(ems_client *cl, ems_session *sess, ems_uint flg, ems_int err);
static ems_int cl_stopped(ems_client *cl, ems_session *sess, ems_uint flg, ems_int err);
static ems_int cl_err(ems_client *cl, ems_session *sess, ems_uint flg, ems_int err);
static ems_int cl_normal(ems_client *cl, ems_session *sess, ems_uint flg, ems_int err);
static ems_int cl_getdc(ems_client *cl, ems_session *sess, ems_uint flg, ems_int err);
static ems_int cl_getconfig(ems_client *cl, ems_session *sess, ems_uint flg, ems_int err);
static ems_int cl_getupdatefile(ems_client *cl, ems_session *sess, ems_uint flg, ems_int err);
static ems_int cl_updatestatus(ems_client *cl, ems_session *sess, ems_uint flg, ems_int err);
static ems_int cl_connect(ems_client *cl, ems_session *sess, ems_uint flg, ems_int err);

typedef ems_int (*cl_evt_func)(ems_client *cl, ems_session *sess, ems_uint flg, ems_int err);
static cl_evt_func ctrl_handler[] = 
{
	[st_start]   		= cl_start,
	[st_stopped] 		= cl_stopped,
	[st_err]     		= cl_err,
	[st_normal]  		= cl_normal,
	[st_getdc]      	= cl_getdc,
	[st_getconfig] 		= cl_getconfig,
	[st_getupdatefile]	= cl_getupdatefile,
	[st_updatestatus] 	= cl_updatestatus,
	[st_connect]		= cl_connect,
	[st_max]     		= NULL
};

typedef ems_int (*cl_timeout_func)(ems_client *fl, ems_session *sess);

static ems_int cl_to_err(ems_client *cl, ems_session *sess);
static ems_int cl_to_normal(ems_client *cl, ems_session *sess);
static ems_int cl_to_getdc(ems_client *cl, ems_session *sess);
static ems_int cl_to_getconfig(ems_client *cl, ems_session *sess);
static ems_int cl_to_getupdatefile(ems_client *cl, ems_session *sess);
static ems_int cl_to_updatestatus(ems_client *cl, ems_session *sess);
static ems_int cl_to_connect(ems_client *cl, ems_session *sess);

static cl_timeout_func timeout_handler[] =
{
	[st_start]   		= NULL,
	[st_stopped] 		= NULL,
	[st_err]     		= cl_to_err,
	[st_normal]  		= cl_to_normal,
	[st_getdc]      	= cl_to_getdc,
	[st_getconfig] 		= cl_to_getconfig,
	[st_getupdatefile]	= cl_to_getupdatefile,
	[st_updatestatus] 	= cl_to_updatestatus,
	[st_connect]		= cl_to_connect,
	[st_max]     		= NULL
};

typedef ems_int (*cl_evt_rsp_cb)(ems_client *, ems_session *sess, json_object *root);

static json_object *cl_kv_into_json(ems_cchar *ctx)
{
	json_object *obj = NULL;
	ems_buffer   buff;
	ems_cchar    *p, *q;
	ems_char    *buf, *k, *v;
	ems_int      len;

	ems_buffer_init(&buff, EMS_BUFFER_1K);
	obj = json_object_new_object();
	do {
		buf = buf_wr(&buff);
		k = v = NULL;

		do {
			memset(buf, 0, buf_size(&buff));
			q = ctx;
			p = strchr(q, '\n');

			if (!p) {
				ctx += strlen(ctx);
				p = ctx;
			}
			else
				ctx = p + 1;

			len = abs(p - q);
			if (len > buf_size(&buff))
				len = buf_size(&buff) -1;

			memcpy(buf, q, len);

			ems_trim(buf);
			ems_l_info("[clnt]server config info >>> %s", buf);

			k = buf;
			v = strchr(k, '=');

			if (v && strlen(v) > 0 && strlen(k) > 0) {
				*v = '\0';
				v++;

				json_object_object_add(obj, k, json_object_new_string(v));
			}
		} while (*ctx);
	} while (0);
	ems_buffer_uninit(&buff);

	return obj;
}

static ems_int 
grp_json_cfg(json_object *dst, json_object *src, ems_cchar *root, ems_cchar *sub, ems_int ary)
{
	json_object *obj = NULL, *child = NULL;
	ems_cchar    *p;
	ems_int       len;
	ems_int       new = EMS_YES;

	ems_assert(dst && src && root);

	if (ary) {
		if (sub) {
			obj = json_object_object_get(dst, root);
			if (obj)
				new = EMS_NO;
			else
				obj = json_object_new_object();

			child = json_object_new_array();
			json_object_object_add(obj, sub, child);
		} else
			obj = json_object_new_array();
	}
	else {
		if (sub) {
			obj = json_object_object_get(dst, root);
			if (obj)
				new = EMS_NO;
		}

		if (!obj)
			obj = json_object_new_object();
	}

	if (new)
		json_object_object_add(dst, root, obj);
	len = strlen(root);

	json_object_object_foreach(src, key, val) 
	{
		if (strlen(key) < len)
			continue;

		if (!strncmp(key, root, len)) {
			p = key + len + 1;
			if (sub  && !strstr(key, sub))
				continue;

			if (ary) {
				if (child) 
					json_object_array_add(child, json_object_new_string(json_object_get_string(val)));
				else
					json_object_array_add(obj, json_object_new_string(json_object_get_string(val)));
			}
			else
				json_object_object_add(obj, p, json_object_new_string(json_object_get_string(val)));
		}
	}

	return EMS_OK;
}

static ems_int grp_json_client_update(json_object *root)
{
	json_object *req, *obj;

	req = json_object_object_get(root, "client");

	if (!req)
		return EMS_OK;

	ems_json_key_atoi(obj, req, "upt_period");
	ems_json_key_atoi(obj, req, "getconf_period");
	ems_json_key_atoi(obj, req, "retry_period");
	ems_json_key_atoi(obj, req, "enable_subdomain");
	ems_json_key_atoi(obj, req, "autoupdate");

	return EMS_OK;
}

static ems_int grp_json_wireless_update(json_object *root)
{
	json_object *req, *obj;

	req = json_object_object_get(root, "wireless");

	if (!req)
		return EMS_OK;
	
	ems_json_key_atoi(obj, req, "stanum");
	
	return EMS_OK;
}

static ems_int grp_json_system_update(json_object *root)
{
	json_object *req, *obj;

	req = json_object_object_get(root, "system");

	if (!req)
		return EMS_OK;

	ems_json_key_atoi(obj, req, "ems_lan_free");

	return EMS_OK;
}

static ems_int grp_json_network_update(json_object *root)
{
	json_object *req, *obj;

	req = json_object_object_get(root, "network");

	if (!req)
		return EMS_OK;

	ems_json_key_atoi(obj, req, "escape");

	return EMS_OK;
}

static ems_int grp_json_tunnel_update(json_object *root)
{
	json_object *req, *obj;

	req = json_object_object_get(root, "tunnel");

	if (!req)
		return EMS_OK;

	ems_json_key_atoi(obj, req, "enable");
	ems_json_key_atoi(obj, req, "port");
	ems_json_key_atoi(obj, req, "heartbeat");

	return EMS_OK;
}

static json_object *cl_dup_object(json_object *root)
{
	if (root)
		return ems_json_tokener_parse(json_object_to_json_string(root));

	return ems_json_tokener_parse("{}");
}

static ems_int grp_ssid_cfg(json_object *dst, json_object *src)
{
	json_object *ary = NULL, *jobj;
	ems_int      num, i, id;
	ems_char     ssid[32];

	ems_json_key_atoi(jobj, src, "ssid_num");

	ary = json_object_new_array();
	ems_json_get_int_def(src, "ssid_num", num, 0);

	if (num <= 0 || num > 3) {
		ems_l_warn("[clnt] ssid number error: %d", num);
		json_object_put(ary);
		return EMS_ERR;
	}


	for (i = 0; i < num; i++) {
		jobj = json_object_new_object();

		id = i + 1;

		/* iface */
		snprintf(ssid, sizeof(ssid), "ssid_%d_iface", id);
		grp_json_cfg(jobj, src, ssid, NULL, EMS_NO);

		/* portal */
		snprintf(ssid, sizeof(ssid), "ssid_%d_portal", id);
		grp_json_cfg(jobj, src, ssid, NULL, EMS_NO);

		/* radius */
		snprintf(ssid, sizeof(ssid), "ssid_%d_radius", id);
		grp_json_cfg(jobj, src, ssid, NULL, EMS_NO);

		/* bwlist */
		snprintf(ssid, sizeof(ssid), "ssid_%d_bwlist", id);
		grp_json_cfg(jobj, src, ssid, "white", EMS_YES);
		grp_json_cfg(jobj, src, ssid, "black", EMS_YES);

		/* dup http obj here , use global http settings  */
		snprintf(ssid, sizeof(ssid), "ssid_%d_http", id);
#if 1
		json_object_object_add(jobj, ssid, 
				cl_dup_object(json_object_object_get(dst, "http")));
#else
		grp_json_cfg(jobj, src, ssid, "grey", EMS_YES);
		grp_json_cfg(jobj, src, ssid, "port", EMS_YES);
		grp_json_cfg(jobj, src, ssid, "cna",  EMS_YES);
		grp_json_cfg(jobj, src, ssid, "url",  EMS_YES);
		grp_json_cfg(jobj, src, ssid, "nas",  EMS_NO);
#endif

		json_object_array_add(ary, jobj);
	}

	json_object_object_add(dst, "ssid", ary);

	return EMS_OK;
}


struct json_object *grp_all_cfgs(json_object *obj)
{
	json_object *root = NULL;
	root = json_object_new_object();

	grp_json_cfg(root, obj, "client",  NULL,   EMS_NO);
	grp_json_cfg(root, obj, "portal",  NULL,   EMS_NO);
	grp_json_cfg(root, obj, "radius",  NULL,   EMS_NO);
	grp_json_cfg(root, obj, "bwlist", "white", EMS_YES);
	grp_json_cfg(root, obj, "bwlist", "black", EMS_YES);
	grp_json_cfg(root, obj, "wireless",NULL,   EMS_NO);
	grp_json_cfg(root, obj, "system",  NULL,   EMS_NO);
	grp_json_cfg(root, obj, "tunnel",  NULL,   EMS_NO);
	grp_json_cfg(root, obj, "network", NULL,   EMS_NO);

	grp_json_cfg(root, obj, "http",   "grey", EMS_YES);
	grp_json_cfg(root, obj, "http",   "port", EMS_YES);
	grp_json_cfg(root, obj, "http",   "cna",  EMS_YES);
	grp_json_cfg(root, obj, "http",   "url",  EMS_YES);
	grp_json_cfg(root, obj, "http",   "nas",  EMS_NO);

	grp_json_client_update(root);
	grp_json_wireless_update(root);
	grp_json_system_update(root);
	grp_json_tunnel_update(root);
	grp_json_network_update(root);

	/* get ssid cfg */
	grp_ssid_cfg(root, obj);

	return root;
}


static ems_void cl_timeout_cb(ems_session *sess, ems_timeout *to)
{
	ems_client *cl = (ems_client *)sess_cbarg(sess);

	ems_assert(cl->st > st_min && cl->st < st_max);

	ems_assert(timeout_handler[cl->st]);

	if (timeout_handler[cl->st])
		timeout_handler[cl->st](cl, sess);
}

static ems_int 
cl_evt_run(ems_client *cl, ems_session *sess, ems_int flg, ems_int err)
{
	ems_assert(cl && cl->st > st_min && cl->st < st_max);

	ems_assert(ctrl_handler[cl->st]);

	return ctrl_handler[cl->st](cl, sess, flg, err);
}

static ems_void cl_evt_cb(ems_session *sess, ems_int err, ems_int flg)
{
	ems_client *cl = (ems_client *)sess_cbarg(sess);

	ems_assert(cl->st > st_min && cl->st < st_max);
	if (err) {
		ems_l_trace("[clnt] sess: %d %s:%d error(>>> %s <<< )",
				ems_sock_fd(&sess->sock),
				ems_sock_addr(&sess->sock),
				ems_sock_port(&sess->sock), 
				ems_lasterrmsg());
	}

	cl_evt_run(cl, sess, flg, err);
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
		ems_l_trace("[clnt]gethostbyename failed %s : %s", 
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

static ems_int 
cl_connect_and_change_status(ems_client *cl, ems_cchar *srv, ems_int port, ems_status st)
{
	ems_session *sess;

	ems_assert(srv);

	if (!srv)
		return EMS_ERR;

	if (port <= 0)
		port = 80;

	sess = cl->sess;
	ems_assert(sess != NULL);
	if (ems_sock_fd(&sess->sock) > 0) {
		sess_event_cancel(sess);
		ems_l_trace("[clnt]shutdown session(%d) with [%s: %d]", 
				ems_sock_fd(&sess->sock),
				ems_sock_addr(&sess->sock),
				ems_sock_port(&sess->sock));
		ems_sock_close(&sess->sock);
	}

	ems_buffer_clear(&sess->buf);
	ems_buffer_clear(&sess->buf_in);

	ems_sock_setaddr(&sess->sock, srv);
	ems_sock_setport(&sess->sock, port);

	if (cl_do_connect(sess) != EMS_OK)
		return EMS_ERR;

	if (ems_flag_like(sess->flg, EMS_FLG_ONLINE))
		return cl_change_status(cl, st);

	sess_event_set(sess, EMS_EVT_READ|EMS_EVT_WRITE,   cl_evt_cb);
	sess_timeout_set_sorted(sess, EMS_TIMEOUT_CONNECT, cl_timeout_cb);

	cl->next = st;
	return cl_change_status(cl, st_connect);
}


/* for evt handle */
static ems_int cl_start(ems_client *cl, ems_session *sess, ems_uint flg, ems_int err)
{
	if (err) {
		cl->lasterr = ERR_CONNECT_DC_FAILED;
		return cl_change_status(cl, st_err);
	}

	if (!cl->sess) {
		cl->sess = ems_session_new();
		ems_assert(cl->sess != NULL);
		if (!cl->sess)
			return cl_change_status(cl, st_err);

		ems_buffer_increase(&cl->sess->buf,    EMS_BUFFER_4k - EMS_BUFFER_1K);
		ems_buffer_increase(&cl->sess->buf_in, EMS_BUFFER_4k - EMS_BUFFER_1K);
		sess_cbarg_set(cl->sess, cl);
	}

	cl->retry_period = ems_atoi(cfg_get(emscfg(), CFG_client_retry_period));
	if (cl->retry_period <= 0) {
		cl->retry_period = 600;
		cfg_set(emscfg(), CFG_client_retry_period, ems_itoa(cl->retry_period));
	}

	cl->upt_period = ems_atoi(cfg_get(emscfg(), CFG_client_upt_period));
	if (cl->upt_period <= 0) {
		cl->upt_period = 300;
		cfg_set(emscfg(), CFG_client_upt_period, ems_itoa(cl->upt_period));
	}

	cl->getconf_period = ems_atoi(cfg_get(emscfg(), CFG_client_getconf_period));
	if (cl->getconf_period <= 0) {
		cl->getconf_period = 600;
		cfg_set(emscfg(), CFG_client_getconf_period, ems_itoa(cl->getconf_period));
	}

	if (cl->upt_period > cl->getconf_period)
		cl->getconf_period = cl->upt_period;

	if (cl_connect_and_change_status(cl, 
			cfg_get(emscfg(), CFG_ems_s_addr), 
			ems_atoi(cfg_get(emscfg(), CFG_ems_s_port)),
			st_getdc) != EMS_OK) {
		cl->lasterr = ERR_CONNECT_DC_FAILED;
		return cl_change_status(cl, st_err);
	}

	return EMS_OK;
}

static ems_int cl_stopped(ems_client *cl, ems_session *sess, ems_uint flg, ems_int err)
{
	if (cl->sess) {
		ems_session_shutdown_and_destroy(cl->sess);
		cl->sess = NULL;
	}

	ems_flag_unset(cl->flg, EMS_FLG_ONLINE);
	cl->n_conf  = -1;
	cl->n_upt   = -1;

	if (cl->pid) {
		waitpid(cl->pid, NULL, WNOHANG);
		cl->pid = 0;
	}

	return EMS_OK;
}

static ems_int cl_err(ems_client *cl, ems_session *sess, ems_uint flg, ems_int err)
{
	ems_assert(cl->sess != NULL);

	if (err) /* we should do it again, for that conect status could callback here */
		return cl_change_status(cl, st_err);

	sess = cl->sess;

	ems_assert(sess != NULL);
	if (sess) {
		ems_l_trace("[clnt]shutdown session(%d) with [%s: %d]", 
				ems_sock_fd(&sess->sock),
				ems_sock_addr(&sess->sock),
				ems_sock_port(&sess->sock));
		ems_sock_close(&sess->sock);
		sess_event_cancel(sess);

		ems_flag_unset(sess->flg, EMS_FLG_ONLINE);
		sess_timeout_set_sorted(sess, cl->retry_period * 1000, cl_timeout_cb);
	}

	if (cl->pid) {
		waitpid(cl->pid, NULL, WNOHANG);
		cl->pid = 0;
	}

	return EMS_OK;
}

static ems_int cl_normal(ems_client *cl, ems_session *sess, ems_uint flg, ems_int err)
{
	if (err) /* do it again in case that connect server failed */
		return cl_change_status(cl, st_normal);

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
		ems_buffer_clear(&sess->buf);
	}

	return EMS_OK;
}

/*
   1. \r\n\r\n
   2. Content-Length to inc current buffer
 */

static ems_int cl_filter_http_rsp(ems_buffer *buff)
{
	ems_char  *sep, *ct_len;
	ems_char  *rd = buf_rd(buff);
	ems_int    total = 0;

#define HTTP_SEP	"\r\n\r\n"
#define CONTENT_LENGTH	"Content-Length: "

	/* strlen("HTTP") == 4 */
	if (buf_len(buff) <= 4)
		return EMS_CONTINUE;

	if (strncmp("HTTP", rd, 4))
		return EMS_ERR; /* pkg is not http rsp */

	sep = ct_len = NULL;

	ct_len = strstr(rd, CONTENT_LENGTH); 
	if (ct_len) {
		ct_len += strlen(CONTENT_LENGTH);
		total = ems_atoi(ct_len);

		if (total <= 0 || total > EMS_BUFFER_16k ) {
			ems_l_warn("[clnt]Content-Length error  0x%x (0 < x < 0x%x)", 
					total, EMS_BUFFER_16k);
			return EMS_ERR;
		}

		if (buf_size(buff) - total <= 0) {
			ems_l_trace("[clnt]total: 0x%x, left: %d, size:%d, do increase",
					total, buf_left(buff), buf_size(buff));
			ems_buffer_increase(buff, total);
			return EMS_CONTINUE;
		}
	}

	sep = strstr(rd, HTTP_SEP);
	if (sep) {
		/* we reached the content of buffer */
		if (total > 0) {
			if (abs(buf_len(buff) - abs((sep + strlen(HTTP_SEP) - rd))) < total) 
				return EMS_CONTINUE;
		}
		*sep = '\0';
		sep += strlen(HTTP_SEP);

		ems_buffer_seek_rd(buff, abs(sep - rd), EMS_BUFFER_SEEK_CUR);
	}

	if (sep) {
		if (ct_len ) {
			if (buf_len(buff) < total)
				return EMS_CONTINUE;
		}

		ems_buffer_refresh(buff);
		return EMS_OK;
	}

	return EMS_CONTINUE;
}

static ems_int cl_preprocess(ems_client *cl, ems_session *sess, cl_evt_rsp_cb h)
{
	ems_cchar *buf;
	ems_int    len, rtn;
	struct json_tokener *tok;
	struct json_object  *jobj;
	enum   json_tokener_error jerr;

	rtn = cl_filter_http_rsp(&sess->buf_in);
	if (rtn != EMS_OK)
		return rtn;

	tok = json_tokener_new();
	if (!tok)
		return EMS_ERR;

	buf = buf_rd(&sess->buf_in);
	len = buf_len(&sess->buf_in);

	jobj = json_tokener_parse_ex(tok, buf, len);

	if ((jerr = json_tokener_get_error(tok)) == json_tokener_continue) 
	{
		json_tokener_free(tok);
		return EMS_CONTINUE;
	}

	json_tokener_free(tok);

	if (!(jobj && (jerr == json_tokener_success))) {
		ems_l_warn("[clnt]parse err: %s", json_tokener_error_desc(jerr));
		if (jobj)
			json_object_put(jobj);

		cl->lasterr = ERR_RESPONSE_ERROR;
		return EMS_ERR;
	}

	{
		ems_cchar *ctx = jobj?json_object_to_json_string(jobj):"no ctx";
		ems_l_info("[clnt]rsp: %s", strlen(ctx) < 512?ctx:"too long");
	}

	if (h) {
		h(cl, sess, jobj);
		rtn = EMS_OK;
	}
	else
		rtn = EMS_ERR;

	if (jobj)
		json_object_put(jobj);

	return rtn;
}


/* never use this in download files */
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
				return cl_preprocess(cl, sess, h);
		}
	}

	do {
		ret = cl_preprocess(cl, sess, h);

		switch (ret) {
		case EMS_BUFFER_INSUFFICIENT:
		case EMS_ERR:
		case EMS_OK:
			return ret;

		case EMS_CONTINUE:
		default:
			break;
		}
	} while (ret != EMS_CONTINUE);

	if (again)
		goto recv_again;

	return EMS_OK;
}


static ems_int cl_msg_getdc_rsp(ems_client *cl, ems_session *sess, json_object *root)
{
	json_object *result, *rsp, *cfg;

	result = json_object_object_get(root, "result");

	if (!(result && json_object_is_type(result, json_type_object))) 
		goto err_out;

	cfg = json_object_object_get(result, "dcConfig");
	if (!(cfg && json_object_is_type(cfg, json_type_string))) 
		goto err_out;

	rsp = cl_kv_into_json(json_object_get_string(cfg));
	if (rsp) {
		ems_str      tmp;
		str_init(&tmp);

		ems_json_get_string_def(rsp, "nm_addr", &tmp, NULL);
		if (str_len(&tmp) > 0)
			str_cpy(&cl->nm_addr, &tmp);

		ems_json_get_string_def(rsp, "nm_port", &tmp, NULL);
		if (str_len(&tmp) > 0)
			cl->nm_port = ems_atoi(str_text(&tmp));

		str_uninit(&tmp);
		json_object_put(rsp);

		if (str_len(&cl->nm_addr) <= 0) 
			goto err_out;

		if (cl_connect_and_change_status(cl, 
				str_text(&cl->nm_addr), cl->nm_port, st_getconfig) != EMS_OK) {
			cl->lasterr = ERR_CONNECT_NM_FAILED;
			return cl_change_status(cl, st_err);
		}

		return EMS_OK;
	}

err_out:
	cl->lasterr = ERR_RESPONSE_ERROR;
	return cl_change_status(cl, st_err);
}


static ems_int cl_getdc(ems_client *cl, ems_session *sess, ems_uint flg, ems_int err)
{
	if (err)
		goto err_out;

	if (ems_flag_like(flg, EMS_EVT_READ)) {
		if (cl_recv_handle(cl, sess, cl_msg_getdc_rsp) != EMS_OK) 
			goto err_out;
	}

	if (ems_flag_like(flg, EMS_EVT_WRITE)) {
		if (cl_send_msg(cl, sess, flg)  != EMS_OK) 
			goto err_out;
	}

	return EMS_OK;
err_out:
	cl->lasterr = ERR_CONNECT_DC_FAILED;
	return cl_change_status(cl, st_err);
}

static ems_int cl_conf_process(ems_client *cl, json_object *root)
{
	ems_int upt, conf, retry, enable, autoupdate;

	ems_json_get_int_def(root, "upt_period",     upt,   0);
	ems_json_get_int_def(root, "getconf_period", conf,  0);
	ems_json_get_int_def(root, "retry_period",   retry, 0);
	ems_json_get_int_def(root, "autoupdate", autoupdate, 0);
	ems_json_get_int_def(root, "enable_subdomain", enable, 0);

	if (ems_atoi(cfg_get(emscfg(), CFG_client_subdomain_enable)) != enable) {
		if (enable)
			ems_flag_set(emscorer()->flg, FLG_SUBDOMAIN_ENABLE);
		else 
			ems_flag_unset(emscorer()->flg, FLG_SUBDOMAIN_ENABLE);

		cfg_set(emscfg(), CFG_client_subdomain_enable, ems_itoa(enable));
	}

	if (upt > 0) {
		cl->upt_period = upt;
		cfg_set(emscfg(), CFG_client_upt_period, ems_itoa(cl->upt_period));
	}

	if (conf > 0) {
		cl->getconf_period = conf;
		cfg_set(emscfg(), CFG_client_getconf_period, ems_itoa(cl->getconf_period));
	}

	if (retry > 0) {
		cl->retry_period = retry;
		cfg_set(emscfg(), CFG_client_retry_period,   ems_itoa(cl->retry_period));
	}

	cfg_set(emscfg(), CFG_client_autoupdate, ems_itoa(autoupdate));

	if (cl->upt_period > cl->getconf_period)
		cl->getconf_period = cl->upt_period;

	ems_assert(cl->getconf_period > 0 && cl->upt_period > 0);
	cl->getconf = cl->getconf_period;

	return EMS_OK;
}

static ems_int cl_iface_ssid_suffix_upt(json_object *jssid, ems_cchar *new_suffix)
{
	json_object *jobj;
	ems_str      ssid_suffix;
	ems_char     ssid[32];
	ems_char     new_ssid[32];

	str_init(&ssid_suffix);
	do {
		ems_assert(new_suffix && jssid);

		jobj = json_object_object_get(jssid, "ssid");
		if (!jobj) {
			ems_l_trace("[clnt]invalid cfg (find no ssid) : %s", json_object_to_json_string(jssid));
			break;
		}

		snprintf(ssid, sizeof(ssid), "%s", json_object_get_string(jobj));

		ems_json_get_string_def(jssid, "ems_ssid_suffix", &ssid_suffix, NULL);

		ems_l_trace("[clnt] old ssid(%s, %s), new suffix: %s", ssid, str_text(&ssid_suffix), new_suffix);

		if (strlen(new_suffix) == str_len(&ssid_suffix)) {
			if (str_len(&ssid_suffix) <= 0) {
				break;
			}

			if (!strcmp(new_suffix, str_text(&ssid_suffix))) {
				break;
			}
		}

		if (str_len(&ssid_suffix) > 0) {
			ems_char *p;
			p = strstr(ssid, str_text(&ssid_suffix));
			if (p)
				*p = '\0';
		}

		snprintf(new_ssid, sizeof(new_ssid), "%s%s", ssid, new_suffix);
		ems_l_trace("[clnt] new ssid(%s)", new_ssid);

		json_object_object_add(jssid, "ssid", json_object_new_string(new_ssid));
	} while (0);

	str_uninit(&ssid_suffix);

	return EMS_OK;
}

static ems_int cl_portal_cfg_upt(json_object *jssid, json_object *ptl)
{
	ems_str val;

	str_init(&val);

	ems_json_get_string_def(ptl, "enable", &val, NULL);
	if (str_len(&val) > 0) 
		json_object_object_add(jssid, "ems_portal_enable", json_object_new_string(str_text(&val)));

	ems_json_get_string_def(ptl, "addr", &val, NULL);
	if (str_len(&val) > 0) 
		json_object_object_add(jssid, "ems_portal_address", json_object_new_string(str_text(&val)));

	ems_json_get_string_def(ptl, "port", &val, NULL);
	if (str_len(&val) > 0) 
		json_object_object_add(jssid, "ems_portal_port", json_object_new_string(str_text(&val)));

	ems_json_get_string_def(ptl, "redirect_port",&val, NULL);
	if (str_len(&val) > 0) 
		json_object_object_add(jssid, "ems_portal_redirect_port", json_object_new_string(str_text(&val)));

	ems_json_get_string_def(ptl, "reg_period", &val, NULL);
	if (str_len(&val) > 0) 
		json_object_object_add(jssid, "ems_portal_register_period", json_object_new_string(str_text(&val)));

	ems_json_get_string_def(ptl, "hb_period",  &val, NULL);
	if (str_len(&val) > 0) 
		json_object_object_add(jssid, "ems_portal_heartbeat_period",json_object_new_string(str_text(&val)));

	str_uninit(&val);

	return EMS_OK;
}

static ems_int cl_radius_cfg_upt(json_object *jssid, json_object *radius)
{
	ems_str val;

	str_init(&val);

	ems_json_get_string_def(radius, "addr", &val, NULL);
	if (str_len(&val) > 0) 
		json_object_object_add(jssid, "ems_radius_address", json_object_new_string(str_text(&val)));

	ems_json_get_string_def(radius, "secret", &val, NULL);
	if (str_len(&val) > 0) 
		json_object_object_add(jssid, "ems_radius_shared_key", json_object_new_string(str_text(&val)));

	ems_json_get_string_def(radius, "authport", &val, NULL);
	if (str_len(&val) > 0) 
		json_object_object_add(jssid, "ems_radius_auth_port", json_object_new_string(str_text(&val)));

	ems_json_get_string_def(radius, "acctport", &val, NULL);
	if (str_len(&val) > 0) 
		json_object_object_add(jssid, "ems_radius_acct_port", json_object_new_string(str_text(&val)));

	ems_json_get_string_def(radius, "rp_period", &val, NULL);
	if (str_len(&val) > 0) 
		json_object_object_add(jssid, "ems_radius_acct_period", json_object_new_string(str_text(&val)));

	ems_json_get_string_def(radius, "disconnect", &val, NULL);
	if (str_len(&val) > 0) 
		json_object_object_add(jssid, "ems_offline_disconnect", json_object_new_string(str_text(&val)));

	str_uninit(&val);

	return EMS_OK;
}

static ems_int cl_bwlist_cfg_upt(json_object *jssid, json_object *root)
{
	json_object_object_add(jssid, "bwlist", cl_dup_object(root));

	return EMS_OK;
}

static ems_int cl_http_cfg_upt(json_object *jssid, json_object *root)
{
	json_object_object_add(jssid, "http", cl_dup_object(root));

	return EMS_OK;
}

static ems_int cl_iface_ssid_suffix_upt(json_object *jssid, ems_cchar *new_suffix);

static ems_int cl_iface_cfg_upt(json_object *jssid, json_object *root)
{
	ems_str str;

	if (!root)
		return EMS_OK;

	str_init(&str);

	ems_json_get_string_def(root, "ssid_hz", &str, "");

	ems_assert(str_text(&str) != NULL);
	cl_iface_ssid_suffix_upt(jssid, str_text(&str));

	json_object_object_add(jssid, "ems_ssid_suffix", json_object_new_string(str_text(&str)));

	str_uninit(&str);

	return EMS_OK;
}


static ems_int cl_wireless_cfg_rebuild(ems_client *cl, json_object *ary, json_object *root)
{
	json_object *jssid;
	ems_assert(cl && ary && root);

	jssid = ems_uci_find_ssid(ary, UCI_DEFAULT_ID);
	if (!jssid) {
		ems_assert(0 && "never show up this line");
		/* do insert ssid here, not now*/
		ems_l_warn("[clnt] should insert ssid here, env error maybe");
		return EMS_ERR;
	}

	json_object_object_add(jssid, "ems_iface_id", json_object_new_string(ems_itoa(UCI_DEFAULT_ID)));
	json_object_object_add(jssid, "disabled",     json_object_new_string("0"));

	cl_iface_cfg_upt(jssid,  json_object_object_get(root, "wireless"));
	cl_portal_cfg_upt(jssid, json_object_object_get(root, "portal"));
	cl_radius_cfg_upt(jssid, json_object_object_get(root, "radius"));
	cl_bwlist_cfg_upt(jssid, json_object_object_get(root, "bwlist"));
	cl_http_cfg_upt(jssid,   json_object_object_get(root, "http"));

	return EMS_OK;
}

static json_object *ems_uci_create_ssid(ems_int id)
{
	json_object *jssid;
	ems_char ssid[32];

	snprintf(ssid, sizeof(ssid), "%d_unspecified", id);

	jssid = json_object_new_object();
	json_object_object_add(jssid, "device",     json_object_new_string("ra0"));
	json_object_object_add(jssid, "network",    json_object_new_string("lan"));
	json_object_object_add(jssid, "mode",       json_object_new_string("ap"));
	json_object_object_add(jssid, "encryption", json_object_new_string("none"));
	json_object_object_add(jssid, "ssid",       json_object_new_string(ssid));
	json_object_object_add(jssid, "disabled",   json_object_new_string("1"));

	json_object_object_add(jssid, "ems_iface_id", json_object_new_string(ems_itoa(id)));
	json_object_object_add(jssid, "section_type", json_object_new_string("wifi-iface"));

	return jssid;
}

static ems_int 
cl_wireless_cfg_update(ems_client *cl, json_object *cfg, json_object *ary)
{
	ems_int    i, id;
	ems_char   option[64];
	json_object *jssid, *jobj;

	ems_assert(ary && cfg && cl);

	for (i = 0; i < json_object_array_length(ary); i++) {
		id = i + 1;

		jssid = ems_uci_find_ssid(cfg, id);
		if (!jssid) {
			jssid = ems_uci_create_ssid(id);
			if (!jssid)
				continue;

			json_object_array_add(cfg, jssid);
		}

		jobj = json_object_array_get_idx(ary, i);

		/* iface */
		snprintf(option, sizeof(option), "ssid_%d_iface", id);
		cl_iface_cfg_upt(jssid, json_object_object_get(jobj, option));

		/* portal */
		snprintf(option, sizeof(option), "ssid_%d_portal", id);
		cl_portal_cfg_upt(jssid, json_object_object_get(jobj, option));

		snprintf(option, sizeof(option), "ssid_%d_radius", id);
		cl_radius_cfg_upt(jssid, json_object_object_get(jobj, option));

		/* bwlist */
		snprintf(option, sizeof(option), "ssid_%d_bwlist", id);
		cl_bwlist_cfg_upt(jssid, json_object_object_get(jobj, option));

		snprintf(option, sizeof(option), "ssid_%d_http", id);
		cl_http_cfg_upt(jssid, json_object_object_get(jobj, option));
	}

	return EMS_OK;
}

static ems_int cl_cfg_remove_section(json_object *cfg, json_object *serv_cfg)
{
	ems_int i;
	ems_str      sname;
	json_object *jobj;

	ems_assert(json_object_array_length(cfg) > 0);
	ems_assert(json_object_array_length(serv_cfg) > 0);

	str_init(&sname);

	/*
	   wireless cfg is larger than server config
	 */
	for (i = json_object_array_length(cfg) - 1; i > json_object_array_length(serv_cfg); i--) 
	{
		jobj = json_object_array_get_idx(cfg, i - 1);
		ems_json_get_string_def(jobj, UCI_SECTION_NAME, &sname, NULL);

		if (str_len(&sname) > 0) {
			ems_uci_remove_section(UCI_CFG_WIRELESS, str_text(&sname));
		}
	}

	str_uninit(&sname);

	return EMS_OK;
}

static ems_int cl_conf_process_system(ems_client *cl, json_object *root)
{
	ems_int   lan_free;

	ems_json_get_int(root, "ems_lan_free", lan_free);

	ems_systemcmd("uci set ykwifi.base.ems_lan_free=%d;" 
		      "uci commit ykwifi.base.ems_lan_free", lan_free);

	/* no need to send EMS_APP_EVT_WIRED_RELOAD to nic module to notify wired reload msg */

	return EMS_OK;
}


static ems_int cl_conf_process_network(ems_client *cl, json_object *root)
{
	ems_int escape, evt;

	ems_json_get_int(root, "escape", escape);
	cfg_set(emscfg(), CFG_network_detect_escape, ems_itoa(escape));

	evt = EMS_APP_START;
	if (escape)
		evt = EMS_APP_STOP;

	ems_send_message(ty_client, ty_net, evt, NULL);

	return EMS_OK;
}

static ems_int cl_conf_process_wireless(ems_client *cl, json_object *root)
{
	ems_int  maxstanum;

	ems_json_get_int_def(root, "stanum", maxstanum,  0);
	if (maxstanum > 0 && maxstanum <= 32) {
		ems_systemcmd("iwpriv ra0 set MaxStaNum=%d;"
			      "uci set wireless.ra0.maxassoc=%d;"
			      "uci commit wireless", maxstanum, maxstanum);
	}
	
	return EMS_OK;
}


static ems_int cl_process_rules(ems_client *cl, json_object *root)
{
	json_object *obj, *ary;

	if (ems_flag_like(emscorer()->flg, FLG_NETWORK_BRIDGE)) goto over;

	obj = json_object_object_get(root, "system");
	if (obj)
		cl_conf_process_system(cl, obj);

	/* we need to check if config continue those sections */
	ary = json_object_new_array();
	ems_uci_load_cfg(UCI_CFG_WIRELESS, NULL, ary);

	obj = json_object_object_get(root, "ssid");
	if (!obj) {
		cl_wireless_cfg_rebuild(cl, ary, root);
	}  else {
		cl_wireless_cfg_update( cl, ary, obj);
	}

	ems_uci_write_cfg(UCI_CFG_WIRELESS, NULL, ary);

	if (obj && json_object_array_length(obj) < json_object_array_length(ary))
	{
		cl_cfg_remove_section(ary, obj);
	}

	ems_systemcmd("/sbin/wifiset");
	ems_send_message(ty_client, ty_nic, EMS_APP_SERVER_RULES_UPDATE, ary);
	json_object_put(ary);

	goto over;
over:
	obj = json_object_object_get(root, "wireless");
	if (obj)
		cl_conf_process_wireless(cl, obj);

	obj = json_object_object_get(root, "client");
	if (obj)
		cl_conf_process(cl, obj);

	obj = json_object_object_get(root, "tunnel");
	if (obj)
		ems_app_process(ty_client, ty_tunnel, EMS_APP_SERVER_RULES_UPDATE, obj);

	obj = json_object_object_get(root, "network");
	if (obj)
		cl_conf_process_network(cl, obj);

	cfg_write(emscfg());

	return EMS_OK;
}


static ems_int cl_getconfig_rsp(ems_client *cl, ems_session *sess, json_object *root)
{
	int64_t      conf_n;
	json_object *result, *cfg, *rsp, *ary;

	result = json_object_object_get(root, "result");
	if (!(result && json_object_is_type(result, json_type_object))) {
		cl->lasterr = ERR_DOWNLOAD_CONFIG_FAILED;
		goto next_st;
	}

	ary = json_object_object_get(result, "config");
	if (!(ary && json_object_is_type(ary, json_type_array))) {
		cl->lasterr = ERR_DOWNLOAD_CONFIG_FAILED;
		goto next_st;
	}

	if (json_object_array_length(ary) <= 0) {
		cl->lasterr = ERR_DOWNLOAD_CONFIG_FAILED;
		goto next_st;
	}

	result = json_object_array_get_idx(ary, 0);
	if (!(result && json_object_is_type(result, json_type_object))) {
		cl->lasterr = ERR_DOWNLOAD_CONFIG_FAILED;
		goto next_st;
	}

	ems_json_get_int64_def(result, "configNumber", conf_n, -1);
	if (conf_n == cl->n_conf) {
		ems_l_trace("[clnt]config not change: %ld", conf_n);
		goto next_st;
	}

	cfg = json_object_object_get(result, "config");
	if (!(cfg && json_object_is_type(cfg, json_type_string))) {
		cl->lasterr = ERR_DOWNLOAD_CONFIG_FAILED;
		goto next_st;
	}

	cl->n_conf = conf_n;

	rsp = cl_kv_into_json(json_object_get_string(cfg));
	if (rsp) {
		ems_flag_set(cl->flg, EMS_FLG_ONLINE);

		cfg = grp_all_cfgs(rsp);
		json_object_put(rsp);

		cl_process_rules(cl, cfg);
		json_object_put(cfg);
		cl->lasterr = 0;
	} else
		cl->lasterr = ERR_RESPONSE_ERROR;

next_st: 
	if (cl_connect_and_change_status(cl, 
		str_text(&cl->nm_addr), cl->nm_port, st_getupdatefile) != EMS_OK) {
		cl->lasterr = ERR_CONNECT_NM_FAILED;
		return cl_change_status(cl, st_err);
	}

	return EMS_OK;
}

static ems_int 
cl_getconfig(ems_client *cl, ems_session *sess, ems_uint flg, ems_int err)
{
	if (err) 
		goto next_st;

	if (ems_flag_like(flg, EMS_EVT_READ)) {
		if (cl_recv_handle(cl, sess, cl_getconfig_rsp) != EMS_OK) 
			goto next_st;
	}

	if (ems_flag_like(flg, EMS_EVT_WRITE)) {
		if (cl_send_msg(cl, sess, flg)  != EMS_OK) 
			goto next_st;
	}

	return EMS_OK;

next_st:
	cl->lasterr = ERR_CONNECT_NM_FAILED;

	if (cl_connect_and_change_status(cl, 
		str_text(&cl->nm_addr), cl->nm_port, st_getupdatefile) != EMS_OK) {
		cl->lasterr = ERR_CONNECT_NM_FAILED;
		return cl_change_status(cl, st_err);
	}

	return EMS_OK;
}


static ems_int 
cl_file_need_update(ems_cchar *name, ems_cchar *ty, ems_cchar *ver, ems_cchar *url)
{
	ems_cchar *val = NULL;
	if (!strcmp(ty, "client")) {
		if (strcmp(ver, ems_popen_get("cat /tmp/ems/conf/ver")))
			return EMS_YES;

	} else if (!strcmp(ty, "rom")) {
		val = core_sysver();
		ems_l_info("[clnt]update rom: (%s %s)", ty, val);
		if (strcmp(ver, val))
			return EMS_YES;
	} else if (!strcmp(ty, "app")) {
		/* check local verion */
		ems_l_trace("[clnt]app update, not support for now");
		return EMS_NO;
	} 
	
	return EMS_NO;
}

#include <sys/types.h>
#include <sys/stat.h>

static ems_void 
cl_installer(ems_cchar *cmd, ems_cchar *name, ems_cchar *ty, ems_cchar *ver, ems_cchar *url)
{
#ifndef DEBUG
	ems_int fd;
#endif
	ems_char *argv[10];

	setsid();
	umask(0);

#ifndef DEBUG
	fd = open("/dev/null", O_RDWR);
	if (fd > 0) {
		dup2(fd, STDIN_FILENO);
		dup2(fd, STDOUT_FILENO);
		close(fd);
	}
#endif

	argv[0] = (ems_char *)cmd;
	argv[1] = (ems_char *)name;
	argv[2] = (ems_char *)ty;
	argv[3] = (ems_char *)ver;
	argv[4] = (ems_char *)url;
	argv[5] = NULL;

	if (execv(cmd, argv) == -1)
		ems_l_trace("[clnt]exec cmd: %s failed: %s", cmd, ems_lasterrmsg());

	exit(0);
}

static ems_int
cl_file_do_update_one(ems_client *cl, ems_cchar *name, ems_cchar *ty, ems_cchar *ver, ems_cchar *url)
{
	ems_processid pid;
	struct  stat  st;
	static ems_cchar   *setup = "/tmp/ems/bin/app_setup.sh";

	if (stat(setup, &st)) {
		cl->lasterr = ERR_SETUPFILE_MISSING;
		ems_l_trace("[clnt]warning setup file missing: %s", setup);
		return EMS_ERR;
	}

	if (!(st.st_mode & S_IXUSR))
		chmod(setup, st.st_mode | S_IXUSR);

	pid = fork();

	switch (pid) {

	case -1:
		ems_l_trace("[clnt]fork() failed: %s", ems_lasterrmsg());
		return EMS_ERR;

	case 0:
		cl_installer(setup, name, ty, ver, url);
		break;
	}

	cl->pid = pid;

	return EMS_OK;
}

static ems_int cl_check_and_update(ems_client *cl, json_object *ary)
{
	json_object *obj;
	json_object *ty, *name, *ver, *url;
	ems_int     i;
	ems_cchar   *f_ty, *f_name, *f_ver,*f_url;

	ems_assert(ary && json_object_is_type(ary, json_type_array));

	for (i = 0; i < json_object_array_length(ary); i++) {
		obj = json_object_array_get_idx(ary, i);

		if (obj && json_object_is_type(obj, json_type_object)) 
		{
			ems_l_info("[clnt]file: %s", json_object_to_json_string(obj));

			ty   = json_object_object_get(obj, "fileType");
			name = json_object_object_get(obj, "fileName");
			ver  = json_object_object_get(obj, "fileVer");
			url  = json_object_object_get(obj, "fileUrl");

			if (!(ty && name && ver && url)) 
				continue;

			if (! (json_object_is_type(ty, json_type_string) &&
				json_object_is_type(name, json_type_string) &&
				json_object_is_type(ver, json_type_string) &&
				json_object_is_type(url, json_type_string))) {
				continue;
			}

			f_ty   = json_object_get_string(ty);
			f_name = json_object_get_string(name);
			f_ver  = json_object_get_string(ver);
			f_url  = json_object_get_string(url);

			if (cl_file_need_update(f_name, f_ty, f_ver, f_url)) {
				/* for now, only one file could update , and we could
				   reset fileUpdateNumber to update versions.
				 */
				cl_file_do_update_one(cl, f_name, f_ty, f_ver, f_url);

				if (strcmp(f_ty, "app")) /* not app == rom or client */
					return EMS_OK;
			}
		}
	}

	return EMS_OK;
}

static ems_int cl_getupdatefile_rsp(ems_client *cl, ems_session *sess, json_object *root)
{
	json_object *result, *obj, *ary;
	ems_long     upt_n;

	result = json_object_object_get(root, "result");
	if (!(result && json_object_is_type(result, json_type_object)))
		goto next_st;

	ary = json_object_object_get(result, "updateFile");
	if (!(ary && json_object_is_type(ary, json_type_array))) {
		ems_l_trace("[clnt]updateFile  missing.");
		goto next_st;
	}

	if (json_object_array_length(ary) <= 0) {
		ems_l_trace("[clnt]update array length null.");
		goto next_st;
	}

	result = json_object_array_get_idx(ary, 0);
	if (!(result && json_object_is_type(result, json_type_object))) {
		ems_l_trace("[clnt]index array missing.");
		goto next_st;
	}

	ems_json_get_int64_def(result, "updateFileNumber", upt_n, -1);
	if (upt_n == cl->n_upt) {
		ems_l_trace("[clnt]update file number not change: %ld", upt_n);
		ems_int  st;
		if (cl->pid > 0) {
			switch(waitpid(cl->pid, &st, WNOHANG)) {
			case 0:
				ems_l_trace("[clnt]app install finished, and success, pid:%ld", cl->pid);
				cl->pid = 0;
				break;

			default:
				if (WIFEXITED(st)) {
					ems_l_trace("[clnt]install failed process: %ld exited, status=%d, reset updateFileNumber", 
							cl->pid, WEXITSTATUS(st));
				} else if (WIFSIGNALED(st)) {
					ems_l_trace("[clnt]install failed process: %ld killed by signal=%d reset updateFileNumber",
							cl->pid, WTERMSIG(st));
				}

				cl->lasterr = ERR_INSTALL_FAILED;
				cl->n_upt   = -1;

				if (WIFEXITED(st) || WIFSIGNALED(st))
					cl->pid = 0;

				break;
			}
		}

		goto next_st;
	}

	obj = json_object_object_get(result, "files");
	if (!(obj && json_object_is_type(obj, json_type_array))) {
		ems_l_trace("[clnt]files missing");
		goto next_st;
	}

	if (ems_atoi(cfg_get(emscfg(), CFG_client_autoupdate))) {
		if (cl_check_and_update(cl, obj) == EMS_OK)
			cl->n_upt = upt_n;
	} else {
		ems_l_info("[clnt] autoupdate disabled, wanna autoupdate? set client.autoupdate=1");
		cl->n_upt = upt_n;
	}

next_st:
	return cl_change_status(cl, st_normal);
}

static ems_int 
cl_getupdatefile(ems_client *cl, ems_session *sess, ems_uint flg, ems_int err)
{
	if (ems_flag_like(flg, EMS_EVT_READ)) {
		if (cl_recv_handle(cl, sess, cl_getupdatefile_rsp) != EMS_OK) 
		{
			cl->lasterr = ERR_CONNECT_NM_FAILED;
			return cl_change_status(cl, st_err);
		}
	}

	if (ems_flag_like(flg, EMS_EVT_WRITE)) {
		if (cl_send_msg(cl, sess, flg)  != EMS_OK) {
			cl->lasterr = ERR_CONNECT_NM_FAILED;
			return cl_change_status(cl, st_err);
		}
	}

	return EMS_OK;
}

static ems_int 
cl_updatestatus(ems_client *cl, ems_session *sess, ems_uint flg, ems_int err)
{
	if (err) {
		cl->lasterr = ERR_CONNECT_NM_FAILED;
		goto err_out;
	}

	if (ems_flag_like(flg, EMS_EVT_WRITE)) {
		if (cl_send_msg(cl, sess, flg)  != EMS_OK)  {
			cl->lasterr = ERR_CONNECT_NM_FAILED;
			goto err_out;
		}

		if (buf_len(&sess->buf) <= 0) {
			cl->lasterr = 0;
			goto err_out;
		}
	}

	return EMS_OK;

err_out:
	return cl_change_status(cl, st_normal);
}

static ems_int cl_connect(ems_client *cl, ems_session *sess, ems_uint flg, ems_int _err)
{
	ems_int    err;
	socklen_t len;

	if (_err)
		goto err_out;

	len = sizeof(err);
	err = 0;
	getsockopt(ems_sock_fd(&sess->sock), SOL_SOCKET, SO_ERROR, (ems_char *)&err, &len);

	if (err) {
		errno = err;
		ems_l_trace("[clnt]sess(%d) connect to %s:%d failed, %s",
				ems_sock_fd(&sess->sock),
				ems_sock_addr(&sess->sock),
				ems_sock_port(&sess->sock),
				ems_geterrmsg(err));
		goto err_out;
	}

	ems_l_trace("[clnt]sess(%d) established with %s:%d",
			ems_sock_fd(&sess->sock),
			ems_sock_addr(&sess->sock),
			ems_sock_port(&sess->sock));

	ems_flag_set(sess->flg, EMS_FLG_ONLINE);

	sess_event_cancel(sess);
	return cl_change_status(cl, cl->next);

err_out:
	ems_assert(ctrl_handler[cl->last] != NULL);
	return ctrl_handler[cl->last](cl, sess, 0, err);
}


/* for timeout handle */
static ems_int cl_to_err(ems_client *cl, ems_session *sess)
{
	ems_l_trace("[clnt] time up for retry");
 
	cl->next = st_stopped;

	ems_assert(ems_flag_unlike(cl->flg, EMS_FLG_ONLINE));
	if (ems_flag_unlike(cl->flg, EMS_FLG_ONLINE))
		return cl_change_status(cl, st_start);

	if (cl_connect_and_change_status(cl, 
		str_text(&cl->nm_addr), cl->nm_port, st_getconfig) != EMS_OK) {
		cl->lasterr = ERR_CONNECT_NM_FAILED;
		return cl_change_status(cl, st_err);
	}

	return EMS_OK;
}

static ems_int cl_to_normal(ems_client *cl, ems_session *sess)
{
	ems_status st;

	st = st_updatestatus;

	ems_l_trace("[clnt] timer trigger (getconf: %d, upt_period: %d)", cl->getconf, cl->upt_period);

	if (cl->getconf <= 0) {
		cl->getconf = cl->getconf_period; /* reset timer */
		st = st_getconfig;
	} else 
		cl->getconf -= cl->upt_period;

	if (cl_connect_and_change_status(cl, 
		str_text(&cl->nm_addr), cl->nm_port, st) != EMS_OK) {
		cl->lasterr = ERR_CONNECT_NM_FAILED;
		return cl_change_status(cl, st_err);
	}

	return EMS_OK;
}

static ems_int cl_to_getdc(ems_client *cl, ems_session *sess)
{
	ems_l_trace("[clnt] getdc timeout(%d) with [%s: %d]", 
			ems_sock_fd(&sess->sock),
			ems_sock_addr(&sess->sock),
			ems_sock_port(&sess->sock));

	cl->lasterr = ERR_CONNECT_DC_FAILED;
	return cl_change_status(cl, st_err);
}

static ems_int cl_to_getconfig(ems_client *cl, ems_session *sess)
{
	ems_l_trace("[clnt] getconfig timeout (%d) with [%s: %d]", 
			ems_sock_fd(&sess->sock),
			ems_sock_addr(&sess->sock),
			ems_sock_port(&sess->sock));

	return cl_evt_run(cl, sess, 0, EMS_YES);
}

static ems_int cl_to_getupdatefile(ems_client *cl, ems_session *sess)
{
	ems_l_trace("[clnt]getupdatefile timeout(%d) with [%s: %d]", 
			ems_sock_fd(&sess->sock),
			ems_sock_addr(&sess->sock),
			ems_sock_port(&sess->sock));

	return cl_evt_run(cl, sess, 0, EMS_YES);
}

static ems_int cl_to_updatestatus(ems_client *cl, ems_session *sess)
{
	ems_l_trace("[clnt]updatestatus timeout(%d) with [%s: %d]", 
			ems_sock_fd(&sess->sock),
			ems_sock_addr(&sess->sock),
			ems_sock_port(&sess->sock));

	return cl_evt_run(cl, sess, 0, EMS_YES);
}

static ems_int cl_to_connect(ems_client *cl, ems_session *sess)
{
	ems_l_trace("[clnt]timeout connect (%d) with [%s: %d]", 
			ems_sock_fd(&sess->sock),
			ems_sock_addr(&sess->sock),
			ems_sock_port(&sess->sock));

	return cl_evt_run(cl, sess, 0, EMS_YES);
}

static ems_int 
cl_fill_request(ems_session *sess, ems_cchar *path, ems_cchar *method, json_object *param, ems_uint id)
{
	ems_int      len;
	json_object *req;
	ems_cchar   *ctx;

	ems_assert(sess && path && method && "never show up this line");

	req = json_object_new_object();

	json_object_object_add(req, "jsonrpc", json_object_new_string("2.0"));
	json_object_object_add(req, "method",  json_object_new_string(method));

	if (!param) {
		param = json_object_new_object();

		json_object_object_add(param, "sn",      json_object_new_string(core_sn()));
		json_object_object_add(param, "devType", json_object_new_string(core_devicetype()));
		json_object_object_add(param, "softVer", json_object_new_string(core_sysver()));
	}

	ems_assert(json_object_is_type(param, json_type_object));

	json_object_object_add(req, "params", param);
	json_object_object_add(req, "id",    json_object_new_int(id));

	ctx = json_object_to_json_string(req);
	if (!ctx)
		ctx = "";

	ems_l_info("[clnt]request(%s): %s", method, strlen(ctx)<512?ctx:"too long");

	len = snprintf(buf_wr(&sess->buf), buf_left(&sess->buf),
		"POST /%s?ak=%s HTTP/1.0\r\n"
		"User-Agent: yingkewifi\r\n"
		"Host: %s\r\n"
		"Connection: close\r\n"
		"Accept: */*\r\n"
		"Content-Length: %d\r\n"
		"\r\n"
		"%s\r\n\r\n", 
		path, core_sn(), ems_sock_addr(&sess->sock), (ems_int)strlen(ctx), ctx);

	json_object_put(req);
	ems_buffer_seek_wr(&sess->buf, len, EMS_BUFFER_SEEK_CUR);

	return EMS_OK;
}


static ems_int cl_status_getdc(ems_client *cl)
{
	ems_session *sess = cl->sess;

	ems_assert(cl && sess && "never show up this line");

	cl_fill_request(sess, "inm_nas_dc_v1", "getDC", NULL, 1);

	sess_event_set(sess,   EMS_EVT_WRITE, 	 cl_evt_cb);
	sess_timeout_set_sorted(sess, EMS_TIMEOUT_SEND, cl_timeout_cb);

	return EMS_OK;
}

static ems_int cl_status_getconfig(ems_client *cl)
{
	json_object *param;
	json_object *ary, *obj;
	ems_session *sess = cl->sess;

	ems_assert(cl && sess && "never show up this line");

	param = json_object_new_object();
	ary   = json_object_new_array();
	obj   = json_object_new_object();

	json_object_object_add(obj, "configNumber", json_object_new_int64(cl->n_conf));
	json_object_object_add(obj, "sn",      json_object_new_string(core_sn()));
	json_object_object_add(obj, "devType", json_object_new_string(core_devicetype()));
	json_object_object_add(obj, "softVer", json_object_new_string(core_sysver()));

	json_object_array_add(ary, obj);
	json_object_object_add(param, "nasInfos", ary);

	cl_fill_request(sess, "inm_nas_v1", "getConf", param, 3);

	sess_event_set(sess,   EMS_EVT_WRITE, 	 cl_evt_cb);
	sess_timeout_set_sorted(sess, EMS_TIMEOUT_SEND, cl_timeout_cb);

	return EMS_OK;
}

static ems_int cl_status_normal(ems_client *cl)
{
	ems_int      tm;
	ems_session *sess = cl->sess;

	ems_assert(cl && sess && "never show up this line");

	sess_event_cancel(sess);
	ems_sock_close(&sess->sock);
	ems_l_trace("[clnt]shutdown session(%d) with [%s: %d]", 
			ems_sock_fd(&sess->sock),
			ems_sock_addr(&sess->sock),
			ems_sock_port(&sess->sock));

	tm = cl->upt_period;
	if (cl->getconf < tm)
		tm = cl->getconf <= 0? 0: cl->getconf;

	sess_timeout_set_sorted(sess, tm * 1000, cl_timeout_cb);

	return EMS_OK;
}

static ems_int cl_status_getupdatefile(ems_client *cl)
{
	json_object *param;
	json_object *ary, *obj;
	ems_session *sess = cl->sess;

	ems_assert(cl && sess && "never show up this line");

	param = json_object_new_object();
	ary   = json_object_new_array();
	obj   = json_object_new_object();

	json_object_object_add(obj, "updateFileNumber", json_object_new_int64(cl->n_upt));
	json_object_object_add(obj, "sn", json_object_new_string(core_sn()));
	json_object_object_add(obj, "devType", json_object_new_string(core_devicetype()));
	json_object_object_add(obj, "softVer", json_object_new_string(core_sysver()));

	json_object_array_add(ary, obj);
	json_object_object_add(param, "nasInfos", ary);

	cl_fill_request(sess, "inm_nas_v1", "getUpdateFile", param, 4);

	sess_event_set(sess,   EMS_EVT_WRITE, 	 cl_evt_cb);
	sess_timeout_set_sorted(sess, EMS_TIMEOUT_SEND, cl_timeout_cb);

	return EMS_OK;
}

static ems_int ems_get_ifname_throughput(ems_cchar *ifname, off_t *in, off_t *out)
{
	ems_char buf[256] = {0};

	snprintf(buf, sizeof(buf), "%s", 
		ems_popen_get("cat /proc/net/dev | grep %s | awk '{printf(\"%%s %%s\", $2, $10)}'", ifname));

	if (strlen(buf) > 0) {
#ifndef GENERIC_LINUX
		sscanf(buf, "%lld %lld", in, out);
#else
		sscanf(buf, "%ld %ld", in, out);
#endif

		return EMS_OK;
	}

	return EMS_ERR;
}

static ems_int ems_diskusage()
{
	return ems_atoi(ems_popen_get("df | awk '/rootfs/ {gsub(\"%%\", \"\", $5); print $5}'"));
}

static ems_int cl_status_updatestatus(ems_client *cl)
{
	json_object *param, *obj, *ary;
	ems_session *sess = cl->sess;
	off_t in, out;
	ems_int st, st_portal, st_radius;

	ems_assert(cl && sess && "never show up this line");

	in = out = 0; 
	ems_get_ifname_throughput(str_text(&emscorer()->net.wan.ifname), &in, &out);

	param = json_object_new_object();
	ary   = json_object_new_array();
	obj   = json_object_new_object();

	json_object_object_add(obj, "sn", json_object_new_string(core_sn()));
	json_object_object_add(obj, "devType", json_object_new_string(core_devicetype()));
	json_object_object_add(obj, "softVer", json_object_new_string(core_sysver()));

	json_object_object_add(obj, "updateFileNumber", json_object_new_int64(cl->n_upt));
	json_object_object_add(obj, "configNumber", json_object_new_int64(cl->n_conf));

	json_object_object_add(obj, "cpu",  json_object_new_int(ems_cpuusage()));
	json_object_object_add(obj, "mem",  json_object_new_int(ems_memusage()));
	json_object_object_add(obj, "disk", json_object_new_int(ems_diskusage()));

	json_object_object_add(obj, "inOctets",  json_object_new_int64(in));
	json_object_object_add(obj, "outOctets", json_object_new_int64(out));
	json_object_object_add(obj, "userCount", json_object_new_int(ems_send_message(ty_client, ty_nic, EMS_APP_EVT_USERNUMBER, NULL)));

	st_portal = 0;
//	ems_app_process(ty_client, ty_portal, EMS_APP_EVT_STATUS, NULL);
	st_radius = 0;
//	ems_app_process(ty_client, ty_radius, EMS_APP_EVT_STATUS, NULL);
	st = st_portal | st_radius;

	json_object_object_add(obj, "status", json_object_new_int(st?1:0));
	json_object_object_add(obj, "portalStatus", json_object_new_int(st_portal ? 1:0));
	json_object_object_add(obj, "radiusStatus", json_object_new_int(st_radius ? 1:0));
	json_object_object_add(obj, "statusDes", json_object_new_string(""));

	json_object_object_add(obj, "apNum",  json_object_new_int(0));

	{
		if (core_ac_mac()) {
			json_object *bssid_ary;

			json_object_object_add(obj, "mac",  json_object_new_string(core_ac_mac()));

			bssid_ary = json_object_new_array();

			ems_app_process(ty_client, ty_nic, EMS_APP_EVT_BSSIDINFO, bssid_ary);
			json_object_object_add(obj, "bssList", bssid_ary);
		}

		if (core_wan_addr()) 
			json_object_object_add(obj, "manageIp",json_object_new_string(core_wan_addr()));
	}

	json_object_array_add(ary, obj);
	json_object_object_add(param, "status", ary);

	cl_fill_request(sess, "inm_nas_v1", "updateStatus", param, 5);

	sess_event_set(sess,   EMS_EVT_WRITE, 	 cl_evt_cb);
	sess_timeout_set_sorted(sess, EMS_TIMEOUT_SEND, cl_timeout_cb);

	return EMS_OK;
}

ems_int cl_change_status(ems_client *cl, ems_status st)
{
	ems_l_trace("[clnt] change status: %s ---> %s",
			ems_status_str(cl->st), ems_status_str(st));

	cl->last = cl->st;
	cl->st   = st;

	switch(cl->st) {
	case st_init:
	case st_stopped:
	case st_err:
		return cl_evt_run(cl, NULL, 0, EMS_NO);

	case st_getdc:
		return cl_status_getdc(cl);

	case st_getconfig:
		return cl_status_getconfig(cl);

	case st_normal:
		return cl_status_normal(cl);

	case st_getupdatefile:
		return cl_status_getupdatefile(cl);

	case st_updatestatus:
		return cl_status_updatestatus(cl);

	default:
		break;
	}

	return EMS_OK;
}


#if (EMS_LOGGER_FILE || DEBUG)
ems_cchar *ems_status_str(ems_status st)
{
	switch(st) {
	case st_init:
		return "init/start";
	case st_normal:
		return "normal";
	case st_hb:
		return "hb/updatestatus";
	case st_reg:
		return "reg/getdc";
	case st_applist:
		return "applist";
	case st_download:
		return "download";
	case st_install:
		return "install/apply";
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

	case st_getconfig:
		return "getconfig";

	case st_getupdatefile:
		return "getupdatefile";

	default:
		break;
	}

	return "unknown";
}
#endif

#endif
