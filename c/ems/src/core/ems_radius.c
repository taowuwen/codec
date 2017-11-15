
#include "ems_core.h"
#include "ems_client.h"
#include "app.h"
#include "app_nic.h"
#include "ems_radius.h"
#include "ems_fw.h"

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>

#define PING_TIMEOUT	2000
#define PING_RETRYTIMES	3

extern ems_char *ems_mac_update(ems_char *dst, ems_cchar *src);

static ems_int ra_start(ems_device *dev, ems_session *sess, ems_uint flg);
static ems_int ra_auth (ems_device *dev, ems_session *sess, ems_uint flg);
static ems_int ra_acct (ems_device *dev, ems_session *sess, ems_uint flg);
static ems_int ra_normal(ems_device *dev, ems_session *sess, ems_uint flg);
static ems_int ra_ping (ems_device *dev, ems_session *sess, ems_uint flg);
static ems_int ra_acct_stopped(ems_device *dev, ems_session *sess, ems_uint flg);
static ems_int ra_err  (ems_device *dev, ems_session *sess, ems_uint flg);

typedef ems_int (*ra_evt_func)(ems_device *dev, ems_session *sess, ems_uint flg);
static ra_evt_func ra_evt_handler[] = 
{
	[st_start]   = ra_start,
	[st_auth]    = ra_auth,
	[st_acct]    = ra_acct,
	[st_normal]  = ra_normal,
	[st_hb]      = ra_ping, 
	[st_acct_stop] = ra_acct_stopped,
	[st_err]     = ra_err,
	[st_max]     = NULL
};

static ems_int ra_to_auth(ems_device *dev, ems_session *sess, ems_timeout *to);
static ems_int ra_to_acct(ems_device *dev, ems_session *sess, ems_timeout *to);
static ems_int ra_to_normal(ems_device *dev, ems_session *sess, ems_timeout *to);
static ems_int ra_to_ping  (ems_device *dev, ems_session *sess, ems_timeout *to);
static ems_int ra_to_acct_stopped(ems_device *dev, ems_session *sess, ems_timeout *to);

typedef ems_int (*ra_timeout_func)(ems_device *dev, ems_session *sess, ems_timeout *to);
static ra_timeout_func ra_timeout_handler[] = 
{
	[st_start]   = NULL,
	[st_auth]    = ra_to_auth,
	[st_acct]    = ra_to_acct,
	[st_normal]  = ra_to_normal,
	[st_hb]      = ra_to_ping, 
	[st_acct_stop] = ra_to_acct_stopped,
	[st_err]     = NULL,
	[st_max]     = NULL
};

typedef ems_int (*dev_recv_cb)(ems_device *dev, ems_session *sess);

static ems_int dev_evt_run(ems_device *dev, ems_session *sess, ems_uint flg)
{
	ems_assert(ra_evt_handler[dev->st] != NULL);

	return ra_evt_handler[dev->st](dev, sess, flg);
}

static ems_int dev_timeout_run(ems_device *dev, ems_session *sess, ems_timeout *to)
{
	ems_assert(ra_timeout_handler[dev->st] != NULL);

	if (ra_timeout_handler[dev->st])
		return ra_timeout_handler[dev->st](dev, sess, to);

	return EMS_OK;
}

static ems_void dev_evt_cb(ems_session *sess, ems_int err, ems_int flg)
{
	ems_device *dev = (ems_device *)sess_cbarg(sess);

	ems_assert(dev->st > st_min && dev->st < st_max);

	if (err) {
		ems_l_trace("[radius] evt err, sess: %d %s",
				ems_sock_fd(&sess->sock),
				ems_sock_addr(&sess->sock));
		dev->reason = RADIUS_ERR_NETWORK;
		dev_change_status(dev, st_err);
		return;
	}

	dev_evt_run(dev, sess, flg);
}

static ems_void dev_timeout_cb(ems_session *sess, ems_timeout *to)
{
	ems_device *dev = (ems_device *)sess_cbarg(sess);

	ems_assert(dev->st > st_min && dev->st < st_max);

	dev_timeout_run(dev, sess, to);
}

#define ems_rc_avpair_add(rh, out, key, val, val_len, ven) \
	rc_avpair_add(rh, out, key, (ems_void *)val, val_len, ven)
	

static ems_int ra_fill_auth_avpairs(ems_device *dev, ems_radius *ra)
{
	static ems_char  buf[253];
	static ems_char  mac[32] = {0};
	ems_uint  tmp;
	ems_session *sess = dev->sess;

	ems_assert(dev && sess != NULL);

	ems_rc_avpair_add(ra->rh, &dev->vp_out, PW_USER_NAME, 
		str_text(&dev->user.name), str_len(&dev->user.name), 0);

	ems_rc_avpair_add(ra->rh, &dev->vp_out, PW_USER_PASSWORD, 
		str_text(&dev->user.pass), str_len(&dev->user.pass), 0);

	if (sess) {
		struct sockaddr_in addr;
		socklen_t  len;

		memset(&addr, 0, sizeof(addr));
		len = sizeof(addr);
		getsockname(ems_sock_fd(&sess->sock), (struct sockaddr *)&addr, &len);

		tmp = ntohl(addr.sin_addr.s_addr);
		ems_rc_avpair_add(ra->rh, &dev->vp_out, PW_NAS_IP_ADDRESS, &tmp, 0, 0);
	}

	tmp = 0;
	ems_rc_avpair_add(ra->rh, &dev->vp_out, PW_NAS_PORT, &tmp, -1, 0);

	/* rfc2865-- 5.41 NAS-Port-Type--> wireless IEEE 802.11 */
	tmp = 19; 
	ems_rc_avpair_add(ra->rh, &dev->vp_out, PW_NAS_PORT_TYPE, &tmp, -1, 0);

	tmp = ntohl(inet_addr(str_text(&dev->user.ip)));
	ems_rc_avpair_add(ra->rh, &dev->vp_out, PW_FRAMED_IP_ADDRESS, &tmp, 0, 0);

	/* called station id, format: apmac:apssid */
	{
		ems_wifi_iface *iface = ra->ssid->_iface;
		ems_mac_update(mac, str_text(&iface->bssid));
		snprintf(buf, sizeof(buf), "%s:%s", mac, str_text(&iface->ssid));
	}
	ems_rc_avpair_add(ra->rh, &dev->vp_out, PW_CALLED_STATION_ID, buf, strlen(buf), 0);

	memset(mac, 0, 32);
	ems_mac_update(mac, str_text(&dev->user.mac));
	ems_rc_avpair_add(ra->rh, &dev->vp_out, PW_CALLING_STATION_ID, mac, strlen(mac), 0);


	snprintf(buf, sizeof(buf), "%s", core_sn());
	ems_rc_avpair_add(ra->rh, &dev->vp_out, PW_NAS_IDENTIFIER, buf, strlen(buf), 0);

	/* rfc2869 5.14 Message-Authenticator set message authenticator into 0 now */
	memset(buf, 0, sizeof(buf));
	ems_rc_avpair_add(ra->rh, &dev->vp_out, PW_MESSAGE_AUTHENTICATOR, buf, 16, 0); 

	return EMS_OK;
}


