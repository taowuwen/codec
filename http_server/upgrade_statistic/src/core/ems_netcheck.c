
#include "ems_core.h"
#include "ems_client.h"
#include "app.h"
#include "ems_netcheck.h"
#include "ems_bridge.h"


#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>


#define PING_SEND_TIMEOUT	2000
#define PING_IDLE_TIMEOUT	5000

static ems_char *net_addrs[] =
{
	"8.8.8.8",
	NULL
};

static ems_int ping_start  (netcheck *ping, ems_session *sess, ems_uint flg);
static ems_int ping_stopped(netcheck *ping, ems_session *sess, ems_uint flg);
static ems_int ping_normal (netcheck *ping, ems_session *sess, ems_uint flg);
static ems_int ping_ping   (netcheck *ping, ems_session *sess, ems_uint flg);
static ems_int ping_err    (netcheck *ping, ems_session *sess, ems_uint flg);

typedef ems_int (*ping_evt_func)(netcheck *ping, ems_session *sess, ems_uint flg);
static ping_evt_func ping_evt_handler[] = 
{
	[st_start]   = ping_start,
	[st_stopped] = ping_stopped,
	[st_normal]  = ping_normal,
	[st_hb]      = ping_ping, 
	[st_err]     = ping_err,
	[st_max]     = NULL
};


static ems_int ping_to_normal(netcheck *ping, ems_session *sess, ems_timeout *to);
static ems_int ping_to_ping  (netcheck *ping, ems_session *sess, ems_timeout *to);
static ems_int ping_to_err   (netcheck *ping, ems_session *sess, ems_timeout *to);

typedef ems_int (*ping_timeout_func)(netcheck *ping, ems_session *sess, ems_timeout *to);
static ping_timeout_func ping_timeout_handler[] = 
{
	[st_start]   = NULL,
	[st_stopped] = NULL,
	[st_normal]  = ping_to_normal,
	[st_hb]      = ping_to_ping, 
	[st_err]     = ping_to_err,
	[st_max]     = NULL
};

static ems_int  ping_evt_run(netcheck *ping, ems_session *sess,  ems_uint flg);
static ems_void ping_evt_cb(ems_session *sess, ems_int err, ems_int flg);
static ems_void ping_timeout_cb(ems_session *sess, ems_timeout *to);

static ems_int fw_redirect(ems_int re, ems_cchar *gw, ems_cchar *pro, ems_int s_port, ems_int d_port)
{
	ems_char   *ac = NULL;
	ems_int     rtn = EMS_OK;

	ems_char   *buf = buf_wr(core_buffer());
	ems_int     len = buf_left(core_buffer());

	ac = "-I";
	if (!re)
		ac = "-D";

	snprintf(buf, len,
"iptables -w -i %s -t nat %s PREROUTING -p %s ! -d %s --dport %d -j DNAT --to-destination %s:%d",
core_gw_ifname(), ac, pro, gw, s_port, gw, d_port);

	rtn = ems_systemcmd(buf);

	{
		if (re)
			ems_systemcmd("uci -p /tmp/state set ykwifi.base.status=1");
		else
			ems_systemcmd("uci -p /tmp/state del ykwifi.base.status");
	}

	return rtn;
}

static ems_int fw_redirect_v2(ems_int re, ems_cchar *gw, ems_cchar *pro, ems_int s_port, ems_int d_port)
{
	ems_char  *ac = NULL;
	ems_int    rtn = EMS_OK;
	ems_char  *buf = buf_wr(core_buffer());
	ems_int    len = buf_left(core_buffer());

	ac = "-I";
	if (!re)
		ac = "-D";

	snprintf(buf, len,
"iptables -w -i %s -t nat %s PREROUTING -p %s --dport %d -j DNAT --to-destination %s:%d",
core_gw_ifname(), ac, pro, s_port, gw, d_port);

	rtn = ems_systemcmd(buf);

	return rtn;
}

static ems_int fw_redirect_web(ems_int re, ems_cchar *gw)
{
	return fw_redirect(re, gw, "tcp", 80, EMS_PORT);
}

static ems_int fw_redirect_dns(ems_int re, ems_cchar *gw)
{
	ems_uint evt = EMS_APP_DNS_INTERCEPT_START;

	if (!re)
		evt = EMS_APP_DNS_INTERCEPT_STOP;

	ems_app_process(ty_net, ty_net, evt, NULL);

	return fw_redirect_v2(re, gw, "udp", 53, DNS_INTERCEPT_PORT);
}

