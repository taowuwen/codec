
#include "audit.h"
#include "ems_core.h"
#include "ems_client.h"
#include "net_user.h"
#include "net.h"
#include "class.h"

typedef struct _net_if_s {
	pcap_t             *pfd;
	ems_str 	    dev;
	struct bpf_program  fp;
	ems_int             fd;
	ems_session        *sess;
	audit_status        st;

	ems_int             cur_channel;

	ems_queue entry;
} net_inf;

static net_inf *netinf_new()
{
	ems_session *sess = NULL;
	net_inf     *inf = (net_inf *)ems_malloc(sizeof(net_inf));

	if (inf) {
		memset(inf, 0, sizeof(net_inf));

		inf->pfd  = NULL;
		str_init(&inf->dev);
		inf->sess = NULL;
		inf->st   = st_stop;

		sess = ems_session_new();
		ems_buffer_uninit(&sess->buf_in);
		ems_buffer_uninit(&sess->buf_out);
		sess_cbarg_set(sess, inf);

		inf->sess = sess;

		inf->cur_channel = 1;
	}

	return inf;
}

static void netinf_destroy(net_inf *inf)
{
	ems_session *sess;

	ems_assert(inf->pfd == NULL);
	str_uninit(&inf->dev);
	ems_assert(inf->fd == 0);
	ems_assert(inf->st == st_stop);

	if (inf->sess) {
		sess = inf->sess;
		ems_sock_setfd(&sess->sock, 0);
		ems_session_shutdown_and_destroy(sess);
		inf->sess = NULL;
	}

	ems_free(inf);
}


static ems_int netinf_change_status(net_inf *net, audit_status st);

static ems_int netinf_init (net_inf *net, ems_session *sess, ems_uint flg);
static ems_int netinf_normal(net_inf *net, ems_session *sess, ems_uint flg);
static ems_int netinf_stop(net_inf *net, ems_session *sess, ems_uint flg);
static ems_int netinf_err (net_inf *net, ems_session *sess, ems_uint flg);

typedef ems_int (*netinf_evt_func)(net_inf *net, ems_session *sess, ems_uint flg);
static netinf_evt_func netinf_evt_hanler[] = 
{
	[st_init]    = netinf_init,
	[st_normal]  = netinf_normal,
	[st_error]   = netinf_err,
	[st_stop]    = netinf_stop
};

static ems_int netinf_to_err(net_inf *net, ems_session *sess, ems_timeout *to);
static ems_int netinf_to_normal(net_inf *net, ems_session *sess, ems_timeout *to);

typedef ems_int (*net_timeout_func)(net_inf *net, ems_session *sess, ems_timeout *to);
static net_timeout_func netinf_timeout_handler[] = 
{
	[st_init]    = NULL,
	[st_normal]  = netinf_to_normal,
	[st_error]   = netinf_to_err,
	[st_stop]    = NULL
};

static ems_int netinf_evt_run(net_inf *net, ems_session *sess, ems_uint flg)
{
	ems_assert(netinf_evt_hanler[net->st] != NULL);

	return netinf_evt_hanler[net->st](net, sess, flg);
}

static ems_int 
netinf_timeout_run(net_inf *net, ems_session *sess, ems_timeout *to)
{
	ems_assert(netinf_timeout_handler[net->st] != NULL);

	if (netinf_timeout_handler[net->st])
		return netinf_timeout_handler[net->st](net, sess, to);

	return EMS_OK;
}

static ems_void netinf_timeout_cb(ems_session *sess, ems_timeout *to)
{
	net_inf *net = (net_inf *)sess_cbarg(sess);

	ems_l_trace("net (%s) timer trigger", str_text(&net->dev));

	netinf_timeout_run(net, sess, to);
}

static ems_void netif_evt_cb(ems_session *sess, ems_int err, ems_int flg)
{
	net_inf *net = (net_inf *)sess_cbarg(sess);

	ems_assert(net->st > st_min && net->st < st_max);

	if (err) {
		ems_l_trace("[net] evt err, sess: %d %s",
				ems_sock_fd(&sess->sock),
				ems_sock_addr(&sess->sock));
		netinf_change_status(net, st_error);
		return;
	}

	netinf_evt_run(net, sess, flg);
}


static ems_int net_start(net_core *net, ems_session *sess, ems_uint flg);
static ems_int net_stop(net_core *net, ems_session *sess, ems_uint flg);