/*
   1. fill auth server address
 */
static ems_int ra_fill_auth_request_info(ems_device *dev)
{
	ems_int      total_len;
	ems_radius  *ra   = (ems_radius *)dev->ctx;
	ems_session *sess = dev->sess;
	AUTH_HDR    *auth;
	ems_uchar     buf_sign[AUTH_VECTOR_LEN];

	ems_buffer_clear(&sess->buf_out);

	auth = (AUTH_HDR *)buf_wr(&sess->buf_out);

	auth->code = PW_ACCESS_REQUEST;
	auth->id   = ra->seq_nbr++;

	dev->auth_out = auth;

	rc_random_vector(dev->vector);
	memcpy(auth->vector, dev->vector, AUTH_VECTOR_LEN);

	ra_fill_auth_avpairs(dev, ra);

	total_len = rc_pack_list(dev->vp_out, str_text(&ra->secret), auth) + AUTH_HDR_LEN;

	auth->length = ntohs((unsigned short) total_len);

	rc_hmac_md5_calc((ems_uchar *)auth, total_len, 
			(ems_uchar *)(str_text(&ra->secret)), str_len(&ra->secret), buf_sign);
	memcpy((ems_char *)auth + total_len - AUTH_VECTOR_LEN, buf_sign, AUTH_VECTOR_LEN);

	ems_buffer_seek_wr(&sess->buf_out, total_len, EMS_BUFFER_SEEK_CUR);

	dev->total_len = total_len;

	return EMS_OK;
}

static ems_int ra_connect(ems_session *sess)
{
	ems_int    fd;
	socklen_t  len;
	struct sockaddr_in addr;
	ems_sock   *sock = &sess->sock;

	ems_assert(sess);
	
	memset(&addr, 0, sizeof(addr));
	if (ems_gethostbyname(ems_sock_addr(sock), &addr) != OK) {
		ems_l_trace("[radius]gethostbyename failed %s : %s", 
				ems_sock_addr(sock), ems_lasterrmsg());
		return EMS_ERR;
	}

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd <= 0)
		return EMS_ERR;

	ems_l_trace("[radius] sess(%d) connect to: %s(%s): %d...",
			fd, 
			ems_sock_addr(sock), 
			inet_ntoa(addr.sin_addr), 
			ems_sock_port(sock));

	addr.sin_family = AF_INET;
	addr.sin_port   = htons(ems_sock_port(sock));

	ems_setnonblocking(fd, YES);
	len = sizeof(struct sockaddr_in);
	if (connect(fd, (struct sockaddr *)&addr, len)) {
		ems_l_trace("[radius] connect to: %s:%d: failed: %s",
				ems_sock_addr(sock), 
				ems_sock_port(sock), 
				ems_lasterrmsg());
		close(fd);
		return EMS_ERR;
	}

	ems_sock_setfd(sock, fd);
	return EMS_OK;
}

static ems_int ra_start(ems_device *dev, ems_session *sess, ems_uint flg)
{
	ems_radius *ra = (ems_radius *)dev->ctx;
	if (!dev->sess) {
		dev->sess = ems_session_new();
		if (!dev->sess) {
			dev->reason = RADIUS_ERR_NETWORK;
			dev_change_status(dev, st_err);
			return EMS_ERR;
		}

		ems_buffer_increase(&dev->sess->buf_in,  EMS_BUFFER_2K);
		ems_buffer_increase(&dev->sess->buf_out, EMS_BUFFER_2K);
	}

	sess = dev->sess;
	sess_cbarg_set(sess, dev);

	ems_sock_setaddr(&sess->sock, str_text(&ra->auth_addr));
	ems_sock_setport(&sess->sock, ra->auth_port);
	if (ra_connect(sess) != EMS_OK) {
		dev->reason = RADIUS_ERR_NETWORK;
		return dev_change_status(dev, st_err);
	}

	ra_fill_auth_request_info(dev);

	dev->retry_times   = ra->retry_times;
	dev->retry_timeout = ra->retry_timeout;
	dev->reason        = 0;

	sess_event_set(sess, EMS_EVT_WRITE, dev_evt_cb);
	sess_timeout_set_sorted(sess, dev->retry_timeout * 1000, dev_timeout_cb);

	return dev_change_status(dev, st_auth);
}

static ems_int 
ra_send_msg(ems_device *dev, ems_session *sess, ems_uint flg)
{
	ems_int ret;

	ems_assert(ems_flag_like(flg, EMS_EVT_WRITE));

	ems_l_trace("[radius] send(%d): (%s, length: %d)", 
			ems_sock_fd(&sess->sock),
			ems_sock_addr(&sess->sock), 
			buf_len(&sess->buf));

	ret = sess_send(sess, &sess->buf);
	if (ret <= 0) {
		switch(ret) {
		case -EAGAIN:
			break;

		default:
			return EMS_ERR;
		}
	}

	if (buf_len(&sess->buf) <= 0)
		sess_event_set(sess, EMS_EVT_READ, dev_evt_cb);

	return EMS_OK;
}