static ems_int ping_fw_redirect(netcheck *ping, ems_int redirect)
{
	static ems_int  re = -1;
	static ems_char buf[256] = {0};
	ems_cchar *gw = NULL;

	if ((re == redirect) || ems_flag_like(emscorer()->flg, FLG_NETWORK_BRIDGE))
		return EMS_OK;

	{
		ems_char cmd[256] = {0};
		snprintf(cmd, sizeof(cmd), 
			"ip route | grep %s | awk '{print $5}'", core_gw_ifname());

		snprintf(buf, sizeof(buf), "%s", ems_popen_get(cmd));
		if (strlen(buf) <= 0)
			return EMS_OK;
	}

	gw = core_gw_addr();

	if (strcmp(gw, buf)) {
		ems_l_trace("gw changed from : %s into: %s", gw, buf);

		if (re) {
			fw_redirect_web(EMS_NO, gw);
			fw_redirect_dns(EMS_NO, gw);
		}

		cfg_set(emscfg(), CFG_lan_addr, buf);
		core_gw_addr_clear();
		gw = core_gw_addr();
		ems_assert(gw != NULL);
	}

	re = redirect;

	fw_redirect_web(re, gw);
	fw_redirect_dns(re, gw);

	return EMS_OK;
}


static u_short ping_chksum(u_short *addr, ems_int len)
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


static ems_int ping_st_into_ping(netcheck *ping)
{
	int i, cc, len;;
	struct icmp    *icp;
	struct timeval *tp;
	ems_char       *wr;
	ems_session    *sess;

	ems_assert(ping && ping->sess);

	sess = ping->sess;
	ems_buffer_clear(&sess->buf);

	icp = (struct icmp *)    (buf_wr(&sess->buf));
	tp  = (struct timeval *) (buf_wr(&sess->buf) + 8);
	wr  = (ems_char *)       (buf_wr(&sess->buf) + 8 + sizeof(struct timeval));

	icp->icmp_type  = ICMP_ECHO;
	icp->icmp_code  = 0;
	icp->icmp_cksum = 0;
	icp->icmp_seq   = htons(ping->ntrans++);
	icp->icmp_id    = ping->ident;

	len = 56;
	cc  = len + 8;
	gettimeofday(tp, NULL);

	for(i = 8; i < len; i++)
		*wr++ = i;

	icp->icmp_cksum = ping_chksum((u_short *)icp, cc);

	ping->retry = 3;
	ems_buffer_seek_wr(&sess->buf, cc, EMS_BUFFER_SEEK_CUR);
	sess_event_set(sess, EMS_EVT_WRITE, ping_evt_cb);
	sess_timeout_set_sorted(sess, PING_SEND_TIMEOUT, ping_timeout_cb);

	return EMS_OK;
}

ems_int ping_change_status(netcheck *ping, ems_status st)
{
	ping->st = st;

	switch(ping->st) {
	case st_start:
	case st_stopped:
	case st_err:
		return ping_evt_run(ping, NULL, 0);
		break;

	case st_hb:
		return ping_st_into_ping(ping);
		break;

	default:
		break;
	}


	return EMS_OK;
}

static ems_int ping_evt_run(netcheck *ping, ems_session *sess,  ems_uint flg)
{
	ems_assert(ping_evt_handler[ping->st]);
	return ping_evt_handler[ping->st](ping, sess, flg);
}

static ems_void ping_evt_cb(ems_session *sess, ems_int err, ems_int flg)
{
	netcheck *ping = (netcheck *)sess_cbarg(sess);

	ems_assert(ping->st > st_min && ping->st < st_max);

	if (err) {
		ems_l_trace("[ping] evt err, sess: %d %s",
				ems_sock_fd(&sess->sock),
				ems_sock_addr(&sess->sock));
		ping_change_status(ping, st_err);
		return;
	}

	ping_evt_run(ping, sess, flg);
}

static ems_void ping_timeout_cb(ems_session *sess, ems_timeout *to)
{
	netcheck *ping = (netcheck *)sess_cbarg(sess);

	ems_assert(ping->st > st_min && ping->st < st_max);

	ems_assert(ping_timeout_handler[ping->st]);

	if (ping_timeout_handler[ping->st])
		ping_timeout_handler[ping->st](ping, sess, to);
}


