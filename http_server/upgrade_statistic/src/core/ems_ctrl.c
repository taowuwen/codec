
#include "ems_core.h"
#include "ems_client.h"
#include "ems_cmd.h"
#include "ems_radius.h"
#include "app.h"
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
extern ems_int ems_cmd_bridge_req(ems_core *core, ems_session *sess, json_object *req);
extern ems_int ems_cmd_bridge_hb(ems_core *core, ems_session *sess, json_object *req);
extern ems_int ems_cmd_bwlist(ems_core *core, ems_session *sess, json_object *req);

extern ems_int ems_cmd_qos(ems_core *core, ems_session *sess, json_object *req);
extern ems_int ems_cmd_portal(ems_core *core, ems_session *sess, json_object *req);
extern ems_int ems_cmd_radius(ems_core *core, ems_session *sess, json_object *req);

extern ems_int ems_cmd_fw(ems_core *core, ems_session *sess, json_object *req);
extern ems_int ems_cmd_user(ems_core *core, ems_session *sess, json_object *req);
extern ems_int ems_cmd_wireless(ems_core *core, ems_session *sess, json_object *req);

extern ems_int ems_cmd_test_radius(ems_core *core, ems_session *sess, json_object *req);
extern ems_int ems_cmd_network(ems_core *core, ems_session *sess, json_object *req);
extern ems_int ems_cmd_config(ems_core *core, ems_session *sess, json_object *req);
#ifdef EMS_LOGGER_FILE
extern ems_int ems_cmd_log(ems_core *core, ems_session *sess, json_object *req);
#endif

extern ems_int ems_cmd_staticstic(ems_core *core, ems_session *sess, json_object *req);
extern ems_int ems_cmd_totalinfo(ems_core *core, ems_session *sess, json_object *req);