static ems_int 
dev_process_rsp(ems_device *dev, ems_session *sess, AUTH_HDR *auth, dev_recv_cb h)
{
	ems_radius  *ra = (ems_radius *)dev->ctx;
	ems_int      rtn = EMS_OK;
	ems_int      length = 0;

	dev->auth_in = (AUTH_HDR *)buf_rd(&sess->buf_in);

	rtn = rc_check_reply(dev->auth_in, buf_size(&sess->buf_in), 
			(ems_char *)str_text(&ra->secret), 
			dev->vector, dev->auth_out->id);

	length = ntohs(dev->auth_in->length) - AUTH_HDR_LEN;
	if (length > 0) {
		dev->vp_in = rc_avpair_gen(ra->rh, NULL, dev->auth_in->data, length, 0);
	} else
		dev->vp_in = NULL;

	if (rtn != OK_RC)
		return EMS_ERR;

	if (h) {
		h(dev, sess);
		rtn = EMS_OK;
	}
	else
		rtn = EMS_ERR;

	return rtn;
}


static ems_int 
dev_preprocess(ems_device *dev, ems_session *sess, dev_recv_cb h)
{
	AUTH_HDR auth;
	ems_int  len = sizeof(auth) - 2;
	ems_int  rtn;

	if (buf_len(&sess->buf_in) < len)
		return EMS_CONTINUE;

	ems_buffer_prefetch(&sess->buf_in, (ems_char *)&auth, len);

	auth.length = ntohs(auth.length);

	if (auth.length >= buf_size(&sess->buf_in))
		return EMS_BUFFER_INSUFFICIENT;

	if (buf_len(&sess->buf_in) < auth.length)
		return EMS_CONTINUE;

	rtn = dev_process_rsp(dev, sess, &auth, h);

	ems_buffer_seek_rd(&sess->buf_in, auth.length, EMS_BUFFER_SEEK_CUR);
	ems_buffer_refresh(&sess->buf_in);

	return rtn;
}

