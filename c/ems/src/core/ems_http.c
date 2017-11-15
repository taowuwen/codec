#include "ems_core.h"
#include "ems_client.h"
#include "ems_dns.h"
#include "app_nic.h"
#include "ems_http.h"
#include "app.h"
#include "ems_fw.h"

#define EMS_CMD_TIMEOUT		3000

static ems_void evt_cmd_cb(ems_session *sess, ems_int err, ems_int flg);
static ems_void timeout_cmd_cb(ems_session *sess, ems_timeout *to);

ems_url_whitelist *ems_url_whitelist_new()
{
	ems_url_whitelist *white = NULL;

	white = (ems_url_whitelist *)ems_malloc(sizeof(ems_url_whitelist));

	if (white) {
		ems_queue_init(&white->entry);
		str_init(&white->key);
	}

	return white;
}

ems_void ems_url_whitelist_destroy(ems_url_whitelist *white)
{
	ems_assert(white);
	if (white) {
		str_uninit(&white->key);
		ems_free(white);
	}
}


ems_cna_rule *ems_cna_rule_new()
{
	ems_cna_rule *cna = NULL;

	cna = (ems_cna_rule *)ems_malloc(sizeof(ems_cna_rule));
	if (cna) {
		ems_queue_init(&cna->entry);
		str_init(&cna->host);
		str_init(&cna->param);
	}

	return cna;
}

ems_void ems_cna_rule_destroy(ems_cna_rule *cna)
{
	ems_assert(cna);
	if (cna) {
		str_uninit(&cna->host);
		str_uninit(&cna->param);
		ems_free(cna);
	}
}

static ems_int http_start(ems_http *http, ems_session *sess, ems_uint flg);
static ems_int http_stopped(ems_http *http, ems_session *sess, ems_uint flg);
static ems_int http_normal(ems_http *http, ems_session *sess, ems_uint flg);
static ems_int http_evt_run(ems_http *http, ems_session *sess, ems_uint flg);

typedef ems_int (*http_evt_func)(ems_http *http, ems_session *sess, ems_uint flg);
static http_evt_func http_handler[] = 
{
	[st_start]   = http_start,
	[st_stopped] = http_stopped,
	[st_normal]  = http_normal,
	[st_max]     = NULL
};

static ems_void evt_http_cb(ems_session *sess, ems_int err, ems_int flg)
{
	ems_http  *http = (ems_http *)sess_cbarg(sess);

	ems_assert(http);
	if (err) {
		ems_assert(0 && "should never be here");
		ems_http_change_status(http, st_stopped);
		/* restart */
		ems_http_change_status(http, st_start);
		return;
	}

	http_evt_run(http, sess, flg);
}

ems_int http_set_grey_free(ems_http *http, ems_int enable)
{
	ems_queue        *p;
	ems_url_greylist *grey;
	json_object      *jobj;

	ems_queue_foreach(&http->grey, p) {
		grey = ems_container_of(p, ems_url_greylist, entry);

		if (inet_addr(str_text(&grey->key)) == INADDR_NONE)
			continue;

		jobj = json_object_new_object();
		json_object_object_add(jobj,"addr",json_object_new_string(str_text(&grey->key)));
		json_object_object_add(jobj,"srcport", json_object_new_int(http->src_port));
		json_object_object_add(jobj,"dstport", json_object_new_int(http->dst_port));
		json_object_object_add(jobj,"enable",  json_object_new_int(enable));

		nic_processmsg(http->ssid, ty_http, ty_fw, EMS_APP_FW_SET_GREY_FREE, jobj);

		json_object_put(jobj);
	}


	return EMS_OK;
}


ems_int http_set_nas_free(ems_http *http, ems_int enable)
{
	json_object      *jobj;

	if (INADDR_NONE == inet_addr(str_text(&http->nas)))
		return EMS_OK;

	jobj = json_object_new_object();
	json_object_object_add(jobj, "addr", json_object_new_string(str_text(&http->nas)));
	json_object_object_add(jobj, "srcport", json_object_new_int(http->src_port));
	json_object_object_add(jobj, "dstport", json_object_new_int(http->dst_port));
	json_object_object_add(jobj, "enable",  json_object_new_int(enable));

	nic_processmsg(http->ssid, ty_http, ty_fw, EMS_APP_FW_SET_NAS_FREE, jobj);

	json_object_put(jobj);

	return EMS_OK;
}

