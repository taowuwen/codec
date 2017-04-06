
#include "ems_core.h"
#include "ems_client.h"
#include "app.h"


#ifdef DLINK_USE_PING
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#endif

#define DLINK_PING_TIMEOUT	2000
#ifdef DEBUG
#define DLINK_TIMEOUT		61000
#else
#define DLINK_TIMEOUT		300000
#endif


typedef struct _ems_downlink_one_s {
	ems_session *sess;
	ems_str      ip;
	ems_str      mac;
	ems_queue    entry;

#ifdef DLINK_USE_PING
	ems_int      ident;
	ems_int      ntrans;
	ems_int      retry;
#endif

	ems_status   st;
} ems_downlink;

static ems_void dlink_destroy(ems_downlink *dlink);
static ems_void dlink_timeout_cb(ems_session *sess, ems_timeout *to);

#ifdef DLINK_USE_PING
static ems_void dlink_evt_cb(ems_session *sess, ems_int err, ems_int flg);
static ems_int  dlink_ping(ems_downlink *dlink);

static ems_int 
dlink_send_msg(ems_downlink *dlink, ems_session *sess, ems_uint flg)
{
	ems_int ret;

	ems_assert(ems_flag_like(flg, EMS_EVT_WRITE));

	ems_l_trace("[dlink] send(%d): (%s, length: %d)", 
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
		sess_event_set(sess, EMS_EVT_READ, dlink_evt_cb);

	return EMS_OK;
}

static ems_int dlink_process_icmp(ems_downlink *dlink, ems_session *sess)
{
	struct ip   *ip;
	struct icmp *icp;
	int hlen, cc = buf_len(&sess->buf_in);
	static ems_char mac[32];

	ip   = (struct ip *) buf_rd(&sess->buf_in);
	hlen = ip->ip_hl << 2;

	if (cc < hlen + ICMP_MINLEN) 
		return EMS_OK;

	cc -= hlen;
	icp = (struct icmp *)(buf_rd(&sess->buf_in) + hlen);

	if(icp->icmp_type != ICMP_ECHOREPLY) {
		ems_l_trace("[dlink] %d bytes from: %s icmp not reply(%d)",
				cc, ems_sock_addr(&sess->sock), icp->icmp_type);
		return EMS_CONTINUE;
	}

	if( icp->icmp_id != dlink->ident) {
		ems_l_trace("[dlink] %d bytes icmp ident: %d error, not : %d", cc, icp->icmp_id, dlink->ident);
		return EMS_CONTINUE;
	}

	ems_l_trace("icmp replied by (%s), length: %d", ems_sock_addr(&sess->sock), cc);

	snprintf(mac, sizeof(mac), "%s", ems_usermac(ems_sock_addr(&sess->sock)));
	if (strcmp(mac, str_text(&dlink->mac))) {
		ems_l_trace("downlink change from: %s into: %s", str_text(&dlink->mac), mac);
		dlink->st = st_err;
		return EMS_ERR;
	}

	dlink->st = st_normal;
	sess_event_cancel(sess);
	sess_timeout_set_sorted(sess, DLINK_TIMEOUT, dlink_timeout_cb);

	return EMS_OK;
}

static ems_int 
dlink_recv_msg(ems_downlink *dlink, ems_session *sess)
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
			ems_l_trace("[dlink] ping error: %s", ems_lasterrmsg());
			return EMS_ERR;
		}
	}

	ems_buffer_seek_wr(&sess->buf_in, ret, EMS_BUFFER_SEEK_CUR);

	ret = dlink_process_icmp(dlink, sess);
	if (ret == EMS_ERR)
		return EMS_ERR;

	ems_buffer_clear(&sess->buf_in);

	goto again;

	return EMS_OK;
}


static ems_void dlink_evt_cb(ems_session *sess, ems_int err, ems_int flg)
{
	ems_downlink *dlink = (ems_downlink *)sess_cbarg(sess);

	if (err) 
		goto err_out;

	if (ems_flag_like(flg, EMS_EVT_WRITE)) {
		if (dlink_send_msg(dlink, sess, flg) != EMS_OK)
			goto err_out;
	}

	if (ems_flag_like(flg, EMS_EVT_READ)) {
		if (dlink_recv_msg(dlink, sess) != EMS_OK)
			goto err_out;
	}

	return;
err_out:
	ems_l_trace("[dlink] shutdown: %d (%s: %s)",
		ems_sock_fd(&sess->sock), str_text(&dlink->ip), str_text(&dlink->mac));

	ems_queue_remove(&dlink->entry);
	dlink_destroy(dlink);
}


static u_short dlink_icmp_checksum(u_short *addr, ems_int len)
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