static ems_int
ra_recv_msg(ems_device *dev, ems_session *sess, ems_uint flg, dev_recv_cb h)
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
				return dev_preprocess(dev, sess, h);
		}
	}

	do {
		ret = dev_preprocess(dev, sess, h);

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

static ems_int ra_device_firewall_set_rules(ems_device *dev, ems_int set)
{
	json_object *obj;

	obj = json_object_new_object();

	json_object_object_add(obj, "userip", json_object_new_string(str_text(&dev->user.ip)));
	json_object_object_add(obj, "usermac", json_object_new_string(str_text(&dev->user.mac)));
	json_object_object_add(obj, "add",    json_object_new_int(set));
	{
		ems_radius *ra = (ems_radius *)dev->ctx;
		nic_processmsg(ra->ssid, ty_radius, ty_fw, EMS_APP_FW_RADIUS_DEVICE_FREE, obj);
	}

	json_object_put(obj);
	return EMS_OK;
}

static ems_int ra_connect_to_acct_server(ems_device *dev, ems_session *sess, ems_radius *ra)
{
	json_object *rsp;
	ems_assert(dev && sess && dev->sess == sess);

	ems_sock_setaddr(&sess->sock,  str_text(&ra->acct_addr));
	ems_sock_setport(&sess->sock,  ra->acct_port);

	if (ra_connect(sess) != EMS_OK) {
		dev->reason = RADIUS_ERR_NETWORK; /* reset to error */
		return dev_change_status(dev, st_err);
	} 

	ra->lasterr = RADIUS_ERR_SUCCESS;
	rsp = json_object_new_object();

	json_object_object_add(rsp, "username",json_object_new_string(str_text(&dev->user.name)));
	json_object_object_add(rsp, "userip", json_object_new_string(str_text(&dev->user.ip)));
	json_object_object_add(rsp, "usermac", json_object_new_string(str_text(&dev->user.mac)));
	json_object_object_add(rsp, "error_code",json_object_new_int(0));
	{
		ems_radius *ra = (ems_radius *)dev->ctx;
		nic_sendmsg(ra->ssid, ty_radius, ty_portal, EMS_APP_CMD_RADIUS_AUTH_RSP, rsp);
	}

	json_object_put(rsp);

	ra_device_firewall_set_rules(dev, EMS_YES);

	/* RFC2866: 5.1 Acct-Status-Type, start/stop(1/2) */
	dev->reason = 1;
	return dev_change_status(dev, st_acct);
}

static ems_int ra_start_to_acct_user(ems_device *dev, ems_session *sess)
{
	ems_radius *ra = (ems_radius *)dev->ctx;

	ems_assert(dev && sess && dev->sess == sess);
	ems_l_trace("[radius]shutdown session(%d) [%s: %d]",
			ems_sock_fd(&sess->sock),
			ems_sock_addr(&sess->sock),
			ems_sock_port(&sess->sock));

	sess_event_cancel(sess);
	sess_timeout_cancel(sess);
	ems_sock_close(&sess->sock);

	if (dev->vp_out) {
		rc_avpair_free(dev->vp_out);
		dev->vp_out = NULL;
	}

	if (dev->vp_in) {
		rc_avpair_free(dev->vp_in);
		dev->vp_in = NULL;
	}

	dev->auth_out = NULL;
	dev->auth_in  = NULL;
	ems_buffer_clear(&sess->buf_out);
	ems_buffer_clear(&sess->buf_in);

	return ra_connect_to_acct_server(dev, sess, ra);
}

static ems_int ra_auth_rsp(ems_device *dev, ems_session *sess)
{
	AUTH_HDR    *auth = dev->auth_in;
	ems_radius  *ra  = (ems_radius *)dev->ctx;

	ems_assert(auth != NULL);

	if (auth->code == PW_ACCESS_ACCEPT) {
		ems_l_info("[radius %s]user: %s@%s login success", 
			str_text(&ra->ssid->_iface->bssid),
			str_text(&dev->user.name), str_text(&dev->user.ip));

		return ra_start_to_acct_user(dev, sess);
	} 
	else if (auth->code == PW_ACCESS_CHALLENGE) {
		ems_l_info("[radius %s]*******server need user: %s@%s access challenge", 
			str_text(&ra->ssid->_iface->bssid),
			str_text(&dev->user.name), str_text(&dev->user.ip));
		dev->reason = auth->code;
		dev_change_status(dev, st_err);
	}
	else {
		ems_l_info("[radius %s]user: %s@%s failed, code: %d", 
			str_text(&ra->ssid->_iface->bssid),
			str_text(&dev->user.name), str_text(&dev->user.ip), auth->code);
		dev->reason = RADIUS_ERR_REJECT;
		dev_change_status(dev, st_err);
	}
	return EMS_OK;
}

static ems_int ra_auth(ems_device *dev, ems_session *sess, ems_uint flg)
{
	if (ems_flag_like(flg, EMS_EVT_READ)) {
		if (ra_recv_msg(dev, sess, flg, ra_auth_rsp) != EMS_OK) {
			dev->reason = RADIUS_ERR_NETWORK;
			return dev_change_status(dev, st_err);
		}

		return EMS_OK;
	}

	if (ems_flag_like(flg, EMS_EVT_WRITE)) {
		if (ra_send_msg(dev, sess, flg) != EMS_OK) {
			dev->reason = RADIUS_ERR_NETWORK;
			return dev_change_status(dev, st_err);
		}

		return EMS_OK;
	}

	ems_assert(0 && "never be here");
	return EMS_OK;
}

static ems_int ra_acct_rsp(ems_device *dev, ems_session *sess)
{
	ems_radius  *ra   = (ems_radius *)dev->ctx;
	AUTH_HDR    *auth = dev->auth_in;

	ems_assert(auth != NULL);

	if (auth->code == PW_ACCOUNTING_RESPONSE) {
		ems_l_info("[radius %s]user(srv: %d, icmp: %d): %s@%s accounting success", 
			str_text(&ra->ssid->_iface->bssid), dev->disconnect_server, dev->disconnect,
			str_text(&dev->user.name), str_text(&dev->user.ip));

		dev->report_period = ra->report_period;

		sess_event_set(sess, EMS_EVT_READ,   dev_evt_cb);
		sess_timeout_set_sorted(sess, dev->report_period * 1000, dev_timeout_cb);

		dev_change_status(dev, st_normal);
		dev->disconnect_server  = ra->disconnect / (ra->report_period + 6) + 1;
	} else {
		ems_l_info("[radius %s]user: %s@%s acct failed, code: %d", 
			str_text(&ra->ssid->_iface->bssid),
			str_text(&dev->user.name), str_text(&dev->user.ip), auth->code);
		dev->err = 6; /* RFC2866, Admin reset */
		dev_change_status(dev, st_acct_stop);
	}

	return EMS_OK;
}

static ems_int ra_retry_connect_server(ems_device *dev, ems_session *sess)
{
	if (dev->disconnect_server-- > 0) {
		ems_l_trace("[radius]user: %s@%s accounting timeout, disconnect_server: %d", 
				str_text(&dev->user.name),
				str_text(&dev->user.ip),
				dev->disconnect_server);

		{
			ems_radius  *ra = (ems_radius *)dev->ctx;
			dev->report_period = ra->report_period;
		}

		sess_event_set(sess, EMS_EVT_READ,   dev_evt_cb);
		sess_timeout_set_sorted(sess, dev->report_period * 1000, dev_timeout_cb);

		return dev_change_status(dev, st_normal);
	}

	{
		ems_radius  *ra   = (ems_radius *)dev->ctx;
		ems_l_info("[radius %s] lost radius server: %s@%s", 
			str_text(&ra->ssid->_iface->bssid),
			str_text(&dev->user.name), str_text(&dev->user.ip));
	}

	return EMS_ERR;
}

static ems_int ra_acct(ems_device *dev, ems_session *sess, ems_uint flg)
{
	if (ems_flag_like(flg, EMS_EVT_READ)) {
		if (ra_recv_msg(dev, sess, flg, ra_acct_rsp) != EMS_OK) 
			goto err_out;
	}

	if (ems_flag_like(flg, EMS_EVT_WRITE)) {
		if (ra_send_msg(dev, sess, flg) != EMS_OK) 
			goto err_out;
	}

	return EMS_OK;

err_out:
	if (ra_retry_connect_server(dev, sess) != EMS_OK) {
		dev->reason = RADIUS_ERR_NETWORK;
		dev_change_status(dev, st_acct_stop);
	}

	return EMS_OK;
}

static ems_int ra_normal_rsp(ems_device *dev, ems_session *sess)
{
	ems_l_trace("[radius] normal got message from radius server, drop it sliently");
	return EMS_OK;
}

static ems_int ra_normal(ems_device *dev, ems_session *sess, ems_uint flg)
{
	if (ems_flag_like(flg, EMS_EVT_READ)) {
		if (ra_recv_msg(dev, sess, flg, ra_normal_rsp) != EMS_OK) {
			dev->reason = RADIUS_ERR_NETWORK;
			return dev_change_status(dev, st_acct_stop);
		}

		return EMS_OK;
	}

	ems_assert(0 && "never be here");
	return EMS_OK;
}

static ems_int ra_ping_recv_handle(ems_device *dev, ems_session *sess)
{
	struct ip   *ip;
	struct icmp *icp;
	int hlen, cc = buf_len(&sess->buf_in);

	ip   = (struct ip *) buf_rd(&sess->buf_in);
	hlen = ip->ip_hl << 2;

	if (cc < hlen + ICMP_MINLEN) 
		return EMS_OK;

	cc -= hlen;
	icp = (struct icmp *)(buf_rd(&sess->buf_in) + hlen);

	if(icp->icmp_type != ICMP_ECHOREPLY) {
		ems_l_trace("[radius] %d bytes from: %s icmp not reply(%d)",
				cc, ems_sock_addr(&sess->sock), icp->icmp_type);
		ems_buffer_clear(&sess->buf_in);
		return EMS_CONTINUE;
	}

	if( icp->icmp_id != dev->ident) {
		ems_l_trace("[radius] %d bytes icmp ident: %d error, not : %d", cc, icp->icmp_id, dev->ident);
		ems_buffer_clear(&sess->buf_in);
		return EMS_CONTINUE;
	}

	ems_l_trace("[radius]icmp replied by (%s), length: %d", ems_sock_addr(&sess->sock), cc);
	{
		/* reset disconnect count */
		ems_radius  *ra = (ems_radius *)dev->ctx;
		dev->disconnect  = ra->disconnect / (ra->report_period + 6) + 1;
	}

	ems_buffer_clear(&sess->buf_in);

	sess_event_cancel(sess);
	sess_timeout_cancel(sess);

	/* RFC2866, interim status  */
	dev->reason = 3;
	return dev_change_status(dev, st_acct);
}

static ems_int 
ra_ping_recv(ems_device *dev, ems_session *sess)
{
	ems_int   ret;
	ems_char *wr  = buf_wr(&sess->buf_in);
	ems_int   len = buf_left(&sess->buf_in);

again:
	ret = recv(ems_sock_fd(&sess->sock), wr, len, 0);

	if (ret <= 0) {
		switch(errno) {

		case EAGAIN:
		case EINTR:
			return EMS_OK;

		default:
			ems_l_trace("[radius] ping error: %s", ems_lasterrmsg());
			return EMS_ERR;
		}
	}

	ems_buffer_seek_wr(&sess->buf_in, ret, EMS_BUFFER_SEEK_CUR);

	ret = ra_ping_recv_handle(dev, sess);

	goto again;

	return EMS_OK;
}


static ems_int ra_ping (ems_device *dev, ems_session *sess, ems_uint flg)
{
	if (ems_flag_like(flg, EMS_EVT_READ)) {
		if (ra_ping_recv(dev, sess) != EMS_OK) {
			dev->err =  2; /* RFC2866 */
			return dev_change_status(dev, st_acct_stop);
		}

		return EMS_OK;
	}

	if (ems_flag_like(flg, EMS_EVT_WRITE)) {
		if (ra_send_msg(dev, sess, flg) != EMS_OK) {
			dev->err =  2; /* RFC2866 */
			return dev_change_status(dev, st_acct_stop);
		}
	}

	return EMS_OK;
}

static ems_int ra_acct_stopped_rsp(ems_device *dev, ems_session *sess)
{
	AUTH_HDR    *auth = dev->auth_in;
	ems_radius  *ra   = (ems_radius *)dev->ctx;

	ems_assert(auth != NULL);

	if (auth->code == PW_ACCOUNTING_RESPONSE) {
		ems_l_info("[radius %s] user: %s@%s accounting stopped success", 
			str_text(&ra->ssid->_iface->bssid),
			str_text(&dev->user.name), str_text(&dev->user.ip));
	}
	else {
		ems_l_info("[radius %s] user: %s@%s accounting stopped failed. code: %d", 
			str_text(&ra->ssid->_iface->bssid),
			str_text(&dev->user.name), str_text(&dev->user.ip), auth->code);
	}

	dev->reason = 0;
	return dev_change_status(dev, st_err);
}

static ems_int ra_acct_stopped(ems_device *dev, ems_session *sess, ems_uint flg)
{
	if (ems_flag_like(flg, EMS_EVT_READ)) {
		if (ra_recv_msg(dev, sess, flg, ra_acct_stopped_rsp) != EMS_OK) {
			dev->reason = RADIUS_ERR_NETWORK;
			return dev_change_status(dev, st_err);
		}

		return EMS_OK;
	}

	if (ems_flag_like(flg, EMS_EVT_WRITE)) {
		if (ra_send_msg(dev, sess, flg) != EMS_OK) {
			dev->reason = RADIUS_ERR_NETWORK;
			return dev_change_status(dev, st_err);
		}

		return EMS_OK;
	}

	ems_assert(0 && "never be here");
	return EMS_OK;
}

static ems_int ra_err(ems_device *dev, ems_session *sess, ems_uint flg)
{
	ems_radius  *ra = (ems_radius *)dev->ctx;
	ems_assert(dev);

	ra->lasterr = dev->reason;
	{
		json_object *rsp;
		rsp = json_object_new_object();

		if (dev->reason == 0)
			dev->reason = 4;

		json_object_object_add(rsp, "username",  json_object_new_string(str_text(&dev->user.name)));
		json_object_object_add(rsp, "userip",    json_object_new_string(str_text(&dev->user.ip)));
		json_object_object_add(rsp, "usermac",   json_object_new_string(str_text(&dev->user.mac)));
		json_object_object_add(rsp, "error_code",json_object_new_int(dev->reason));

		nic_sendmsg(ra->ssid, ty_radius, ty_portal, EMS_APP_CMD_RADIUS_AUTH_RSP, rsp);
		json_object_put(rsp);
	}
	ra_device_firewall_set_rules(dev, EMS_NO);

	ems_l_info("[radius] shutdown device: %s, %s, %s, %s", 
			str_text(&dev->user.name),
			str_text(&dev->user.pass),
			str_text(&dev->user.ip),
			str_text(&dev->user.mac));

	ems_queue_remove(&dev->entry);
	ems_device_destroy(dev);

	return EMS_OK;
}

static ems_int ra_to_auth(ems_device *dev, ems_session *sess, ems_timeout *to)
{
	dev->retry_times--;
	ems_l_trace("[radius] radius auth timeout, retry times %d left", dev->retry_times);
	if (dev->retry_times > 0) {
		ems_buffer_seek_rd(&sess->buf_out, 0, EMS_BUFFER_SEEK_SET);
		sess_event_set(sess, EMS_EVT_WRITE, dev_evt_cb);
		sess_timeout_set_sorted(sess, dev->retry_timeout * 1000, dev_timeout_cb);
		return EMS_OK;
	}

	dev->reason = RADIUS_ERR_CANNOT_CONNECT;
	return dev_change_status(dev, st_err);
}

static ems_int ra_to_acct(ems_device *dev, ems_session *sess, ems_timeout *to)
{
	dev->retry_times--;
	if (dev->retry_times > 0) {
		ems_buffer_seek_rd(&sess->buf_out, 0, EMS_BUFFER_SEEK_SET);
		sess_event_set(sess, EMS_EVT_WRITE, dev_evt_cb);
		sess_timeout_set_sorted(sess, dev->retry_timeout * 1000, dev_timeout_cb);
		return EMS_OK;
	}

	if (ra_retry_connect_server(dev, sess) != EMS_OK) {
		dev->reason = RADIUS_ERR_CANNOT_CONNECT;
		return dev_change_status(dev, st_acct_stop);
	}

	return EMS_OK;
}

static ems_int ra_to_normal(ems_device *dev, ems_session *sess, ems_timeout *to)
{
	ems_l_trace("[radius] time to report status: %s@%s", 
			str_text(&dev->user.name),
			str_text(&dev->user.ip));
	return dev_change_status(dev, st_hb);
}

static ems_int ra_to_ping(ems_device *dev, ems_session *sess, ems_timeout *to)
{
	ems_l_trace("[radius] timeout dectect ping(%s) retry times: %d, disconnect: %d", 
			str_text(&dev->user.ip), dev->retry_times, dev->disconnect);

	sess_event_cancel(sess);

	if (dev->retry_times > 0) {
		dev->retry_times--;
		ems_l_trace("[radius] retry send ping: %d", dev->retry_times);
		ems_buffer_seek_rd(&sess->buf, 0, EMS_BUFFER_SEEK_SET);
		sess_event_set(sess,   EMS_EVT_WRITE, dev_evt_cb);
		sess_timeout_set_sorted(sess, PING_TIMEOUT,  dev_timeout_cb);
		return EMS_OK;
	}

	dev->disconnect--;
	if (dev->disconnect <= 0) {
		ems_radius  *ra  = (ems_radius *)dev->ctx;
		ems_l_info("[radius %s] lost user: %s@%s", 
			str_text(&ra->ssid->_iface->bssid),
			str_text(&dev->user.name), str_text(&dev->user.ip));

		dev->err = 2; /* lost user */
		return dev_change_status(dev, st_acct_stop);
	}

	/* just continue */
	ems_buffer_clear(&sess->buf_in);
	sess_event_cancel(sess);
	sess_timeout_cancel(sess);

	/* RFC2866, interim status  */
	dev->reason = 3;
	return dev_change_status(dev, st_acct);
}

static ems_int ra_to_acct_stopped(ems_device *dev, ems_session *sess, ems_timeout *to)
{
	dev->retry_times--;
	if (dev->retry_times) {
		ems_buffer_seek_rd(&sess->buf_out, 0, EMS_BUFFER_SEEK_SET);
		sess_event_set(sess, EMS_EVT_WRITE, dev_evt_cb);
		sess_timeout_set_sorted(sess, dev->retry_timeout * 1000, dev_timeout_cb);
		return EMS_OK;
	}

	dev->reason = RADIUS_ERR_CANNOT_CONNECT;
	return dev_change_status(dev, st_err);
}

static ems_int ra_update_user_traffic(ems_device *dev)
{
	off_t src_pkgs, src_bytes, dst_pkgs, dst_bytes;
	ems_char buf[256] = {0};

	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "%s", 
		ems_popen_get("iptaccount -s -f -l " FW_PREFIX "%s | grep %s | cut -d';' -f2-",
			str_text(&dev->user.mac), str_text(&dev->user.ip)));

	if (strlen(buf) > 0) {
#ifndef GENERIC_LINUX
		sscanf(buf, "%lld;%lld;%lld;%lld", &src_pkgs, &src_bytes, &dst_pkgs, &dst_bytes);
#else
		sscanf(buf, "%ld;%ld;%ld;%ld", &src_pkgs, &src_bytes, &dst_pkgs, &dst_bytes);
#endif

		dev->user.in_bytes  += dst_bytes;
		dev->user.in_pkgs   += dst_pkgs;
		dev->user.out_bytes += src_bytes;
		dev->user.out_pkgs  += src_pkgs;
	}

	return EMS_OK;
}