typedef ems_int (*net_evt_func)(net_core *net, ems_session *sess, ems_uint flg);
static net_evt_func net_evt_handler[] = 
{
	[st_start]   = net_start,
	[st_normal]  = NULL,
	[st_stop]    = net_stop
};

static ems_void net_timeout_cb(ems_timeout *tmout)
{
	net_core *net = ems_container_of(tmout, net_core, to);

	switch(net->st) {
	case st_normal:
		ems_l_trace("net time to rescan net devices");
		audit_postmsg(mod_net, mod_net, MSG_NETWORK_RELOAD);
		ems_timeout_insert_sorted(timeouter(), &net->to, net->to_period * 1000, net_timeout_cb);
		break;

	default:
		break;
	}
}


static ems_int net_evt_run(net_core *net, ems_session *sess, ems_uint flg)
{
	ems_assert(net_evt_handler[net->st] != NULL);

	return net_evt_handler[net->st](net, sess, flg);
}


static ems_int net_open_pcap(net_inf *net)
{
	ems_int    dt;
	ems_char   errbuf[PCAP_ERRBUF_SIZE];

	ems_l_trace("net open dev : %s", str_text(&net->dev));

#if 0
	net->pfd = pcap_open_live(str_text(&net->dev), 8192, 0, 1000, errbuf);
	if (net->pfd == NULL) {
		ems_l_trace("pcap_open_live failed: %s", errbuf);
		return EMS_ERR;
	}
#else

	net->pfd = pcap_create(str_text(&net->dev), errbuf);
	if (net->pfd == NULL) {
		ems_l_trace("pcap_create failed: %s", errbuf);
		return EMS_ERR;
	}

	if (pcap_set_snaplen(net->pfd, 65535) != 0) {
		ems_l_trace("pcap_set_snaplen failed: %s", pcap_geterr(net->pfd));
		goto err_out;
	}

	if (pcap_set_promisc(net->pfd, 1)) {
		ems_l_trace("pcap_set_promisc failed: %s", pcap_geterr(net->pfd));
		goto err_out;
	}

	if (pcap_set_immediate_mode(net->pfd, 1)) {
		ems_l_trace("pcap_set_immediate_mode failed: %s", pcap_geterr(net->pfd));
		goto err_out;
	}

	if (pcap_activate(net->pfd) != 0) {
		ems_l_trace("pcap_set_immediate_mode failed: %s", pcap_geterr(net->pfd));
		goto err_out;
	}
#endif

	dt = pcap_datalink(net->pfd);

	ems_l_trace("data: %s, %s", 
		pcap_datalink_val_to_name(dt), pcap_datalink_val_to_description(dt));

	return EMS_OK;
err_out:
	pcap_close(net->pfd);
	net->pfd = NULL;
	return EMS_ERR;
}

static ems_int is_netinf_exist(net_core *net, ems_cchar *nick)
{
	net_inf   *inf;
	ems_queue *p;

	ems_assert(nick != NULL && ems_strlen(nick) > 0);

	ems_queue_foreach(&net->pcap, p) {
		inf = ems_container_of(p, net_inf, entry);

		ems_assert(str_len(&inf->dev) > 0);

		if (!strcmp(nick, str_text(&inf->dev)))
			return EMS_YES;
	}
	
	return EMS_NO;
}

static ems_cchar *net_get_ip_address(pcap_if_t *inf)
{
	static ems_char buf[64];
	pcap_addr_t    *addr;
	struct sockaddr_in *sock = NULL;

	memset(buf, 0, sizeof(buf));

	for (addr =inf->addresses; addr != NULL; addr = addr->next) {

		if (!addr->addr)
			continue;

		sock = (struct sockaddr_in *)addr->addr;

		if (sock->sin_family == AF_INET 
		/*   || sock->sin_family == AF_INET6 */
			) {
			return inet_ntop(sock->sin_family, (void *)&sock->sin_addr, buf, 64);
		}
	}

	return NULL;
}

