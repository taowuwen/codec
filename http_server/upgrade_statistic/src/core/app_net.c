

#include "ems_core.h"
#include "ems_client.h"
#include "app.h"
#include "ems_netcheck.h"

static ems_int 
dns_parse_and_rsp(netcheck *ping, ems_session *sess, struct sockaddr_in *from)
{
	ems_int   ret;
	ems_char *qa;
	ems_char *ra;
	ems_char *n;
	in_addr_t addr;

	ems_char buf[] = {
		0xc0, 0x0c, 0x00, 0x01, 0x00, 0x01, 
		0x00, 0x00, 0x00, 0x00, 0x00, 0x04
	};

#if 0
	//ems_cchar *gw;
	gw = cfg_get(emscfg(), CFG_lan_addr);
	if (!gw) {
		ems_assert(0 && "should never be here");
		return EMS_ERR;
	}
#endif

	addr = inet_addr("192.168.19.87");
	ems_buffer_write(&sess->buf, buf_rd(&sess->buf_in), buf_len(&sess->buf_in));
	ems_buffer_write(&sess->buf, buf,   sizeof(buf));
	ems_buffer_write(&sess->buf, (ems_char *)&addr, sizeof(addr));

	qa = buf_rd(&sess->buf) + 2;
	ra = buf_rd(&sess->buf) + 3;
	 n = buf_rd(&sess->buf) + 7;

	*qa |= 0x84;
	*ra |= 0x80;
	*n  |= 0x01;

	ret = sendto(ems_sock_fd(&sess->sock),
			buf_rd(&sess->buf), 
			buf_len(&sess->buf), 
			0, 
			(struct sockaddr *)from, 
			sizeof(struct sockaddr_in));

	ems_buffer_clear(&sess->buf);
	if ( ret <= 0)
		return EMS_ERR;

	return EMS_OK;
}

static ems_int dns_redirect(netcheck *ping, ems_session *sess)
{
	socklen_t  len;
	struct sockaddr_in from;
	int ret;
	ems_buffer  *buf = &sess->buf_in;

	ems_assert(ping->sess_dns == sess);

	len = sizeof(from);
again:
	ret = recvfrom(ems_sock_fd(&sess->sock), buf_wr(buf), buf_left(buf), 0,
			(struct sockaddr *)&from, &len);

	if (ret <= 0) {
		return EMS_ERR;
	}

	ems_buffer_seek_wr(buf, ret, EMS_BUFFER_SEEK_CUR);

	dns_parse_and_rsp(ping, sess, &from);

	ems_buffer_clear(buf);

	goto again;

	return EMS_OK;
}

static ems_void dns_evt_cb(ems_session *sess, ems_int err, ems_int flg)
{
	netcheck *ping = (netcheck *)sess_cbarg(sess);

	if (err) {
		ems_l_trace("[ping] dns err sess: %d", ems_sock_fd(&sess->sock));
		ems_session_shutdown_and_destroy(ping->sess_dns);
		ping->sess_dns = NULL;
		return;
	}

	if (ems_flag_like(flg, EMS_EVT_READ)) {
		dns_redirect(ping, sess);
		return;
	}
}

static ems_int dns_bind(ems_session *sess)
{
	int fd, rc, opt = 1;
	struct sockaddr_in addr;
	ems_sock *sock = &sess->sock;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		ems_l_trace("[ping]socket err: %s", ems_lasterrmsg());
		return EMS_ERR;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
	{
		ems_l_trace("[ping]setsockopt err: %s", ems_lasterrmsg());
		close(fd);
		return EMS_ERR;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port   = htons(DNS_INTERCEPT_PORT);
	addr.sin_addr.s_addr = htonl(0);

	if ((rc = bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in))) == -1)
	{
		ems_l_trace("err bind: %s", ems_lasterrmsg());
		close(fd);
		return EMS_ERR;
	}

	ems_sock_setaddr(sock, "0.0.0.0");
	ems_sock_setfd(sock, fd);
	return EMS_OK;
}

static ems_int dns_intercept_start(netcheck *ping)
{
	ems_session *sess;
	ems_assert(ping != NULL);

	ping->sess_dns = ems_session_new();
	if (!ping->sess_dns)
		return EMS_ERR;

	if (dns_bind(ping->sess_dns) != EMS_OK) {
		ems_session_shutdown_and_destroy(ping->sess_dns);
		ping->sess_dns = NULL;
		return EMS_ERR;
	}

	sess = ping->sess_dns;
	sess_cbarg_set(sess, ping);
	sess_event_set(sess, EMS_EVT_READ, dns_evt_cb);

	ems_l_trace("[ping] dns server (%d) %s", 
			ems_sock_fd(&sess->sock), ems_sock_addr(&sess->sock));

	return EMS_OK;
}