static ems_int ping_gw_addr(ems_session *sess)
{
	static ems_char buf[64] = {0};

	snprintf(buf, sizeof(buf), "%s",
		ems_popen_get("ip route | grep default | head -1 | awk '{print $3}'"));

	if (strlen(buf) <= 0)
		return EMS_ERR;

	ems_sock_setaddr(&sess->sock, buf);

	return EMS_OK;
}

static ems_int ping_gw_connect(ems_session *sess)
{
	struct protoent *proto;
	ems_sock        *sock = &sess->sock;
	socklen_t        len;
	struct sockaddr_in addr;
	ems_int   fd;

	memset(&addr, 0, sizeof(addr));

	if (ems_gethostbyname(ems_sock_addr(sock), &addr) != OK) {
		ems_l_trace("gethostbyename failed %s : %s", 
				ems_sock_addr(sock), ems_lasterrmsg());
		return EMS_ERR;
	}

	if ((proto = getprotobyname("icmp")) == NULL) {
		ems_l_trace("getprotobyname failed %s : %s", 
				ems_sock_addr(sock), ems_lasterrmsg());
		return EMS_ERR;
	}

	if ((fd = socket(AF_INET, SOCK_RAW, proto->p_proto)) < 0) {
		ems_l_trace("socket error: %s", ems_lasterrmsg());
		return EMS_ERR;
	}

	addr.sin_family = AF_INET;
	len = sizeof(struct sockaddr_in);

	if (connect(fd, (struct sockaddr *)&addr, len)) {
		close(fd);
		ems_l_trace("connect error: %s", ems_lasterrmsg());
		return EMS_ERR;
	}

	ems_sock_setfd(&sess->sock, fd);

	return EMS_OK;
}

static ems_int ping_start(netcheck *ping, ems_session *sess, ems_uint flg)
{
	if (!ping->sess) {
		ping->sess = ems_session_new();
		if (!ping->sess)
			return EMS_ERR;

		sess_cbarg_set(ping->sess, ping);
	}

	sess = ping->sess;
	ems_buffer_clear(&sess->buf);
	ems_buffer_clear(&sess->buf_in);

	ping->ident  = random() & 0xFFFF;
	ping->ntrans = 0;
	ping->id     = 0;
	ping->retry  = 3;

	if (ping_gw_addr(sess) != EMS_OK)
		return ping_change_status(ping, st_err);
	{
		ems_cchar *wan = cfg_get(emscfg(), CFG_wan_addr);

		if (!wan || strcmp(ems_sock_addr(&sess->sock), wan)) 
		{
			str_clear(&emscorer()->gw);
			str_clear(&emscorer()->ifname);
			str_clear(&emscorer()->ac_mac);
			str_clear(&emscorer()->ssid);
			ems_flush_system_info();
		}
	}

	if (ping_gw_connect(sess) != EMS_OK)
		return ping_change_status(ping, st_err);

	ems_l_trace("[ping] sess: %d gw addr: %s", 
			ems_sock_fd(&sess->sock), ems_sock_addr(&sess->sock));

	return ping_change_status(ping, st_hb);
}

static ems_int ping_stopped(netcheck *ping, ems_session *sess, ems_uint flg)
{
	sess = ping->sess;

	if (sess) {
		ems_session_shutdown_and_destroy(sess);
		ping->sess = NULL;
	}

	ping_fw_redirect(ping, EMS_NO);

	return EMS_OK;
}

static ems_int ping_normal(netcheck *ping, ems_session *sess, ems_uint flg)
{
	ems_assert(0 && "should never be here");
	return EMS_OK;
}

static ems_int 
ping_send(netcheck *ping, ems_session *sess, ems_uint flg)
{
	ems_int ret;

	ems_assert(ems_flag_like(flg, EMS_EVT_WRITE));
//	ems_l_trace("send to(%s) length: %d", ems_sock_addr(&sess->sock), buf_len(&sess->buf));
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
		sess_event_set(sess, EMS_EVT_READ, ping_evt_cb);

	return EMS_OK;
}