static ems_int ra_fill_acct_avpairs(ems_device *dev, ems_radius *ra)
{
	static ems_char  buf[253] = {0};
	static ems_char  mac[32]  = {0};
	ems_uint  tmp;
	ems_session *sess = dev->sess;

	ems_assert(dev && sess != NULL);

	ems_rc_avpair_add(ra->rh, &dev->vp_out, PW_USER_NAME, 
		str_text(&dev->user.name), str_len(&dev->user.name), 0);

	if (sess) {
		struct sockaddr_in addr;
		socklen_t  len;

		memset(&addr, 0, sizeof(addr));
		len = sizeof(addr);
		getsockname(ems_sock_fd(&sess->sock), (struct sockaddr *)&addr, &len);

		tmp = ntohl(addr.sin_addr.s_addr);
		ems_rc_avpair_add(ra->rh, &dev->vp_out, PW_NAS_IP_ADDRESS, &tmp, 0, 0);
	}

	tmp = 0;
	ems_rc_avpair_add(ra->rh, &dev->vp_out, PW_NAS_PORT, &tmp, -1, 0);

	/* rfc2865-- 5.41 NAS-Port-Type--> wireless IEEE 802.11 */
	tmp = 19; 
	ems_rc_avpair_add(ra->rh, &dev->vp_out, PW_NAS_PORT_TYPE, &tmp, -1, 0);

	tmp = ntohl(inet_addr(str_text(&dev->user.ip)));
	ems_rc_avpair_add(ra->rh, &dev->vp_out, PW_FRAMED_IP_ADDRESS, &tmp, 0, 0);

	/* called station id, format: apmac:apssid */
	{
		ems_wifi_iface *iface = ra->ssid->_iface;
		ems_mac_update(mac, str_text(&iface->bssid));
		snprintf(buf, sizeof(buf), "%s:%s", mac, str_text(&iface->ssid));
	}
	ems_rc_avpair_add(ra->rh, &dev->vp_out, PW_CALLED_STATION_ID, buf, strlen(buf), 0);

	memset(mac, 0, 32);
	ems_mac_update(mac, str_text(&dev->user.mac));
	ems_rc_avpair_add(ra->rh, &dev->vp_out, PW_CALLING_STATION_ID, mac, strlen(mac), 0);

	snprintf(buf, sizeof(buf), "%s", core_sn());
	ems_rc_avpair_add(ra->rh, &dev->vp_out, PW_NAS_IDENTIFIER, buf, strlen(buf), 0);

	if (str_len(&dev->user.sessid) <= 0) {
		gettimeofday(&dev->user.start, NULL);

		snprintf(buf, sizeof(buf), FW_PREFIX "%u%ld%ld", 
				ems_getpid(), dev->user.start.tv_sec, dev->user.start.tv_usec);
		str_set(&dev->user.sessid, buf);
	} else
		snprintf(buf, sizeof(buf), "%s", str_text(&dev->user.sessid));

	ems_rc_avpair_add(ra->rh, &dev->vp_out, PW_ACCT_SESSION_ID, buf, strlen(buf), 0);

	tmp = dev->reason;
	ems_rc_avpair_add(ra->rh, &dev->vp_out, PW_ACCT_STATUS_TYPE, &tmp, 0, 0);

	switch(dev->reason) {
	case 1: /* acct start*/
		break;

	case 2: /* acct stop */
	{
		struct timeval tv;
		gettimeofday(&tv, NULL);

		tmp = abs(tv.tv_sec - dev->user.start.tv_sec);
		ems_rc_avpair_add(ra->rh, &dev->vp_out, PW_ACCT_SESSION_TIME, &tmp, 0, 0);

		tmp = dev->reason;
		ems_rc_avpair_add(ra->rh, &dev->vp_out, PW_ACCT_TERMINATE_CAUSE, &tmp, 0, 0);
	}       
	       /* no break here*/
	case 3: /* acct interim, just update status */
	{
		ems_uint b, g;

		ra_update_user_traffic(dev);

		b   = (dev->user.in_bytes & 0x00ffffffff);
		g   = (dev->user.in_bytes >> 32);
		tmp =  dev->user.in_pkgs;
		ems_rc_avpair_add(ra->rh, &dev->vp_out, PW_ACCT_INPUT_OCTETS,    &b, 0, 0);
		ems_rc_avpair_add(ra->rh, &dev->vp_out, PW_ACCT_INPUT_GIGAWORDS, &g, 0, 0);
		ems_rc_avpair_add(ra->rh, &dev->vp_out, PW_ACCT_INPUT_PACKETS,   &tmp, 0, 0);

		b   = (dev->user.out_bytes & 0x00ffffffff);
		g   = (dev->user.out_bytes >> 32);
		tmp =  dev->user.out_pkgs;
		ems_rc_avpair_add(ra->rh, &dev->vp_out, PW_ACCT_OUTPUT_OCTETS,   &b, 0, 0);
		ems_rc_avpair_add(ra->rh, &dev->vp_out, PW_ACCT_OUTPUT_GIGAWORDS,&g, 0, 0);
		ems_rc_avpair_add(ra->rh, &dev->vp_out, PW_ACCT_OUTPUT_PACKETS,  &tmp, 0, 0);
	}
	       break;

	default:
		break;
	}

	return EMS_OK;
}