ems_int net_load_all_netinf(net_core *net)
{
	net_inf   *n_inf;
	pcap_if_t *all;
	char buf[PCAP_ERRBUF_SIZE];
	ems_session *sess = NULL;

	pcap_if_t   *inf;

	if (pcap_findalldevs(&all, buf) ) {
		ems_l_error("net error, pcap_findalldevs: %s", buf);
		return EMS_ERR;
	}

	for (inf = all; inf != NULL; inf = inf->next) {
		ems_assert(inf->name != NULL);


		if (!inf->name) continue;
		ems_l_trace("got network interface: %s", inf->name);

		if (strncmp(inf->name, "wlp3s0", 6)) continue;

		if (inf->flags & PCAP_IF_LOOPBACK) continue; 

#ifdef BOARD_MT7620N
		if (strncmp(inf->name, "br-lan", 6)) continue;
#elif BOARD_AR9344
		if (strncmp(inf->name, "br", 2)) continue;
#else
		/* no, nothing, captuer all maybe */
#endif

		if (is_netinf_exist(net, inf->name)) {
			ems_l_trace("link: %s, already exist, continue", inf->name);

#ifdef BOARD_AR9344 
			if (!strcmp(inf->name, "br1"))
				str_set(&net->pkgs.gwip,net_get_ip_address(inf));
#endif

			continue;
		}

		n_inf = netinf_new();
		if (!n_inf) continue;

		sess = n_inf->sess;

		str_set(&n_inf->dev, inf->name);
		ems_sock_setaddr(&sess->sock, net_get_ip_address(inf));

		ems_l_trace("interface: %s (%s) attched", str_text(&n_inf->dev),ems_sock_addr(&sess->sock));

		ems_queue_insert_tail(&net->pcap, &n_inf->entry);

		netinf_change_status(n_inf, st_init);
	}

	pcap_freealldevs(all);

	return EMS_OK;
}

/*
 1. load plugins here
    and send start msg here
 */
static ems_int net_start(net_core *net, ems_session *sess, ems_uint flg)
{
	ems_l_trace("net start goes from here...");

	net_load_plugins(&net->flt, id_net);

	if (ems_queue_empty(&net->flt)) {
		ems_l_trace("net at least one filter should supply");
		return EMS_ERR;
	}

	net_plug_broadcast(&net->flt, A_AUDIT_START, NULL);

	if (net_load_all_netinf(net) != EMS_OK) {
		net_change_status(net, st_stop);
		return EMS_ERR;
	}

#if 0
	if (ems_queue_empty(&net->pcap)) {
		net_change_status(net, st_stop);
		return EMS_ERR;
	}
#endif

	ems_timeout_insert_sorted(timeouter(), &net->to, net->to_period * 1000, net_timeout_cb);
	return net_change_status(net, st_normal);
}

/*
   send stop msg here 
   and 
   unload plugins here
 */
static ems_int net_stop(net_core *net, ems_session *sess, ems_uint flg)
{
	net_inf   *inf;
	ems_queue *p, *q;

	ems_timeout_cancel(&net->to);

	net_plug_broadcast(&net->flt, A_AUDIT_STOP, NULL);
	net_unload_plugins(&net->flt);

	ems_queue_foreach_safe(&net->pcap, p, q) {
		inf = ems_container_of(p, net_inf, entry);
		netinf_change_status(inf, st_stop);
	}

	return EMS_OK;
}

static ems_cchar *net_gw_self_addr(net_inf *inf)
{
#ifndef GENERIC_LINUX
	ems_session *sess = inf->sess;
#endif

	static ems_char buf[512] = {0};

	memset(buf, 0, sizeof(buf));

#ifndef GENERIC_LINUX
	if (ems_sock_addr(&sess->sock))
		snprintf(buf, sizeof(buf), "(host not %s) and ", ems_sock_addr(&sess->sock));
#endif

	return buf;
}


/*
   1. get rules, we can skip this for now 
   2. compile and set
 */