static ems_int ping_recv_handle(netcheck *ping, ems_session *sess)
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
		ems_l_trace("[ping] %d bytes icmp not echo reply, %d", cc, icp->icmp_type);
		ems_buffer_clear(&sess->buf_in);
		return EMS_OK;
	}

	if( icp->icmp_id != ping->ident) {
		ems_l_trace("[ping] %d bytes icmp ident: %d != %d", cc, icp->icmp_id, ping->ident);
		ems_buffer_clear(&sess->buf_in);
		return EMS_OK;
	}

	ems_buffer_clear(&sess->buf_in);

	sess_event_cancel(sess);
	sess_timeout_set_sorted(sess, PING_IDLE_TIMEOUT, ping_timeout_cb);

	ems_flag_set(emscorer()->flg, FLG_NETWORK_READY);

	ping_fw_redirect(ping, EMS_NO);

	if (ems_flag_unlike(emscorer()->flg, FLG_NETWORK_BRIDGE_RUN)) {
		ems_flag_set(emscorer()->flg, FLG_NETWORK_BRIDGE_RUN);

		if (ems_flag_unlike(emscorer()->flg, FLG_NETWORK_BRIDGE)) 
			ems_app_process(ty_net, ty_bridge, EMS_APP_START, NULL);
			//bridge_run(emscorer(), EMS_YES);
	}

	return ping_change_status(ping, st_normal);
}

static ems_int 
ping_recv(netcheck *ping, ems_session *sess)
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
			ems_l_trace("[ping] ping error: %s", ems_lasterrmsg());
			return EMS_ERR;
		}
	}

	ems_buffer_seek_wr(&sess->buf_in, ret, EMS_BUFFER_SEEK_CUR);

	ret = ping_recv_handle(ping, sess);

	goto again;

	return EMS_OK;
}


static ems_int ping_ping(netcheck *ping, ems_session *sess, ems_uint flg)
{
	if (ems_flag_like(flg, EMS_EVT_READ)) {
		if (ping_recv(ping, sess) != EMS_OK) 
			return ping_change_status(ping, st_err);

		return EMS_OK;
	}

	if (ems_flag_like(flg, EMS_EVT_WRITE)) {
		if (ping_send(ping, sess, flg) != EMS_OK)
			return ping_change_status(ping, st_err);
	}

	return EMS_OK;
}

static ems_int ping_err(netcheck *ping, ems_session *sess, ems_uint flg)
{
	ems_assert(ping->sess != NULL);

	sess = ping->sess;

	if (sess) {
		sess_event_cancel(sess);
		ems_l_trace("[ping]shutdown session(%d) with [%s]", 
				ems_sock_fd(&sess->sock),
				ems_sock_addr(&sess->sock));
		ems_sock_close(&sess->sock);
		sess_timeout_set_sorted(sess, PING_IDLE_TIMEOUT, ping_timeout_cb);
	}

	ems_flag_unset(emscorer()->flg, FLG_NETWORK_READY);
	ping_fw_redirect(ping, EMS_YES);

	if (ems_flag_unlike(emscorer()->flg, FLG_NETWORK_BRIDGE)) {
		if (!ems_app_run(ty_downlink))
			ems_flag_unset(emscorer()->flg, FLG_NETWORK_BRIDGE_RUN);
		ems_app_process(ty_net, ty_bridge, EMS_APP_STOP, NULL);
	}

	return EMS_OK;
}


static ems_int ping_to_normal(netcheck *ping, ems_session *sess, ems_timeout *to)
{
	return ping_change_status(ping, st_hb);
}

static ems_int ping_to_ping(netcheck *ping, ems_session *sess, ems_timeout *to)
{
	ems_cchar *addr = NULL;

	ems_l_trace("[ping] timeout..., retry times: %d", ping->retry);

	ping->retry--;

	sess_event_cancel(sess);

	if (ping->retry) {
		ems_buffer_seek_rd(&sess->buf, 0, EMS_BUFFER_SEEK_SET);
		sess_event_set(sess,  EMS_EVT_WRITE,  ping_evt_cb);
		sess_timeout_set_sorted(sess, PING_SEND_TIMEOUT, ping_timeout_cb);
		return EMS_OK;
	}

	addr = net_addrs[ping->id++];

	if (addr) {
		sess_event_cancel(sess);
		ems_l_trace("[ping]shutdown session(%d) with [%s]", 
				ems_sock_fd(&sess->sock),
				ems_sock_addr(&sess->sock));
		ems_sock_close(&sess->sock);

		ems_sock_setaddr(&sess->sock, addr);

		if (ping_gw_connect(sess) != EMS_OK)
			return ping_change_status(ping, st_err);

		ems_l_trace("[ping] sess: %d addr: %s", 
			ems_sock_fd(&sess->sock), ems_sock_addr(&sess->sock));

		return ping_change_status(ping, st_hb);
	}

	return ping_change_status(ping, st_err);
}

static ems_int ping_to_err(netcheck *ping, ems_session *sess, ems_timeout *to)
{
	return ping_change_status(ping, st_start);
}