static ems_int dlink_ping(ems_downlink *dlink)
{
	int i, cc, len;;
	struct icmp    *icp;
	struct timeval *tp;
	ems_char       *wr;
	ems_session    *sess = NULL;

	sess = dlink->sess;
	ems_assert(sess != NULL);

	ems_buffer_reset(&sess->buf);

	icp = (struct icmp *)    (buf_wr(&sess->buf));
	tp  = (struct timeval *) (buf_wr(&sess->buf) + 8);
	wr  = (ems_char *)       (buf_wr(&sess->buf) + 8 + sizeof(struct timeval));

	icp->icmp_type  = ICMP_ECHO;
	icp->icmp_code  = 0;
	icp->icmp_cksum = 0;
	icp->icmp_seq   = htons(dlink->ntrans++);
	icp->icmp_id    = dlink->ident;

	len = 56;
	cc  = len + 8;
	gettimeofday(tp, NULL);

	for(i = 8; i < len; i++)
		*wr++ = i;

	icp->icmp_cksum = dlink_icmp_checksum((u_short *)icp, cc);
	ems_buffer_seek_wr(&sess->buf, cc, EMS_BUFFER_SEEK_CUR);

	dlink->retry = 3;

	dlink->st = st_hb;

	sess_event_set(sess, EMS_EVT_WRITE, dlink_evt_cb);
	sess_timeout_set_sorted(sess, DLINK_PING_TIMEOUT, dlink_timeout_cb);

	return EMS_OK;

}

static ems_int dlink_connect(ems_session *sess)
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

	ems_l_trace("[dlink] sess: %d host addr: %s", 
			ems_sock_fd(&sess->sock), ems_sock_addr(&sess->sock));

	return EMS_OK;
}
#endif

static ems_void dlink_timeout_cb(ems_session *sess, ems_timeout *to)
{
	ems_downlink *dlink = (ems_downlink *)sess_cbarg(sess);

	switch(dlink->st) {
#ifdef DLINK_USE_PING

	case st_hb:
	{
		dlink->retry--;
		sess_event_cancel(sess);
		if (dlink->retry > 0) {
			ems_l_trace("[dlink] retry send ping: %d", dlink->retry);
			ems_buffer_seek_rd(&sess->buf, 0, EMS_BUFFER_SEEK_SET);
			sess_event_set(sess, EMS_EVT_WRITE, dlink_evt_cb);
			sess_timeout_set_sorted(sess, DLINK_PING_TIMEOUT, dlink_timeout_cb);
			return;
		}

		dlink->st = st_err;
		ems_l_trace("[dlink] lost downlink: %s: %s", str_text(&dlink->ip), str_text(&dlink->mac));
		ems_queue_remove(&dlink->entry);
		dlink_destroy(dlink);
	}
	break;

	case st_normal:
		dlink_ping(dlink);
	break;
#else
	case st_start:
		ems_l_trace("[dlink] downlink timeout %s: %s", str_text(&dlink->ip), str_text(&dlink->mac));
		ems_queue_remove(&dlink->entry);
		dlink_destroy(dlink);
	break;
#endif

	default:
		break;
	}
}



static ems_int dlink_start(ems_downlink *dlink)
{
	ems_session *sess;

	if (dlink->sess) {
#ifdef DLINK_USE_PING
		sess = dlink->sess;
		sess_event_cancel(sess);
		ems_sock_close(&sess->sock);
#endif
	} else {
		dlink->sess = ems_session_new();
		if (!dlink->sess)
			return EMS_ERR;

#ifndef DLINK_USE_PING
		ems_buffer_uninit(&dlink->sess->buf_in);
		ems_buffer_uninit(&dlink->sess->buf_out);
#endif
		sess_cbarg_set(dlink->sess, dlink);
	}

	dlink->st = st_start;

	sess = dlink->sess;

#ifdef DLINK_USE_PING
	dlink->ident  = random() & 0xffff;
	dlink->ntrans = 0;

	ems_sock_setaddr(&sess->sock, str_text(&dlink->ip));

	if (dlink_connect(dlink->sess) != EMS_OK) 
		return EMS_ERR;

	return dlink_ping(dlink);
#else
	sess_timeout_set_sorted(sess, DLINK_TIMEOUT, dlink_timeout_cb);
#endif
	return EMS_OK;
}


static void dlink_destroy(ems_downlink *dlink)
{
	if (dlink) {
		if (dlink->sess) {
			ems_session_shutdown_and_destroy(dlink->sess);
			dlink->sess = NULL;
		}

		str_uninit(&dlink->ip);
		str_uninit(&dlink->mac);

		ems_free(dlink);
	}
}

static ems_downlink *dlink_find(ems_queue *list, ems_cchar *ip)
{
	ems_queue    *p;
	ems_downlink *dlink;

	ems_assert(list && ip);

	if (!ip)
		return NULL;

	ems_queue_foreach(list, p) {
		dlink = ems_container_of(p, ems_downlink, entry);

		ems_assert(str_text(&dlink->ip) && ip);

		if (!strcmp(str_text(&dlink->ip), ip))
			return dlink;
	}

	return NULL;
}

static ems_int dlink_init(app_module *mod)
{
	ems_queue *list;
	list = (ems_queue *)ems_malloc(sizeof(ems_queue));

	if (!list)
		return EMS_ERR;

	ems_queue_init(list);

	mod->ctx = (ems_void *)list;

	return EMS_OK;
}