ems_int http_set_http_free(ems_http *http, ems_int enable)
{
	json_object  *jobj;

	jobj = json_object_new_object();
	json_object_object_add(jobj, "srcport", json_object_new_int(http->src_port));
	json_object_object_add(jobj, "dstport", json_object_new_int(http->dst_port));
	json_object_object_add(jobj, "enable",  json_object_new_int(enable));

	nic_processmsg(http->ssid, ty_http, ty_fw, EMS_APP_FW_SET_HTTP_FREE, jobj);

	json_object_put(jobj);

	return EMS_OK;

}

static ems_int http_start(ems_http *http, ems_session *sess, ems_uint flg)
{
	ems_assert(http->sess == NULL);

	if (!http->sess) {
		http->sess = ems_session_new();
		if (!http->sess)
			return EMS_ERR;

		ems_sock_setaddr(&http->sess->sock, EMS_ADDR);
	}

	sess = http->sess;

	http->dst_port = 9200;
again:
	ems_sock_setport(&sess->sock, http->dst_port);

	if (ems_sock_be_server(&sess->sock) != EMS_OK) {
		http->dst_port++;
		if (http->dst_port <= 9220)
			goto again;

		return EMS_ERR;
	}

	sess_cbarg_set(sess, http);
	sess_event_set(sess, EMS_EVT_READ, evt_http_cb);

	return ems_http_change_status(http, st_normal);
}