static ems_int ra_fill_acct_info(ems_device *dev, ems_radius *ra)
{
	ems_int      total_len;
	ems_session *sess = dev->sess;
	AUTH_HDR    *auth;

	ems_buffer_clear(&sess->buf_out);

	auth = (AUTH_HDR *)buf_wr(&sess->buf_out);
	auth->code = PW_ACCOUNTING_REQUEST; /* RFC2866 */
	auth->id   = ra->seq_nbr++;
	dev->auth_out = auth;

	if (dev->vp_out) {
		rc_avpair_free(dev->vp_out);
		dev->vp_out = NULL;
	}

	if (dev->vp_in) {
		rc_avpair_free(dev->vp_in);
		dev->vp_in = NULL;
	}

	ra_fill_acct_avpairs(dev, ra);

	total_len    = rc_pack_list(dev->vp_out, str_text(&ra->secret), auth) + AUTH_HDR_LEN;
	auth->length = ntohs((unsigned short) total_len);

	/* RFC2866, page 7, Request Authenticator */
	memset(auth->vector, 0, AUTH_VECTOR_LEN);
	memcpy((ems_char *)auth + total_len, str_text(&ra->secret), str_len(&ra->secret));

	rc_md5_calc(dev->vector, (ems_uchar *)auth, total_len + str_len(&ra->secret));
	memcpy(auth->vector, dev->vector, AUTH_VECTOR_LEN);

	ems_buffer_seek_wr(&sess->buf_out, total_len, EMS_BUFFER_SEEK_CUR);
	dev->total_len = total_len;

	return EMS_OK;
}