static ems_int dns_intercept_stop(netcheck *ping)
{
	ems_assert(ping != NULL);

	if (ping->sess_dns) {
		ems_session_shutdown_and_destroy(ping->sess_dns);
		ping->sess_dns = NULL;
	}

	return EMS_OK;
}

netcheck *net_check_new()
{
	return (netcheck *)ems_malloc(sizeof(netcheck));
}

ems_void  net_check_destroy(netcheck *ping)
{
	if (ping)
		ems_free(ping);
}

static ems_int net_init(app_module *mod)
{
	netcheck *ping = NULL;

	ping = net_check_new();

	if (ping) {
		memset(ping, 0, sizeof(netcheck));

		ping->st   = st_stopped;
		ping->sess = NULL;
		ping->sess_dns = NULL;

		mod->ctx = (ems_void *)ping;
	}

	return EMS_OK;
}

static ems_int net_uninit(app_module *mod)
{
	netcheck *ping = (netcheck *)mod->ctx;

	if (!ping)
		return EMS_OK;

	mod->ctx = NULL;

	ems_assert(ems_flag_unlike(mod->flg, FLG_RUN));
	ems_assert(ping->st == st_stopped);
	ems_assert(ping->sess_dns == NULL);

	if (ping->st != st_stopped) {
		ping_change_status(ping, st_stopped);
		ping->st = st_stopped;
	}

	dns_intercept_stop(ping);
	ping->sess     = NULL;
	ping->sess_dns = NULL;

	ems_free(ping);

	return EMS_OK;
}

static ems_int net_run(app_module *mod, ems_int run)
{
	netcheck  *ping = (netcheck *)mod->ctx;

	ems_assert(ping);

	if (run) {
		if (ems_flag_like(mod->flg, FLG_RUN))
			return EMS_OK;

		ems_l_trace("net detector start");
		ems_flag_set(mod->flg, FLG_RUN);
		ping_change_status(ping, st_start);
	} else {
		if (ems_flag_unlike(mod->flg, FLG_RUN))
			return EMS_OK;

		ems_l_trace("net detector stopping");
		ems_flag_unset(mod->flg, FLG_RUN);
		ping_change_status(ping, st_stopped);
	}

	return EMS_OK;
}


static ems_int 
net_dns_intercept_run(app_module *mod, ems_int run)
{
	netcheck *ping = (netcheck *)mod->ctx;

	if (run) {
		if (ems_flag_like(mod->flg, FLG_NET_DNS_RUN))
			return EMS_OK;

		ems_l_trace("net dns intercept start");
		if (dns_intercept_start(ping) == EMS_OK) {
			ems_flag_set(mod->flg, FLG_NET_DNS_RUN);
			return EMS_OK;
		}
	} else {
		if (ems_flag_unlike(mod->flg, FLG_NET_DNS_RUN))
			return EMS_OK;

		ems_l_trace("net dns intercept stopped");
		ems_flag_unset(mod->flg, FLG_NET_DNS_RUN);
		return dns_intercept_stop(ping);
	}

	return EMS_ERR;
}

static ems_int
net_process(app_module *mod, ems_uint s, ems_uint d, ems_uint evt, json_object *root)
{
	ems_l_trace("net evt: 0x%x, from: 0x%x, args: %s", 
			evt, s, root?json_object_to_json_string(root):"");

	switch(evt) {
	case EMS_APP_START:
		return net_run(mod, EMS_YES);

	case EMS_APP_STOP:
		return net_run(mod, EMS_NO);

	case EMS_APP_DNS_INTERCEPT_STOP:
		net_dns_intercept_run(mod, EMS_NO);
		break;

	default:
		break;
	}

	if (ems_flag_unlike(mod->flg, FLG_RUN)) {
		ems_l_trace("net, current not running");
		return EMS_ERR;
	}

	switch(evt) {
	case EMS_APP_DNS_INTERCEPT_START:
		net_dns_intercept_run(mod, EMS_YES);
		break;

	default:
		break;
	}

	return EMS_OK;
}


app_module app_net = 
{
	.ty      = ty_net,
	.desc    = ems_string("netdetect"),
	.ctx     = NULL,
	.flg     = 0,

	.init    = net_init,
	.uninit  = net_uninit,
	.run     = net_run,
	.process = net_process,
	.process_rule  = NULL,
	.version_match = NULL,
	.install       = NULL
};