static ems_int netinf_init(net_inf *net, ems_session *sess, ems_uint flg)
{
	ems_char    errbuf[PCAP_ERRBUF_SIZE];
	bpf_u_int32 netmask;
	bpf_u_int32 network;
	ems_buffer *buf = core_buffer();
	sess = net->sess;

	ems_cchar  *filter = cfg_get(emscfg(), CFG_net_filter_rule);
	if (!filter)
		filter = "";

	ems_buffer_refresh(buf);

	snprintf(buf_wr(buf), buf_left(buf), "%s%s", net_gw_self_addr(net), filter);
	filter = buf_wr(buf);
	ems_l_trace("net(%s) filter: %s", str_text(&net->dev), filter);

	if (net_open_pcap(net) != EMS_OK) {
		ems_l_trace("net open pcap failed: %s", ems_lasterrmsg());
		goto err_stop;
	}

	if (pcap_lookupnet(str_text(&net->dev), &network, &netmask, errbuf) < 0) 
	{
		network = netmask = 0;
		ems_l_trace("net, pcap_lookupnet failed on %s, %s", 
				str_text(&net->dev), errbuf);
		//goto err_out;
	}

	if (pcap_compile(net->pfd, &net->fp, filter, 0, network) == -1) {
		ems_l_trace("couldn't parse filter %s: %s", 
				filter, pcap_geterr(net->pfd));
		goto err_out;
	}

	if (pcap_setfilter(net->pfd, &net->fp) == -1) {
		ems_l_trace("couldn't install filter: %s, filter: %s", 
				pcap_geterr(net->pfd), filter);
		goto err_out;
	}

	net->fd = pcap_get_selectable_fd(net->pfd);
	if (net->fd == -1) {
		ems_l_trace("pcap get selectable fd failed: %s", pcap_geterr(net->pfd));
		goto err_out;
	}

	pcap_setnonblock(net->pfd, 1, errbuf);

	ems_sock_setfd(&sess->sock, net->fd);
	sess_event_set(sess, EMS_EVT_READ, netif_evt_cb);
	sess_timeout_set_sorted(sess, 3000, netinf_timeout_cb);

	return netinf_change_status(net, st_normal);

err_out:
	return netinf_change_status(net, st_error);

err_stop:
	return netinf_change_status(net, st_stop);
}

ems_cchar *net_mac2str(const u_char *s)
{
	static ems_char mac[32] = {0};

	snprintf(mac, 32, "%02x:%02x:%02x:%02x:%02x:%02x",
			s[0], s[1], s[2], s[3], s[4], s[5]);

	return mac;
}

ems_void
net_packages(u_char *args, const struct pcap_pkthdr *header, const u_char *packet)
{
	{
		ems_l_trace(" %ld -> %ld length: (%d, %d)", 
				header->ts.tv_sec, header->ts.tv_usec,
				header->caplen, header->len);
	}

	net_core  *net   = netcorer();
	net_flow  *flow  = &net->pkgs;

	const struct flow_ethernet *ether;
	const struct flow_ip       *ip;
	const u_char               *payload;

	ems_int size_ip;
	ems_int size_payload;

	ether = (struct flow_ethernet*)(packet);
	ip    = (struct flow_ip*)(packet + SIZE_ETHERNET);

	size_ip = IP_HL(ip)*4;
	if (size_ip < 20 || (IP_V(ip) != 4)) {
		return;
	}

	flow->l2 = (ems_uchar *)ether;
	flow->l3 = (ems_uchar *)ip;
	flow->hdr    = header;
	flow->packet = packet;

	switch(ip->ip_p) {

	case IPPROTO_TCP: {
		ems_int  size_tcp;
		const struct flow_tcp  *tcp;

		tcp = (struct flow_tcp*)(packet + SIZE_ETHERNET + size_ip);

		size_tcp = TH_OFF(tcp) * 4;

		if (size_tcp < 20) return;

		payload = (u_char *)(packet + SIZE_ETHERNET + size_ip + size_tcp);
		size_payload = ntohs(ip->ip_len) - (size_ip + size_tcp);

		flow->l4 = (ems_uchar *)tcp;
		flow->l5 = (ems_uchar *)payload;
		flow->l5_len = size_payload;

#ifdef DEBUG
		{
			ems_char dst[32] = {0};
			ems_char tmbuf[64]  = {0};
			struct tm   *tm;
			ems_cchar   *user;
			ems_buffer  *buf = core_buffer();

			tm = localtime(&header->ts.tv_sec);

			snprintf(dst, 32, "%s", inet_ntoa(ip->ip_dst));
			
			snprintf(tmbuf, 64, "%04d-%02d-%02d %02d:%02d:%02d", 
					1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday,
					tm->tm_hour, tm->tm_min, tm->tm_sec);

			/* for now , usermac, apmac, time, url, userip, ....*/
			ems_buffer_refresh(buf);

			user = net_user_nick(net->h_user, net_mac2str(ether->ether_shost));
			if (!user)
				user = "*";
			snprintf(buf_wr(buf), buf_left(buf), 
				"%s,%s,%s,%u.%u,[%s:%d -> %s:%d],(caplen: %d len: %d payload: %d), %s,%s", 
				net_mac2str(ether->ether_shost), str_text(&flow->apmac), tmbuf,
				(ems_uint)header->ts.tv_sec, (ems_uint)header->ts.tv_usec,
				inet_ntoa(ip->ip_src), ntohs(tcp->th_sport), dst, ntohs(tcp->th_dport),
				header->caplen, header->len, flow->l5_len,
				str_text(&flow->gwip), user);

			//output_log(id_http, id_http_url, buf_rd(buf));
                        ems_l_trace("%s", buf_rd(buf));
		}
#endif
		}
		break;
	case IPPROTO_UDP:
		ems_l_trace("udp");
	case IPPROTO_ICMP:
		ems_l_trace("icmp");
	case IPPROTO_IP:
		ems_l_trace("ip");
		return;
	default:
		ems_l_trace("   Protocol: unknown\n");
		return;
	}

#if 0 
	if (flow->l5_len > 0) {
		flow->user = net_user_nick(net->h_user, net_mac2str(ether->ether_shost));
#ifdef BOARD_AR9344
		if (flow->user)
#endif
		net_plug_broadcast(&net->flt, A_AUDIT_NET_PKGS, (ems_void *)flow);
	}
#endif

	return;
}