static ems_int st_into_acct(ems_device *dev)
{
	ems_radius  *ra   = (ems_radius *)dev->ctx;
	ems_session *sess = dev->sess;

	ems_assert(dev && ra && dev->sess);

	ra_fill_acct_info(dev, ra);

	dev->retry_timeout = ra->retry_timeout;
	dev->retry_times   = ra->retry_times;

	sess_event_set(sess, EMS_EVT_WRITE, dev_evt_cb);
	sess_timeout_set_sorted(sess, dev->retry_timeout * 1000, dev_timeout_cb);

	return EMS_OK;
}

static ems_int st_into_acct_stop(ems_device *dev)
{
	ems_radius  *ra   = (ems_radius *)dev->ctx;
	ems_session *sess = dev->sess;

	ems_assert(dev && ra && dev->sess);

	switch(dev->reason) {
	case RADIUS_ERR_NETWORK:
	case RADIUS_ERR_CANNOT_CONNECT:
		break;

	default:
		/* RFC2866: 5.1 Acct-Status-Type, start/stop(1/2) */
		dev->reason = 2;
		ra_fill_acct_info(dev, ra);

		dev->retry_timeout = ra->retry_timeout;
		dev->retry_times   = ra->retry_times;

		sess_event_set(sess, EMS_EVT_WRITE, dev_evt_cb);
		sess_timeout_set_sorted(sess, dev->retry_timeout * 1000, dev_timeout_cb);

		break;
	}

	/* send msg to portal, to disconnect with */
	{
		json_object *jobj;
		jobj = json_object_new_object();

		json_object_object_add(jobj, "username",json_object_new_string(str_text(&dev->user.name)));
		json_object_object_add(jobj, "userip",  json_object_new_string(str_text(&dev->user.ip)));
		json_object_object_add(jobj, "usermac", json_object_new_string(str_text(&dev->user.mac)));
		json_object_object_add(jobj, "error_code",json_object_new_int(dev->reason));

		nic_sendmsg(ra->ssid, ty_radius, ty_portal, EMS_APP_CMD_PORTAL_LOGOUT, jobj);
		json_object_put(jobj);
	}

	ra_device_firewall_set_rules(dev, EMS_NO);

	if (dev->reason != 2)
		return dev_change_status(dev, st_err);

	return EMS_OK;
}