static ems_int dlink_uninit(app_module *mod)
{
	ems_queue *list = mod->ctx;

	if (list) {
		ems_queue_clear(list, ems_downlink, entry, dlink_destroy);
		ems_free(list);
	}

	mod->ctx = NULL;
	return EMS_OK;
}

static ems_int dlink_ems_run(app_module *mod, ems_int run)
{
	ems_int evt = EMS_APP_START;

	if (!run) {
		evt = EMS_APP_STOP;
		/* no need to start radius here, for that portal gonna start radius module*/
		ems_send_message(ty_downlink, ty_radius, evt, NULL); 
	}

	ems_send_message(ty_downlink, ty_fw,     evt, NULL);
	ems_send_message(ty_downlink, ty_bwlist, evt, NULL);
	ems_send_message(ty_downlink, ty_portal, evt, NULL);
	ems_send_message(ty_downlink, ty_client, evt, NULL);

	return EMS_OK;
}

static ems_int dlink_run(app_module *mod, ems_int run)
{
	if (run) {
		if (ems_flag_like(mod->flg, FLG_RUN))
			return EMS_OK;

		ems_l_trace("downlink running here");
		ems_flag_set(mod->flg, FLG_RUN);

		dlink_ems_run(mod, EMS_YES);
	} else {
		if (ems_flag_unlike(mod->flg, FLG_RUN))
			return EMS_OK;

		ems_l_trace("downlink stopped");
		ems_queue_clear((ems_queue *)mod->ctx, ems_downlink, entry, dlink_destroy);
		ems_flag_unset(mod->flg, FLG_RUN);

		dlink_ems_run(mod, EMS_NO);
	}

	return EMS_OK;
}

static ems_int dlink_evt_user_in(app_module *mod, json_object *root)
{
	ems_str  ip, mac;
	ems_downlink *dlink = NULL;
	ems_queue    *list = (ems_queue *)mod->ctx;

	str_init(&ip);
	str_init(&mac);

	do {
		ems_json_get_string_def(root, "ip",  &ip,  NULL);
		ems_json_get_string_def(root, "mac", &mac, NULL);

		if (str_text(&ip) && str_text(&mac)) {
			dlink = dlink_find(list, str_text(&ip));

			if (!dlink) {
				dlink = (ems_downlink *) ems_malloc(sizeof(ems_downlink));
				if (!dlink) break;

				dlink->sess = NULL;

				str_init(&dlink->ip);
				str_init(&dlink->mac);
				ems_queue_init(&dlink->entry);

				str_set(&dlink->ip, str_text(&ip));
				str_set(&dlink->mac, str_text(&mac));

				ems_queue_insert_tail(list, &dlink->entry);

				if (dlink_start(dlink) != EMS_OK) {
					ems_queue_remove(&dlink->entry);
					dlink_destroy(dlink);
				}
			}
			else {
				/* update dlink */
				if (strcmp(str_text(&mac), str_text(&dlink->mac))) 
				{
					str_set(&dlink->mac, str_text(&mac));
					if (dlink_start(dlink) != EMS_OK) {
						ems_queue_remove(&dlink->entry);
						dlink_destroy(dlink);
					}
				}
			}
		}
	} while (0);

	str_uninit(&ip);
	str_uninit(&mac);

	return EMS_OK;
}

static ems_int dlink_num(app_module *mod, json_object *root)
{
	ems_int    len  = 0;
	ems_queue *list = (ems_queue *)mod->ctx;

	ems_queue_len(list, len);
	return len;
}

static ems_int
dlink_process(app_module *mod, ems_uint s, ems_uint d, ems_uint evt, json_object *root)
{
	ems_l_trace("downlink evt: 0x%x, from: 0x%x, args: %s", 
			evt, s, root?json_object_to_json_string(root):"");

	switch(evt) {
	case EMS_APP_START:
		return dlink_run(mod, EMS_YES);

	case EMS_APP_STOP:
		return dlink_run(mod, EMS_NO);

	default:
		break;
	}

	if (ems_flag_unlike(mod->flg, FLG_RUN)) {
		ems_l_trace("download link not running");
		return EMS_OK;
	}

	switch(evt) {
#if 0
	case EMS_APP_EVT_FW_RELOAD:
		return dlink_fw_reload(mod, root);
#endif

	case EMS_EVT_DOWNLINK_IN:
		return dlink_evt_user_in(mod, root);

	case EMS_EVT_DOWNLINK_NUM:
		return dlink_num(mod, root);

	default:
		break;
	}

	return EMS_OK;
}


app_module app_downlink = 
{
	.ty      = ty_downlink,
	.desc    = ems_string("downlink"),
	.ctx     = NULL,
	.flg     = 0,

	.init    = dlink_init,
	.uninit  = dlink_uninit,
	.run     = dlink_run,
	.process = dlink_process,
	.process_rule  = NULL,
	.version_match = NULL,
	.install       = NULL
};