static ems_int http_stopped(ems_http *http, ems_session *sess, ems_uint flg)
{
	if (ems_flag_like(http->flg, EMS_FLG_HTTP_ENABLE_NASGETINFO))
		http_set_nas_free(http, EMS_NO);

	if (ems_flag_like(http->flg, EMS_FLG_HTTP_ENABLE_GREYLIST))
		http_set_grey_free(http, EMS_NO);

	http_set_http_free(http, EMS_NO);

	if (http->sess) {
		ems_session_shutdown_and_destroy(http->sess);
		http->sess = NULL;
	}

	ems_queue_clear(&http->cmd, ems_session, entry, ems_session_shutdown_and_destroy);
	ems_queue_clear(&http->cna_list, ems_cna, entry, http_cna_destroy);

	return EMS_OK;
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

static ems_int http_web_redirect_to_homepage(ems_http *http, ems_session *sess)
{
	ems_int    len;
	ems_buffer *buf = core_buffer();

	len = snprintf(buf_wr(buf), buf_left(buf),
		"%s 302 Moved Temporarily\r\n"
		"Server: yingkewifi\r\n"
		"Connection: close\r\n"
		"Date: %s GMT\r\n"
		"Location: http://%s/cgi-bin/luci/\r\n"
		"Content-Type: text/plain\r\n\r\n"
		"<html></html>\r\n"
		, http_header_get(http->hdr, HTTP_Version), ems_current_tm(), core_gw_addr());

	ems_buffer_write(&sess->buf, buf_rd(buf), len);

	sess_event_set(sess,   EMS_EVT_READ | EMS_EVT_WRITE,   evt_cmd_cb);
	sess_timeout_set_sorted(sess, EMS_CMD_TIMEOUT, timeout_cmd_cb);

	ems_flag_set(sess->flg, SESSION_FLAG_DIE_AFTER_SEND);

	return EMS_OK;
}

ems_int http_web_redirect_to_mgmt(ems_http *http, ems_session *sess)
{
	ems_int    len;
	ems_buffer *buf = core_buffer();

	len = snprintf(buf_wr(buf), buf_left(buf),
		"HTTP/1.1 302 Found\r\n"
		"Server: yingkewifi\r\n"
		"Connection: close\r\n"
		"Date: %s GMT\r\n"
		"Location: http://%s/cgi-bin/luci/link_fail\r\n"
		"Content-Type: text/plain\r\n\r\n"
		, ems_current_tm(), core_gw_addr());

	ems_buffer_write(&sess->buf, buf_rd(buf), len);

	sess_event_set(sess,   EMS_EVT_READ | EMS_EVT_WRITE,   evt_cmd_cb);
	sess_timeout_set_sorted(sess, EMS_CMD_TIMEOUT, timeout_cmd_cb);

	ems_flag_set(sess->flg, SESSION_FLAG_DIE_AFTER_SEND);

	return EMS_OK;
}

static ems_int http_web_redirect_to_error(ems_http *http, ems_session *sess, ems_buffer *buf)
{
	ems_int    len;

	len = snprintf(buf_wr(buf), buf_left(buf),
		"%s 302 Moved Temporarily\r\n"
		"Server: yingkewifi\r\n"
		"Connection: close\r\n"
		"Date: %s GMT\r\n"
		"Location: http://%s/cgi-bin/luci/guide_fail\r\n"
		"Content-Type: text/plain\r\n\r\n"
		"<html></html>\r\n"
		, http_header_get(http->hdr, HTTP_Version), ems_current_tm(), core_gw_addr());

	ems_buffer_seek_wr(buf, len, EMS_BUFFER_SEEK_CUR);

	return EMS_OK;
}

static ems_int 
http_web_redirect_to_same_page(ems_http *http, ems_session *sess, ems_buffer *buf, ems_cchar *srcurl)
{
	ems_int    len;

	len = snprintf(buf_wr(buf), buf_left(buf),
		"%s 302 Moved Temporarily\r\n"
		"Server: yingkewifi\r\n"
		"Connection: close\r\n"
		"Date: %s GMT\r\n"
		"Location: %s\r\n"
		"Content-Type: text/plain\r\n\r\n"
		"<html></html>\r\n"
		, http_header_get(http->hdr, HTTP_Version),
		ems_current_tm(), srcurl?srcurl:"no");

	ems_buffer_seek_wr(buf, len, EMS_BUFFER_SEEK_CUR);

	return EMS_OK;
}

ems_char *ems_mac_update(ems_char *dst, ems_cchar *src)
{
	ems_char *p = dst;

	while (*src) {
		*dst = *src++;

		if (*dst == ':')
			*dst = '-';
		dst++;
	}

	*dst = '\0';

	return p;
}

ems_cchar *ems_usermac(ems_cchar *ip)
{
	return ems_popen_get("/sbin/arp -n | grep -v incomplete | grep '(%s)' | awk '{print $4}'", ip);
}

ems_int ems_urlencode(ems_buffer *dst, ems_buffer *src, ems_int autoinc)
{
	ems_char *buf = NULL;
	ems_int   len  = 0, ret;

	do {
		ret = EMS_ERR;

		buf = url_encode(buf_rd(src), buf_len(src));

		if (!buf) break;

		len = ems_strlen(buf);

		if (len >= buf_left(dst) && autoinc) 
			ems_buffer_increase(dst, len);

		if (len >= buf_left(dst)) {
			ret = EMS_BUFFER_INSUFFICIENT;
			break;
		}

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
http_web_portal_fill_with(ems_http *http, ems_session *sess, ems_buffer *buf, ems_cchar *date, ems_cchar *srcurl)
{
	ems_int    rtn;
	static ems_char wlanusermac[32], wlanapmac[32];
	ems_buffer arg;
	ems_wifi_iface *iface = NULL;

	ems_assert(http && http->ssid && http->ssid->_iface);

	iface = http->ssid->_iface;

	ems_cchar *wlanuserip  = ems_sock_addr(&sess->sock);

	ems_mac_update(wlanusermac, ems_usermac(wlanuserip));
	ems_mac_update(wlanapmac,   str_text(&iface->bssid));

	ems_buffer_init(&arg, EMS_BUFFER_2K);
	memset(buf_wr(&arg), 0, buf_left(&arg));

	rtn = snprintf(buf_wr(&arg), buf_left(&arg), 
			"wlanuserip=%s&"
			"wlanacname=%s&"
			"ssid=%s&"
			"wlanusermac=%s&"
			"wlanapmac=%s&"
			"apsn=%s",
			wlanuserip, core_sn(), str_text(&iface->ssid), wlanusermac, wlanapmac, core_sn());

	ems_buffer_seek_wr(&arg, rtn, EMS_BUFFER_SEEK_CUR);
	{
		ems_char *tmp = url_encode(core_devicetype(), strlen(core_devicetype()));
		if (tmp) {
			// strlen("&devicetype=") == 12;
			ems_buffer_write(&arg, "&devicetype=", 12);
			ems_buffer_write(&arg, tmp, strlen(tmp));
			ems_free(tmp);
		}
	}

	if (srcurl) {
		// strlen("&srcurl=") == 8;
		ems_buffer_write(&arg, "&srcurl=", 8);
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

	ems_l_trace("[http]args: %s", buf_rd(&arg));
	rtn = snprintf(buf_wr(buf), buf_left(buf),
		"%s 302 Found\r\n"
		"Date: %s GMT\r\n"
		"Server: yingkewifi\r\n"
		"Connection: close\r\n"
		"Location: http://%s:%d/portal/Login.do?%s\r\n"
		"Content-Type: text/plain;charset=utf-8\r\n"
		"\r\n"
		"<html></html>\r\n"
		, http_header_get(http->hdr, HTTP_Version), date,
		str_text(&iface->auth.ptl.addr),
		iface->auth.ptl.redirect_port,
		buf_rd(&arg));

	rtn = rtn < buf_left(buf) ? rtn: buf_left(buf);
	ems_buffer_uninit(&arg);

	ems_buffer_seek_wr(buf, rtn, EMS_BUFFER_SEEK_CUR);

	return EMS_OK;
}

static ems_cchar *http_web_callback(ems_cchar *param)
{
	ems_char *p, *q;

#define NAS_CALLBACK	"callback="

	q = NULL;

	p = strstr(param, NAS_CALLBACK);
	if (p) {
		q = p + strlen(NAS_CALLBACK);

		p = strchr(q, '&');
		if (p)
			*p = '\0';
	}

	return q;
}

static ems_int 
http_get_userinfo(ems_http *http, ems_cchar *ip, ems_str *user, ems_str *mac)
{
	json_object *userinfo = NULL;
	
	userinfo = json_object_new_object();

	if (userinfo) {
		json_object_object_add(userinfo, "ip", json_object_new_string(ip));
		nic_processmsg(http->ssid, ty_http, ty_radius, EMS_APP_EVT_USERINFO, userinfo);

		ems_json_get_string_def(userinfo, "nick", user, NULL); 
		ems_json_get_string_def(userinfo, "mac",  mac,  NULL);
		json_object_put(userinfo);
	}

	json_object_put(userinfo);

	return EMS_OK;
}

static ems_int http_kickout_user(ems_http *http, ems_cchar *ip, ems_cchar *user, ems_cchar *mac)
{
	json_object *req = json_object_new_object();

	ems_assert(user && mac && ip);
	if (!req)
		return EMS_ERR;

	json_object_object_add(req, "userip",   json_object_new_string(ip));
	json_object_object_add(req, "usermac",  json_object_new_string(mac?mac:""));
	json_object_object_add(req, "username", json_object_new_string(user?user:"no"));

	nic_processmsg(http->ssid, ty_http,  ty_radius,  EMS_APP_CMD_RADIUS_LOGOUT, req);

	json_object_put(req);

	return EMS_OK;
}

static ems_int 
http_web_nasinfo_fill_with(ems_http *http, ems_session *sess, ems_buffer *buf, ems_cchar *date, ems_cchar *func)
{
	ems_int      res;
	json_object *rsp, *obj;
	ems_cchar   *ctx = NULL;
	ems_cchar   *wlanuserip = ems_sock_addr(&sess->sock);
	ems_str      user, mac;
	ems_wifi_iface *iface = NULL;

	iface = http->ssid->_iface;

	str_init(&user);
	str_init(&mac);
	http_get_userinfo(http, wlanuserip, &user, &mac);

	rsp = json_object_new_object();
	obj = json_object_new_object(); 

	{
		static ems_char wlanusermac[32], wlanapmac[32], usermac[32];

		snprintf(usermac, 32, "%s", ems_usermac(wlanuserip));
		ems_mac_update(wlanusermac, usermac);
		ems_mac_update(wlanapmac,   str_text(&iface->bssid));

		res = 0;
		if (        str_len(&user) <= 0 
			 || str_len(&mac)  <= 0 
			 || strcmp(str_text(&mac), usermac)) {
			if (str_len(&user) > 0) {
				ems_l_trace("[http]user(%s) mac updated: %s->%s", wlanuserip, str_text(&mac), usermac);
				http_kickout_user(http, wlanuserip, str_text(&user), str_text(&mac));
			}

			str_set(&user, "");
			res = 1;
		} 

		json_object_object_add(obj, "portalip",   json_object_new_string(str_text(&iface->auth.ptl.addr)));
		json_object_object_add(obj, "username",   json_object_new_string(str_text(&user)));
		json_object_object_add(obj, "wlanusermac",json_object_new_string(wlanusermac));
		json_object_object_add(obj, "wlanuserip", json_object_new_string(wlanuserip));
		json_object_object_add(obj, "ssid",       json_object_new_string(str_text(&iface->ssid)));
		json_object_object_add(obj, "wlanapmac",  json_object_new_string(wlanapmac));
		json_object_object_add(obj, "wlanacmac",  json_object_new_string(ems_mac_update(wlanapmac, core_ac_mac())));
		json_object_object_add(obj, "wlanacname", json_object_new_string(core_sn()));
		json_object_object_add(obj, "devicetype", json_object_new_string(core_devicetype()));
		json_object_object_add(obj, "sn",         json_object_new_string(core_sn()));
		json_object_object_add(obj, "acip",       json_object_new_string(core_gw_addr()));
	}
	json_object_object_add(rsp, "result", json_object_new_int(res));
	json_object_object_add(rsp, "data",   obj);

	str_uninit(&user);
	str_uninit(&mac);

	ctx = json_object_to_json_string(rsp);

	if (!ctx)
		ctx = "";

	if (!func)
		func = "afunc";

	ems_l_trace("[http]nasgetinfo: %s", ctx);

	res = snprintf(buf_wr(buf), buf_left(buf),
		"%s 200 OK\r\n"
		"Date: %s GMT\r\n"
		"Server: yingkewifi\r\n"
		"Connection: close\r\n"
		"Content-Length: %d\r\n"
		"Content-Type: text/plain;charset=utf-8\r\n"
		"\r\n"
		"%s(%s);"
		, http_header_get(http->hdr, HTTP_Version),
		date, (ems_int)(strlen(ctx) + strlen(func) + 3), func, ctx);

	ems_buffer_seek_wr(buf, res, EMS_BUFFER_SEEK_CUR);

	json_object_put(rsp);

	return EMS_OK;
}

static ems_int http_web_try_nasgetinfo(ems_http *http, ems_session *sess, ems_cchar *param, ems_cchar *host)
{
	ems_int len;
	/* check params */
#define PORTAL_PARAMS "/portal/?"
	len = 9; /* 9 == strlen(PORTAL_PARAMS); */
	if (ems_strlen(param) <= len) 
		return EMS_ERR;
		
	if (strncmp(param, PORTAL_PARAMS, len)) 
		return EMS_ERR;

	/* check host */
	if (ems_strlen(host) < str_len(&http->nas)) 
		return EMS_ERR;

	if (strncmp(str_text(&http->nas), host, str_len(&http->nas)))
		return EMS_ERR;

	return EMS_OK;
}

static ems_int 
http_web_try_subdomain(ems_http *http, ems_session *sess, ems_cchar *host)
{
	ems_int rtn;
	json_object *obj;

	rtn = EMS_ERR;

	obj = json_object_new_object();
	json_object_object_add(obj, "url", json_object_new_string(host));

	if (nic_processmsg(http->ssid, ty_http, ty_fw, EMS_APP_CHECK_SUBDOMAIN, obj) == EMS_YES) 
		rtn = EMS_OK;

	json_object_put(obj);

	return rtn;
}

static ems_int
http_web_try_urlwhitelist(ems_http *http, ems_session *sess, ems_cchar *param, ems_cchar *host)
{
	json_object *obj;
	ems_queue *p;
	ems_url_whitelist *url;

	ems_assert(http && sess && param && host);

	ems_queue_foreach(&http->url, p) {
		url = ems_container_of(p, ems_url_whitelist, entry);

		ems_assert(url);

		if (str_len(&url->key) > ems_strlen(param))
			continue;

		if (strstr(param, str_text(&url->key))) {
			obj = json_object_new_object();
			json_object_object_add(obj, "addr", json_object_new_string(host));
			nic_processmsg(http->ssid, ty_http, ty_fw, EMS_APP_CHECK_URL_WHITELIST, obj);
			json_object_put(obj);
			return EMS_OK;
		}
	}

	return EMS_ERR;
}

static ems_int 
http_cna_return_failed(ems_http *http, ems_session *sess, ems_buffer *buf)
{
	ems_int ret = 0;

	ret = snprintf(buf_wr(buf), buf_left(buf),
		"%s 200 OK\r\n"
		"Date: %s GMT\r\n"
		"Server: yingkewifi\r\n"
		"Connection: close\r\n"
		"Content-Type: text/plain;charset=utf-8\r\n"
		"\r\n"
		"<HTML><HEAD><TITLE>Failed</TITLE></HEAD><BODY>Failed</BODY></HTML>"
		,
		http_header_get(http->hdr, HTTP_Version), ems_current_tm());

	ems_buffer_seek_wr(buf, ret, EMS_BUFFER_SEEK_CUR);

	return EMS_OK;
}

static ems_int 
http_cna_return_success(ems_http *http, ems_session *sess, ems_buffer *buf)
{
	ems_int ret = 0;

	ret = snprintf(buf_wr(buf), buf_left(buf),
		"%s 200 OK\r\n"
		"Date: %s GMT\r\n"
		"Server: yingkewifi\r\n"
		"Connection: close\r\n"
		"Content-Type: text/plain;charset=utf-8\r\n"
		"\r\n"
		"<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>"
		,
		http_header_get(http->hdr, HTTP_Version), ems_current_tm());

	ems_buffer_seek_wr(buf, ret, EMS_BUFFER_SEEK_CUR);

	return EMS_OK;
}

static ems_int
http_web_try_cnarequest(ems_http *http, ems_session *sess, ems_cchar *param, ems_cchar *host)
{
	ems_int    rtn;
	ems_queue *p;
	ems_cna_rule *cna;

	ems_assert(http && sess && param && host);

	ems_queue_foreach(&http->cna, p) {
		cna = ems_container_of(p, ems_cna_rule, entry);

		ems_assert(cna);

		if (str_len(&cna->host) != ems_strlen(host)) 
			continue;

		if (strncmp(str_text(&cna->host), host, str_len(&cna->host)))
			continue;
		/*
		   here, we should check user online or not, 
		   but we are not isg or fitap, it's none of fat ap's bussines
		 */
		ems_l_trace("[http]CNA request: %s", host);

		rtn = http_cna_user_run(
				http,
				http_cna_user_find(http, ems_sock_addr(&sess->sock)),
				sess,
				host,
				param);
		switch(rtn) {
			case HTTP_RETURN_FAILED:
				http_cna_return_failed(http, sess, core_buffer());
				break;

			case HTTP_RETURN_SUCCESS:
				http_cna_return_success(http, sess, core_buffer());
				break;

			default:
				/*http_web_portal_fill_with(http, sess, core_buffer(), ems_current_tm(), srcurl);*/
				/* just return error here */
				return EMS_ERR;
				break;
		}

		return EMS_OK;
	}

	return EMS_ERR;
}


/*
   get params
   get hosts
   get srcurl

   a. check nasgetinfo  ----> nasgetinfo's check
   b. check url whitelist ---> redirect to same page
   c. check subdomain ----> redirect to same page
   d. check cna ----> goes to cna status machine
   e. redirect to portal
 */

static ems_int 
http_web_do_redirect(ems_http *http, ems_session *sess, http_header *hdr, ems_buffer *buf)
{
	ems_buffer url;
	ems_int    l;
	ems_char   *srcurl;

	ems_cchar *param = http_header_get(hdr, HTTP_Param);
	ems_cchar *host  = http_header_get(hdr, HTTP_Host);

	ems_assert(param && host);

	ems_buffer_init(&url, ems_strlen(param) + ems_strlen(host) + EMS_BUFFER_1K);

	if (ems_flag_like(http->flg, EMS_FLG_HTTP_ENABLE_NASGETINFO)) {
		if (http_web_try_nasgetinfo(http, sess, param, host) == EMS_OK) {
			http_web_nasinfo_fill_with(http, sess, buf, ems_current_tm(), http_web_callback(param));
			goto finished;
		}
	}

	l = snprintf(buf_wr(&url), buf_left(&url), "http://%s:%d%s", host, http->src_port, param);
	ems_buffer_seek_wr(&url, l, EMS_BUFFER_SEEK_CUR);

	if (ems_flag_like(emscorer()->flg, FLG_SUBDOMAIN_ENABLE)){
		if (http_web_try_subdomain(http, sess, host) == EMS_OK) {
			ems_l_trace("[http] host: %s in domain whitelist ", host);
			http_web_redirect_to_same_page(http, sess, buf, buf_rd(&url));
			goto finished;
		}
	}

	if (ems_flag_like(http->flg, EMS_FLG_HTTP_ENABLE_URL)) {
		if (http_web_try_urlwhitelist(http, sess, param, host) == EMS_OK) {
			ems_l_trace("[http] params: %s in url whitelist ", param);
			http_web_redirect_to_same_page(http, sess, buf, buf_rd(&url));
			goto finished;
		}
	}

	/* encode url */
	ems_buffer_clear(&sess->buf_in);
	ems_urlencode(&sess->buf_in, &url, EMS_YES);

	srcurl = NULL;
	ems_assert(buf_len(&sess->buf_in) > 0 && param != NULL && host != NULL);
	if (buf_len(&sess->buf_in) > 0 && param != NULL && host != NULL)
		srcurl = buf_rd(&sess->buf_in);

	ems_l_trace("[http]request url:[%d] %s", buf_len(&url), buf_rd(&url));
	ems_l_trace("[http]url encoded:[%d] %s", buf_len(&sess->buf_in), srcurl?srcurl:"NULL");

	if (ems_flag_like(http->flg, EMS_FLG_HTTP_ENABLE_CNA)) {
		if (http_web_try_cnarequest(http, sess, param, host) == EMS_OK)
			goto finished;
	}

	if (ems_flag_like(emscorer()->flg, FLG_CONFIG_READY))
		http_web_portal_fill_with(http, sess, buf, ems_current_tm(), srcurl);
	else
		http_web_redirect_to_error(http, sess, buf);

finished:
	ems_buffer_uninit(&url);

	return EMS_OK;
}

static ems_int http_web_redirect_to_portal(ems_http *http, ems_session *sess)
{
	ems_buffer *buf = core_buffer();

	http_header_parse(http->hdr, buf_rd(&sess->buf_in));
	http_web_do_redirect(http, sess, http->hdr, buf);

	ems_l_trace("[http]sess(%s:%d): buffer length: %d, buf_left: %d", 
			ems_sock_addr(&sess->sock),
			ems_sock_port(&sess->sock),
			buf_len(buf), buf_left(&sess->buf));

	if (buf_left(&sess->buf) <= buf_len(buf))
		ems_buffer_increase(&sess->buf, buf_len(buf)); 

	ems_buffer_write(&sess->buf, buf_rd(buf), buf_len(buf));
	sess_event_set(sess, EMS_EVT_READ | EMS_EVT_WRITE, evt_cmd_cb);
	sess_timeout_set_sorted(sess, EMS_CMD_TIMEOUT, timeout_cmd_cb);

	ems_flag_set(sess->flg, SESSION_FLAG_DIE_AFTER_SEND);

	return EMS_OK;
}

static ems_int http_web_msg_redirect(ems_http *http, ems_session *sess)
{
	ems_flag_set(sess->flg, FLG_SESSION_IS_WEB);
	ems_buffer_clear(core_buffer());

	if (ems_flag_like(emscorer()->flg, FLG_FIRST_CONFIG)) {
		return http_web_redirect_to_homepage(http, sess);
	}

	if (ems_flag_like(emscorer()->flg, FLG_NETWORK_READY)) {
		return http_web_redirect_to_portal(http, sess);
	}

	return http_web_redirect_to_mgmt(http, sess);
}

static ems_int
http_cmd_process(ems_http *http, ems_session *sess)
{
	ems_int      rtn;

	if (ems_flag_like(sess->flg, FLG_SESSION_IS_WEB))
	{
		ems_buffer_clear(&sess->buf_in);
		return EMS_CONTINUE;
	}

	do {
		rtn = EMS_ERR;

		if (!http_msg_is_web(buf_rd(&sess->buf_in))) break;

		if (!http_header_full(buf_rd(&sess->buf_in))) {
			ems_l_trace("[http][%s: %d]buffer size: %d, buffer_left: %d do increase", 
					ems_sock_addr(&sess->sock),
					ems_sock_port(&sess->sock),
					buf_size(&sess->buf_in),
					buf_left(&sess->buf_in));

			if (buf_size(&sess->buf_in) >= EMS_BUFFER_8k) 
				break; 

			ems_buffer_increase(&sess->buf_in, EMS_BUFFER_1K);
			rtn = EMS_CONTINUE;
			break;
		}

		ems_l_trace("[http][%s:%d]: header size: %d, left: %d", 
				ems_sock_addr(&sess->sock), ems_sock_port(&sess->sock),
				buf_len(&sess->buf_in), buf_left(&sess->buf_in));

		rtn = http_web_msg_redirect(http, sess);
		ems_buffer_clear(&sess->buf_in);

	} while (0);

	return rtn;
}

static ems_int
http_cmd_read(ems_http *http, ems_session *sess, ems_int flg)
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

	switch (http_cmd_process(http, sess)) {
		
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
http_cmd_write(ems_http *http, ems_session *sess, ems_int flg)
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
	ems_http *http = (ems_http *)sess_cbarg(sess);

	if (err)
		goto err_out;

	do {
		if (ems_flag_like(flg, EMS_EVT_READ) && 
		    http_cmd_read(http, sess, flg) == EMS_ERR) 
		{
			goto err_out;
		}

		if (ems_flag_like(flg, EMS_EVT_WRITE) &&
		    http_cmd_write(http, sess, flg) == EMS_ERR)
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
	ems_l_trace("[http]timeout for cmd sess(%d): %s:%d",
			ems_sock_fd(&sess->sock),
			ems_sock_addr(&sess->sock),
			ems_sock_port(&sess->sock));

	ems_queue_remove(&sess->entry);
	ems_session_shutdown_and_destroy(sess);
}


static ems_int 
http_sess_in(ems_http *http, ems_int sockfd, ems_cchar *addr, ems_int port)
{
	ems_session  *sess = NULL;

	sess = ems_session_new();
	if (!sess)
		return EMS_ERR;

	ems_sock_setaddr(&sess->sock, addr);
	ems_sock_setport(&sess->sock, port);
	ems_sock_setfd(&sess->sock, sockfd);

	ems_queue_insert_tail(&http->cmd, &sess->entry);

	sess_cbarg_set(sess, http);
	sess_event_set(sess, EMS_EVT_READ, evt_cmd_cb);
	sess_timeout_set_sorted(sess, EMS_CMD_TIMEOUT, timeout_cmd_cb);

	return EMS_OK;
}

static ems_int 
http_accept_next(ems_http *http, ems_int sock)
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
			ems_l_trace("[http]accept error: %s", ems_lasterrmsg());
			return NO;
		}
	}

	ems_l_trace("[http] NEW connection (%d) from: %s:%d in", 
				ret, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
	{
		struct linger lg;

		lg.l_onoff  = 1;
		lg.l_linger = 2;
		setsockopt(ret, SOL_SOCKET, SO_LINGER, (ems_cchar *)&lg, sizeof(lg));
	}

	if (http_sess_in(http, ret, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port)))
	{
		close(ret);
	}

	return EMS_YES;
}

static ems_int http_normal(ems_http *http, ems_session *sess, ems_uint flg)
{
	ems_int    next;

	ems_assert(ems_flag_like(flg, EMS_EVT_READ));

	if (ems_flag_unlike(flg, EMS_EVT_READ))
		return EMS_OK;

	do {
		next = EMS_NO;
		next = http_accept_next(http, ems_sock_fd(&sess->sock));
	} while (next);

	return EMS_OK;
}

static ems_int http_evt_run(ems_http *http, ems_session *sess, ems_uint flg)
{
	ems_assert(http->st >= st_start && http->st <= st_normal);

	if (http_handler[http->st])
		return http_handler[http->st](http, sess, flg);

	return EMS_OK;
}

ems_int ems_http_change_status(ems_http *http, ems_status st)
{
	http->st = st;

	switch(st) {
	case st_start:
	case st_stopped:
		return http_evt_run(http, NULL, 0);

	default:
		break;
	}

	return EMS_OK;
}