static ems_int ra_ping_connect(ems_session *sess)
{
	struct protoent *proto;
	ems_sock        *sock = &sess->sock;
	socklen_t        len;
	struct sockaddr_in addr;
	ems_int   fd;

	memset(&addr, 0, sizeof(addr));

	if (ems_gethostbyname(ems_sock_addr(sock), &addr) != OK) {
		ems_l_trace("[radius]gethostbyename failed %s : %s", 
				ems_sock_addr(sock), ems_lasterrmsg());
		return EMS_ERR;
	}

	if ((proto = getprotobyname("icmp")) == NULL) {
		ems_l_trace("[radius]getprotobyname failed %s : %s", 
				ems_sock_addr(sock), ems_lasterrmsg());
		return EMS_ERR;
	}

	if ((fd = socket(AF_INET, SOCK_RAW, proto->p_proto)) < 0) {
		ems_l_trace("[radius]socket error: %s", ems_lasterrmsg());
		return EMS_ERR;
	}

	addr.sin_family = AF_INET;
	len = sizeof(struct sockaddr_in);

	if (connect(fd, (struct sockaddr *)&addr, len)) {
		close(fd);
		ems_l_trace("[radius]connect error: %s", ems_lasterrmsg());
		return EMS_ERR;
	}

	ems_sock_setfd(&sess->sock, fd);

	ems_l_trace("[radius] sess: %d host addr: %s", 
			ems_sock_fd(&sess->sock), ems_sock_addr(&sess->sock));

	return EMS_OK;
}


static ems_int st_ping_prepare_sess(ems_device *dev)
{
	ems_session *sess = NULL;

	if (!dev->sess_dev) {
		dev->sess_dev = ems_session_new();
		if (!dev->sess_dev) {
			dev->err = 2;
			return dev_change_status(dev, st_acct_stop);
		}

		sess_cbarg_set(dev->sess_dev, dev);
	}

	sess = dev->sess_dev;

	ems_buffer_clear(&sess->buf);
	ems_buffer_clear(&sess->buf_in);

	dev->ident  = random() & 0xFFFF;
	dev->ntrans = 0;

	ems_sock_setaddr(&sess->sock, str_text(&dev->user.ip));

	return ra_ping_connect(sess);
}

static u_short ra_ping_chksum(u_short *addr, ems_int len)
{
	u_short answer, *w = addr;
	ems_int sum = 0, nleft = len;

	/*
	 *  Our algorithm is simple, using a 32 bit accumulator (sum),
	 *  we add sequential 16 bit words to it, and at the end, fold
	 *  back all the carry bits from the top 16 bits into the lower
	 *  16 bits.
	 */
	while( nleft > 1 )  {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if( nleft == 1 ) {
		u_short	u = 0;

		*(u_char *)(&u) = *(u_char *)w ;
		sum += u;
	}

	/*
	 * add back carry outs from top 16 bits to low 16 bits
	 */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */

	return answer;
}

static ems_int st_into_ping(ems_device *dev)
{
	int i, cc, len;;
	struct icmp    *icp;
	struct timeval *tp;
	ems_char       *wr;
	ems_session    *sess = NULL;


	if (!dev->sess_dev) {
		if (st_ping_prepare_sess(dev) != EMS_OK) {
			dev->err =  2; /* RFC2866 */
			dev->reason = 2;
			return dev_change_status(dev, st_acct_stop);
		}
	}

	sess = dev->sess_dev;
	ems_assert(sess != NULL);

	ems_buffer_reset(&sess->buf);

	icp = (struct icmp *)    (buf_wr(&sess->buf));
	tp  = (struct timeval *) (buf_wr(&sess->buf) + 8);
	wr  = (ems_char *)       (buf_wr(&sess->buf) + 8 + sizeof(struct timeval));

	icp->icmp_type  = ICMP_ECHO;
	icp->icmp_code  = 0;
	icp->icmp_cksum = 0;
	icp->icmp_seq   = htons(dev->ntrans++);
	icp->icmp_id    = dev->ident;

	len = 56;
	cc  = len + 8;
	gettimeofday(tp, NULL);

	for(i = 8; i < len; i++)
		*wr++ = i;

	icp->icmp_cksum = ra_ping_chksum((u_short *)icp, cc);

	dev->retry_times = PING_RETRYTIMES;

	ems_buffer_seek_wr(&sess->buf, cc, EMS_BUFFER_SEEK_CUR);
	sess_event_set(sess,   EMS_EVT_WRITE, dev_evt_cb);
	sess_timeout_set_sorted(sess, PING_TIMEOUT,  dev_timeout_cb);

	return EMS_OK;
}

ems_int dev_change_status(ems_device *dev, ems_status st)
{
	if (dev->st == st)
		return EMS_OK;

	ems_l_trace("[radius] dev(%s) change status: %s --> %s", 
		str_text(&dev->user.ip), ems_status_str(dev->st), ems_status_str(st));

	dev->st = st;

	switch(dev->st) {
	case st_start:
	case st_err:
		return dev_evt_run(dev, NULL, 0);
		break;

	case st_acct:
		return st_into_acct(dev);

	case st_acct_stop:
		return st_into_acct_stop(dev);

	case st_hb:
		st_into_ping(dev);
		break;

	default:
		break;
	}

	return EMS_OK;
}