static ems_int netinf_normal(net_inf *net, ems_session *sess, ems_uint flg)
{
	ems_assert(ems_flag_like(flg, EMS_EVT_READ));

	if (ems_flag_like(flg, EMS_EVT_READ)) {
		if (pcap_dispatch(net->pfd, -1, net_packages, (ems_uchar *)net) < 0) {
			ems_l_trace("pcap_dispatch failed: %s", pcap_geterr(net->pfd));
			return netinf_change_status(net, st_error);
		}
	}

	return EMS_OK;
}

static ems_int netinf_err (net_inf *inf, ems_session *sess, ems_uint flg)
{
	ems_l_trace("net(%s) error ocurr", str_text(&inf->dev));

	sess_event_cancel(inf->sess);
	sess_timeout_set_sorted(inf->sess, 1000, netinf_timeout_cb);

	if (inf->pfd) {
		pcap_freecode(&inf->fp);
		pcap_close(inf->pfd);
		inf->pfd = NULL;
		inf->fd  = 0;
	}

	return EMS_OK;
}

static ems_int netinf_to_normal(net_inf *net, ems_session *sess, ems_timeout *to)
{
	ems_char cmd[128];

	snprintf(cmd, sizeof(cmd), 
		"iwconfig %s channel %d", 
			str_text(&net->dev), net->cur_channel);
	ems_systemcmd(cmd);

	net->cur_channel = net->cur_channel % 13 + 1;
	sess_timeout_set_sorted(net->sess, 3000, netinf_timeout_cb);

//	ems_l_trace("net(%s) error timeout tiger..", str_text(&net->dev));
//	netinf_change_status(net, st_init);
	return EMS_OK;
}

static ems_int netinf_to_err(net_inf *net, ems_session *sess, ems_timeout *to)
{
	ems_l_trace("net(%s) error timeout tiger..", str_text(&net->dev));
	netinf_change_status(net, st_init);
	return EMS_OK;
}

static ems_int netinf_stop(net_inf *inf, ems_session *sess, ems_uint flg)
{
	sess = inf->sess;

	ems_l_trace("net(%s) stopping", str_text(&inf->dev));

	if (sess) {
		ems_sock_setfd(&sess->sock, 0);
		ems_session_shutdown_and_destroy(sess);
		inf->sess = NULL;
	}

	if (inf->pfd) {
		pcap_freecode(&inf->fp);
		pcap_close(inf->pfd);
		inf->pfd = NULL;
		inf->fd  = 0;
	}

	ems_queue_remove(&inf->entry);
	netinf_destroy(inf);

	return EMS_OK;
}


ems_int net_change_status(net_core *net, audit_status st)
{
	ems_l_trace("[net] change  status: %s >> %s",
			audit_status_str(net->st), audit_status_str(st));

	net->st = st;

	switch(net->st) {
	case st_start:
	case st_init:
	case st_stop:
	case st_error:
		return net_evt_run(net, NULL, 0);

	case st_normal:
		break;

	default:
		break;
	}

	return EMS_OK;
}

static ems_int netinf_change_status(net_inf *net, audit_status st)
{
	ems_l_trace("[net %s] change  status: %s >> %s", str_text(&net->dev),
			audit_status_str(net->st), audit_status_str(st));

	net->st = st;

	switch(net->st) {
	case st_start:
	case st_init:
	case st_stop:
	case st_error:
		return netinf_evt_run(net, NULL, 0);

	case st_normal:
		break;

	default:
		break;
	}

	return EMS_OK;
}