static ems_int 
ctrl_cmd_handle(ems_ctrl *ctrl, ems_session *sess, json_object *req, ems_uint msgid)
{
	ems_l_trace("<< \033[01;33m req: %s \033[00m>>",
			req?json_object_to_json_string(req): "no arg");

	switch(msgid & 0x0000ffff) {

	case CMD_EMS_C:
		return ems_cmd_c(emscorer(), sess, req);

	case CMD_EMS_CTRL:
		return ems_cmd_ctrl(emscorer(), sess, req);

	case CMD_EMS_STATUS:
		return ems_cmd_status(emscorer(), sess, req);

	case CMD_EMS_BWLIST:
		return ems_cmd_bwlist(emscorer(), sess, req);

	case CMD_EMS_QOS:
		return ems_cmd_qos(emscorer(), sess, req);

	case CMD_EMS_PORTAL:
		return ems_cmd_portal(emscorer(), sess, req);

	case CMD_EMS_RADIUS:
		return ems_cmd_radius(emscorer(), sess, req);

	case CMD_BRIDGE_REQ:
		return ems_cmd_bridge_req(emscorer(), sess, req);

	case CMD_BRIDGE_HB:
		return ems_cmd_bridge_hb(emscorer(), sess, req);

	case CMD_EMS_FW:
		return ems_cmd_fw(emscorer(), sess, req);

	case CMD_EMS_USER:
		return ems_cmd_user(emscorer(), sess, req);

	case CMD_EMS_WIRELESS:
		return ems_cmd_wireless(emscorer(), sess, req);

	/* for test radius */
	case CMD_EMS_TEST_RADIUS:
		return ems_cmd_test_radius(emscorer(), sess, req);

	case CMD_EMS_NETWORK:
		return ems_cmd_network(emscorer(), sess, req);

	case CMD_EMS_CONFIG:
		return ems_cmd_config(emscorer(), sess, req);

#ifdef EMS_LOGGER_FILE
	case CMD_EMS_LOG:
		return ems_cmd_log(emscorer(), sess, req);
#endif

	case CMD_UPDATESTATUS:
	case CMD_GET_DC:
	case CMD_GET_CONF:
	case CMD_GET_UPDATEFILE:
	case CMD_DOWNLOAD:
		return ems_app_process(ty_ctrl, ty_statistic, msgid & 0x0000ffff, req);

	case CMD_TOTAL_INFO:
		return ems_cmd_totalinfo(emscorer(), sess, req);

	case CMD_STATICSTIC:
		return ems_cmd_staticstic(emscorer(), sess, req);

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


#define HTTP_NEWLINE	"\r\n"
#define HTTP_GET	"GET"
#define HTTP_POST	"POST"
#define HTTP_HOST  	"Host: "

static ems_int ctrl_msg_is_web(ems_session *sess)
{
	if (!strncmp(buf_rd(&sess->buf_in), HTTP_GET,  3))
		return EMS_YES;

	if (!strncmp(buf_rd(&sess->buf_in), HTTP_POST, 4))
		return EMS_YES;

	return EMS_NO;
}

static ems_cchar *ems_current_tm()
{
	static ems_char  tm_buf[128] = {0};
	time_t	tm;

	time(&tm);
	snprintf(tm_buf, sizeof(tm_buf), "%s", ctime(&tm));
	tm_buf[strlen(tm_buf) -1] = '\0';

	return tm_buf;
}

static ems_int ctrl_web_redirect_to_homepage(ems_ctrl *ctrl, ems_session *sess)
{
	ems_int    len;
	ems_buffer *buf = core_buffer();

	len = snprintf(buf_wr(buf), buf_left(buf),
		"HTTP/1.0 302 Moved Temporarily\r\n"
		"Server: yingkewifi\r\n"
		"Connection: close\r\n"
		"Date: %s GMT\r\n"
		"Location: http://%s/cgi-bin/luci/\r\n"
		"Content-Type: text/plain\r\n\r\n"
		"<html></html>\r\n"
		, ems_current_tm(), core_gw_addr());

	ems_buffer_write(&sess->buf, buf_rd(buf), len);

	sess_event_set(sess,   EMS_EVT_READ | EMS_EVT_WRITE,   evt_cmd_cb);
	sess_timeout_set_sorted(sess, EMS_CMD_TIMEOUT, timeout_cmd_cb);

	ems_flag_set(sess->flg, SESSION_FLAG_DIE_AFTER_SEND);

	return EMS_OK;
}

static ems_int ctrl_web_redirect_to_mgmt(ems_ctrl *ctrl, ems_session *sess)
{
	ems_int    len;
	ems_buffer *buf = core_buffer();

	len = snprintf(buf_wr(buf), buf_left(buf),
		"HTTP/1.0 302 Moved Temporarily\r\n"
		"Server: yingkewifi\r\n"
		"Connection: close\r\n"
		"Date: %s GMT\r\n"
		"Location: http://%s/cgi-bin/luci/link_fail\r\n"
		"Content-Type: text/plain\r\n\r\n"
		"<html></html>\r\n"
		, ems_current_tm(), core_gw_addr());

	ems_buffer_write(&sess->buf, buf_rd(buf), len);

	sess_event_set(sess,   EMS_EVT_READ | EMS_EVT_WRITE,   evt_cmd_cb);
	sess_timeout_set_sorted(sess, EMS_CMD_TIMEOUT, timeout_cmd_cb);

	ems_flag_set(sess->flg, SESSION_FLAG_DIE_AFTER_SEND);

	return EMS_OK;
}

#ifndef SKIP_ERROR
static ems_int ctrl_web_redirect_to_error(ems_ctrl *ctrl, ems_session *sess, ems_buffer *buf)
{
	ems_int    len;

	len = snprintf(buf_wr(buf), buf_left(buf),
		"HTTP/1.0 302 Moved Temporarily\r\n"
		"Server: yingkewifi\r\n"
		"Connection: close\r\n"
		"Date: %s GMT\r\n"
		"Location: http://%s/cgi-bin/luci/guide_fail\r\n"
		"Content-Type: text/plain\r\n\r\n"
		"<html></html>\r\n"
		, ems_current_tm(), core_gw_addr());

	ems_buffer_seek_wr(buf, len, EMS_BUFFER_SEEK_CUR);

	return EMS_OK;
}
#endif

static ems_int 
ctrl_web_redirect_to_same_page(ems_ctrl *ctrl, ems_session *sess, ems_buffer *buf, ems_cchar *srcurl)
{
	ems_int    len;

	len = snprintf(buf_wr(buf), buf_left(buf),
		"HTTP/1.0 302 Moved Temporarily\r\n"
		"Server: yingkewifi\r\n"
		"Connection: close\r\n"
		"Date: %s GMT\r\n"
		"Location: %s\r\n"
		"Content-Type: text/plain\r\n\r\n"
		"<html></html>\r\n"
		, ems_current_tm(), srcurl?srcurl:"no");

	ems_buffer_seek_wr(buf, len, EMS_BUFFER_SEEK_CUR);

	return EMS_OK;
}

ems_char *ems_mac_update(ems_char *dst, ems_cchar *src)
{
	while (*src) {
		*dst = *src++;

		if (*dst == ':')
			*dst = '-';
		dst++;
	}

	return dst;
}

ems_char *ems_usermac(ems_cchar *ip)
{
	static ems_char cmd[128];
	static ems_char buf[32];

	snprintf(cmd, 128, "/sbin/arp -n | grep -v incomplete | grep '(%s)' | awk '{print $4}'", ip);
	snprintf(buf, 32, "%s", ems_popen_get(cmd));

	return buf;
}

ems_int ems_urlencode(ems_buffer *dst, ems_buffer *src)
{
	ems_char *buf = NULL;
	ems_int   len  = 0, ret;

	do {
		ret = EMS_ERR;

		buf = url_encode(buf_rd(src), buf_len(src));

		if (!buf) break;

		len = ems_strlen(buf);

		ems_l_trace("url encode: %s, buf_left(%d), len: %d", buf, buf_left(dst), len);

		if (len >= buf_left(dst)) break;

		ems_buffer_write(dst, buf, len);
		{
			ems_char *ch = buf_wr(dst);
			*ch = '\0';
		}

		ret = EMS_OK;
	} while (0);

	if (buf)
		ems_free(buf);

	return ret;
}

static ems_int 
ems_getparams(ems_session *sess, ems_buffer *param)
{
	ems_int     len;
	ems_buffer *buf = &sess->buf_in;
	ems_char   *p, *q, *ender;

	p = buf_rd(buf);

	if (!strncmp(p, HTTP_GET, 3)) {
		p += 4;
	} else if (!strncmp(p, HTTP_POST, 4)) {
		p += 5;
	} else {
		ems_assert(0 && "never show up this line");
		return EMS_ERR;
	}

	ender = strstr(p, HTTP_NEWLINE);
	if (!ender) {
		ems_l_trace("header: %s", p);
		return EMS_ERR;
	}

	*ender = '\0';
	ender += 2;
	ems_buffer_seek_rd(buf, abs(ender - buf_rd(buf)), EMS_BUFFER_SEEK_CUR);

	q = strchr(p, ' ');
	if (!q)
		return EMS_ERR;
	*q = '\0';

	len = abs(p - q);
	if (len > buf_left(param)) {
		ems_l_trace("param too long(need %d > %d left)", len, buf_left(param));
		return EMS_ERR;
	}

	len = snprintf(buf_wr(param), buf_left(param), "%s", p);
	ems_buffer_seek_wr(param, len, EMS_BUFFER_SEEK_CUR);

	ems_l_trace("param: %s", p);

	return EMS_OK;
}

ems_int ems_gethost(ems_session *sess, ems_buffer *host, ems_int *permit)
{
	ems_int     len = 0;
	ems_buffer *buf = &sess->buf_in;
	ems_char   *p, *ender;

	p = strstr(buf_rd(buf), HTTP_HOST);
	if (!p)
		return EMS_ERR;

	p += strlen(HTTP_HOST);

	ender = strstr(p, HTTP_NEWLINE);
	if (!ender)
		return EMS_ERR;

	*ender = '\0';
	ender += 2;
	ems_buffer_seek_rd(buf, abs(ender - buf_rd(buf)), EMS_BUFFER_SEEK_CUR);

	len = snprintf(buf_wr(host), buf_left(host), "%s", p);
	ems_buffer_seek_wr(host, len, EMS_BUFFER_SEEK_CUR);

	if (ems_flag_like(emscorer()->flg, FLG_SUBDOMAIN_ENABLE)){
		json_object *obj;
		ems_char    *comma;

		comma = strchr(p, ':');
		if (comma)
			*comma = '\0';

		obj = json_object_new_object();
		json_object_object_add(obj, "url", json_object_new_string(p));

		if (ems_app_process(ty_ctrl, ty_fw, EMS_APP_CHECK_SUBDOMAIN, obj) == EMS_YES) 
			*permit = EMS_YES;

		json_object_put(obj);
	}
	ems_l_trace("host: %s", buf_rd(host));

	return EMS_OK;
}

static ems_cchar *
ems_srcurl(ems_session *sess, ems_buffer *param, ems_buffer *host, ems_int *permit)
{
	ems_int     rtn;

	do {
		rtn = ems_getparams(sess, param);
		if (rtn != OK) {
			ems_l_trace("(%s)params too long", ems_sock_addr(&sess->sock));
			break;
		}

#define url_header	"http://"
		ems_buffer_write(host, url_header, strlen(url_header));
		rtn = ems_gethost(sess, host, permit);
		if (rtn != OK) {
			ems_l_trace("(%s)did not get hosts", ems_sock_addr(&sess->sock));
			break;
		}

		/* check param's goto apple.com */
		/* 7 == strlen("http://") */
		if (  !*permit && 
			ems_flag_like(emscorer()->flg, FLG_SUBDOMAIN_ENABLE) && 
			inet_addr(buf_rd(host) + 7) != INADDR_NONE
			)
		{
			json_object *obj;

			/* for speed up */
			static ems_char  arg[80] = {0};
			snprintf(arg, sizeof(arg), "%s", buf_rd(param));

			obj = json_object_new_object();
			json_object_object_add(obj, "arg", json_object_new_string(arg));
			json_object_object_add(obj, "ip",  json_object_new_string(buf_rd(host) + 7));

			if (ems_app_process(ty_ctrl, ty_fw, EMS_APP_CHECK_PARAM_APPLE_COM, obj) == EMS_YES) 
				*permit = EMS_YES;

			json_object_put(obj);
		}

		if (buf_left(host) -1 < buf_len(param)) {
			ems_l_trace("(%s) params too long", ems_sock_addr(&sess->sock));
			rtn = EMS_ERR;
			break;
		}

		ems_buffer_write(host, buf_rd(param), buf_len(param));
		{
			ems_char *ch = buf_wr(host);
			*ch = '\0';
		}

		ems_l_trace("url(%d): %s", buf_len(host), buf_rd(host));
		ems_buffer_clear(&sess->buf_in);

		if (*permit) {
			ems_buffer_write(&sess->buf_in, buf_rd(host), buf_len(host));
			{
				ems_char *ch = buf_wr(&sess->buf_in);
				*ch = '\0';
			}
		} else {
			if (ems_urlencode(&sess->buf_in, host) != EMS_OK) {
				ems_l_trace("url too long");
				rtn = EMS_ERR;
				break;
			}
		}

		ems_l_trace("srcurl: %s", buf_rd(&sess->buf_in));

		rtn = EMS_OK;

	} while (0);

	if (rtn != OK)
		return NULL;

	return buf_rd(&sess->buf_in);
}

static ems_int 
ctrl_web_portal_fill_with(ems_ctrl *ctrl, ems_session *sess, ems_buffer *buf, ems_cchar *date, ems_cchar *srcurl)
{
	ems_int    rtn;
	static ems_char wlanusermac[32], wlanapmac[32];
	ems_buffer arg;

	ems_cchar *wlanuserip  = ems_sock_addr(&sess->sock);

	ems_mac_update(wlanusermac, ems_usermac(wlanuserip));
	ems_mac_update(wlanapmac,   core_ac_mac());

	ems_buffer_init(&arg, EMS_BUFFER_2K);
	memset(buf_wr(&arg), 0, buf_left(&arg));

	rtn = snprintf(buf_wr(&arg), buf_left(&arg), 
			"wlanuserip=%s&"
			"wlanacname=%s&"
			"ssid=%s&"
			"wlanusermac=%s&"
			"wlanapmac=%s&"
			"devicetype=%s&"
			"apsn=%s",
			wlanuserip, core_sn(), core_ssid(), wlanusermac, wlanapmac, core_devicetype(), core_sn());

	ems_buffer_seek_wr(&arg, rtn, EMS_BUFFER_SEEK_CUR);
	if (srcurl) {
		ems_buffer_write(&arg, "&srcurl=", strlen("&srcurl="));
		ems_buffer_write(&arg, srcurl, strlen(srcurl));
	}
	
#ifdef WEIXIN
	ems_char *tmpbuf = url_encode(buf_rd(&arg), buf_len(&arg));
	if (tmpbuf) {
		ems_buffer_write(&arg, "&auth=", strlen("&auth="));
		ems_buffer_write(&arg, tmpbuf, strlen(tmpbuf));
		ems_free(tmpbuf);
	}
#endif

	ems_l_trace("args: %s", buf_rd(&arg));

	rtn = snprintf(buf_wr(buf), buf_left(buf),
		"HTTP/1.0 302 Found\r\n"
		"Date: %s GMT\r\n"
		"Server: yingkewifi\r\n"
		"Connection: close\r\n"
		"Location: http://%s:%d/portal/Login.do?%s\r\n"
		"Content-Type: text/plain;charset=utf-8\r\n"
		"\r\n"
		"<html></html>\r\n"
		, date,
		core_portal_addr(),
		core_portal_redirect_port(),
		buf_rd(&arg));

	rtn = rtn < buf_left(buf) ? rtn: buf_left(buf);
	ems_buffer_uninit(&arg);

	ems_buffer_seek_wr(buf, rtn, EMS_BUFFER_SEEK_CUR);

	return EMS_OK;
}

static ems_cchar *ctrl_web_callback(ems_buffer *buf)
{
	ems_char *p, *q;

#define NAS_CALLBACK	"callback="

	q = NULL;

	p = strstr(buf_rd(buf), NAS_CALLBACK);
	if (p) {
		q = p + strlen(NAS_CALLBACK);

		p = strchr(q, '&');
		if (p)
			*p = '\0';
	}

	return q;
}

static ems_int 
ctrl_web_nasinfo_fill_with(ems_ctrl *ctrl, ems_session *sess, ems_buffer *buf, ems_cchar *date, ems_cchar *func)
{
	ems_int      res;
	json_object *rsp, *obj;
	ems_cchar   *ctx = NULL;
	ems_cchar   *wlanuserip = ems_sock_addr(&sess->sock);
	ems_cchar   *username   = ems_app_radius_username(wlanuserip);

	rsp = json_object_new_object();
	obj = json_object_new_object(); 

	res = 0;
	if (!username) {
		res      = 1;
		username = "";
	}

	json_object_object_add(rsp, "result", json_object_new_int(res));
	{
		static ems_char wlanusermac[32], wlanapmac[32];

		ems_mac_update(wlanusermac, ems_usermac(wlanuserip));
		ems_mac_update(wlanapmac,   core_ac_mac());

		json_object_object_add(obj, "portalip",   json_object_new_string(core_portal_addr()));
		json_object_object_add(obj, "username",   json_object_new_string(username));
		json_object_object_add(obj, "wlanusermac",json_object_new_string(wlanusermac));
		json_object_object_add(obj, "wlanuserip", json_object_new_string(wlanuserip));
		json_object_object_add(obj, "ssid",       json_object_new_string(core_ssid()));
		json_object_object_add(obj, "wlanapmac",  json_object_new_string(wlanapmac));
		json_object_object_add(obj, "wlanacmac",  json_object_new_string(wlanapmac));
		json_object_object_add(obj, "wlanacname", json_object_new_string(core_sn()));
		json_object_object_add(obj, "devicetype", json_object_new_string(core_devicetype()));
		json_object_object_add(obj, "sn",         json_object_new_string(core_sn()));
		json_object_object_add(obj, "acip",       json_object_new_string(core_gw_addr()));
	}
	json_object_object_add(rsp, "data",   obj);

	ctx = json_object_to_json_string(rsp);

	if (!ctx)
		ctx = "";

	if (!func)
		func = "afunc";

	ems_l_trace("nasgetinfo: %s", ctx);

	res = snprintf(buf_wr(buf), buf_left(buf),
		"HTTP/1.0 200 OK\r\n"
		"Date: %s GMT\r\n"
		"Server: yingkewifi\r\n"
		"Connection: close\r\n"
		"Content-Length: %d\r\n"
		"Content-Type: text/plain;charset=utf-8\r\n"
		"\r\n"
		"%s(%s);"
		, 
		date, (ems_int)(strlen(ctx) + strlen(func) + 3), func, ctx);

	ems_buffer_seek_wr(buf, res, EMS_BUFFER_SEEK_CUR);

	json_object_put(rsp);

	return EMS_OK;
}

static ems_int ctrl_web_redirect_to_portal(ems_ctrl *ctrl, ems_session *sess)
{
	ems_buffer param, host;
	ems_cchar  *srcurl = NULL;
	ems_buffer *buf = core_buffer();
	ems_int    len  = 0;
	ems_int    permit = EMS_NO;

	ems_buffer_init(&param,  EMS_BUFFER_1K);
	ems_buffer_init(&host,   EMS_BUFFER_1K);

	memset(buf_wr(&param), 0, buf_size(&param));
	memset(buf_wr(&host),  0, buf_size(&host));

	srcurl = ems_srcurl(sess, &param, &host, &permit);

	/* check whether request is NASGetInfo */
	do {
		/* check params */
#define PORTAL_PARAMS "/portal/?"
		len = 9; /* 9 == strlen(PORTAL_PARAMS); */
		if (buf_len(&param) <= len) break;
		if (strncmp(buf_rd(&param), PORTAL_PARAMS, len)) break;

		/* check host */
		len = strlen(NAS_INFO_URL);
		if (buf_len(&host) < len) break;

		if (strncmp(NAS_INFO_URL, buf_rd(&host), len) && 
		    strncmp("http://nasgetinfo.com", buf_rd(&host), len+2))
			break;

		ctrl_web_nasinfo_fill_with(ctrl, sess, buf, ems_current_tm(), ctrl_web_callback(&param));

		goto handle_over;
	} while (0);

	if (permit) {
		ctrl_web_redirect_to_same_page(ctrl, sess, buf, srcurl);
	} else {
#ifndef SKIP_ERROR
		if (ems_flag_like(emscorer()->flg, FLG_CONFIG_READY)) {
			ctrl_web_portal_fill_with(ctrl, sess, buf, ems_current_tm(), srcurl);
		} else {
			ctrl_web_redirect_to_error(ctrl, sess, buf);
#if 0
			ems_int      st = 0;
			json_object *root, *emsc;

			root = json_object_new_object();
			do {
				ems_app_process(ty_ctrl, ty_client, EMS_APP_EMS_STATUS, root);
				emsc = json_object_object_get(root, "ems_c");
				if (!emsc) break;

				ems_json_get_int_def(emsc, "status", st, 0);
				/* we should skip radius error and continue on*/
				{
					json_object *obj_err;
					ems_int      err;

					obj_err = json_object_object_get(emsc, "err");
					if (!obj_err) break;

					ems_json_get_int_def(obj_err, "code", err, 0);
					if (    (err == RADIUS_ERR_CANNOT_CONNECT) ||
						(err == RADIUS_ERR_REJECT)         || 
						(err == RADIUS_ERR_NETWORK)) 
					{
						/* reset st and make it continue on  */
						st = 0;
					}
				}
			} while (0);
			json_object_put(root);

			if (st == 4)
				ctrl_web_redirect_to_error(ctrl, sess, buf);
			else {
				ems_assert(st != 0 && "never be here");
				ctrl_web_portal_fill_with(ctrl, sess, buf, ems_current_tm(), srcurl);
			}
#endif
		}
#else
		/* strlen("http://") == 7 */
		ctrl_web_portal_fill_with(ctrl, sess, buf, ems_current_tm(), srcurl);
#endif
	}

handle_over:

	ems_buffer_uninit(&param);
	ems_buffer_uninit(&host);

	ems_l_trace("sess(%s:%d): buffer length: %d", 
			ems_sock_addr(&sess->sock),
			ems_sock_port(&sess->sock),
			buf_len(buf));

	ems_buffer_write(&sess->buf, buf_rd(buf), buf_len(buf));
	sess_event_set(sess, EMS_EVT_READ | EMS_EVT_WRITE, evt_cmd_cb);
	sess_timeout_set_sorted(sess, EMS_CMD_TIMEOUT, timeout_cmd_cb);

	ems_flag_set(sess->flg, SESSION_FLAG_DIE_AFTER_SEND);

	return EMS_OK;
}

static ems_int ctrl_web_msg_redirect(ems_ctrl *ctrl, ems_session *sess)
{
	ems_flag_set(sess->flg, FLG_SESSION_IS_WEB);
	ems_buffer_clear(core_buffer());

	if (ems_flag_like(emscorer()->flg, FLG_FIRST_CONFIG)) {
		return ctrl_web_redirect_to_homepage(ctrl, sess);
	}

	if (ems_flag_like(emscorer()->flg, FLG_NETWORK_READY)) {
		return ctrl_web_redirect_to_portal(ctrl, sess);
	}

	return ctrl_web_redirect_to_mgmt(ctrl, sess);
}

static ems_int
ctrl_cmd_process(ems_ctrl *ctrl, ems_session *sess)
{
	ems_request  req;

	if (ems_flag_like(sess->flg, FLG_SESSION_IS_WEB))
	{
		ems_buffer_clear(&sess->buf_in);
		return EMS_CONTINUE;
	}

	if (ctrl_msg_is_web(sess)) {
		ems_int      rtn;
		rtn = ctrl_web_msg_redirect(ctrl, sess);
		ems_buffer_clear(&sess->buf_in);
		return rtn;
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

