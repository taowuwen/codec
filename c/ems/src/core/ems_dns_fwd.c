
#include "ems_core.h"
#include "ems_client.h"
#include "ems_fw.h"
#include "ems_dns.h"

static ems_int dns_connect(ems_session *sess)
{
	int fd;
	struct sockaddr_in addr;
	ems_sock *sock = &sess->sock;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		ems_l_trace("[dns]socket err: %s", ems_lasterrmsg());
		return EMS_ERR;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family      = AF_INET;
	addr.sin_port        = htons(53);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (connect(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in))) {
		ems_l_trace("[dns] connect failed");
		close(fd);
		return EMS_ERR;
	}

	ems_sock_setaddr(sock, "127.0.0.1");
	ems_sock_setfd(sock, fd);
	return EMS_OK;
}

static ems_cchar *dns_get_name(ems_cchar *payload, ems_cchar *rsp, ems_char *name)
{
	ems_uchar len;
	ems_short offset;

	len = *payload++;

	while (len > 0) {
		if (len & 0xc0) {
			/* current is a offset */
			ems_uchar low = *payload++;
			offset = ((len & 0x03) << 8 | (low & 0xff));
			dns_get_name(rsp + offset, rsp, name);
			len = 0;
		} else {
			memcpy(name, payload, len);

			name    += len;
			payload += len;

			len = *payload++;
			if (len > 0)
				*name++ = '.';
		}
	}

	return payload;
}

static ems_cchar *dns_get_short(ems_cchar *payload, ems_short *n)
{
	*n = ntohs(*(ems_short *)payload);
	payload += sizeof(ems_short);
	return payload;
}

#if 0
static ems_cchar *dns_get_int(ems_cchar *payload, ems_int  *n)
{
	*n = ntohl(*(ems_int *)payload);
	payload += sizeof(ems_int);
	return payload;
}
#endif

static ems_cchar * 
fw_dns_get_question(ems_cchar *payload, ems_cchar *rsp, dns_question *qust)
{
	ems_char  url[512];

	memset(url, 0, sizeof(url));

	payload = dns_get_name(payload, rsp, url);
	/* lower case url */

	str_set(&qust->qname, url);

	payload = dns_get_short(payload, &qust->qtype);
	payload = dns_get_short(payload, &qust->qclass);

	return payload;
}

static ems_int 
dns_get_all_address(ems_cchar *payload, ems_cchar *rsp, struct in_addr *addr, ems_int *n_addr)
{
	ems_char   url[512];
	dns_item  *itm;
	ems_int    total = *n_addr;

	*n_addr = 0;

	while (total > 0) {
		total--;

		memset(url, 0, sizeof(url));

		payload = dns_get_name(payload, rsp, url);
		ems_l_trace("[dns] dns name: %s total: %d", url, total);

		itm = (dns_item *)payload;

		if (ntohs(itm->ty) == 1 && ntohs(itm->cls) == 1) {
			ems_assert(ntohs(itm->len) == 4);
			memcpy(&addr[*n_addr], itm->buf, 4);

			ems_l_trace("[dns] A : %s", inet_ntoa(addr[*n_addr]));

			*n_addr = *n_addr +1;
		}

		payload += sizeof(dns_item) + ntohs(itm->len);
	}

	return EMS_OK;
}

static ems_int dns_addr_exist(dns_url *url, struct in_addr ip)
{
	ems_int i;

	if (!url || !url->addr)
		return EMS_NO;

	for (i = 0; i < url->n_addr; i++) {
		if (url->addr[i].s_addr == ip.s_addr)
			return EMS_YES;
	}

	return EMS_NO;
}

static ems_int
dns_apply_new_addresss(ems_fw *fw, dns_url *url, struct in_addr *addr, ems_int n_addr)
{
	ems_int  i;

	ems_assert(n_addr > 0 && addr != NULL);

	do {
		if (n_addr != url->n_addr) break;

		for (i = 0; i < n_addr; i++) {
			if (!dns_addr_exist(url, addr[i]))
				goto do_apply;
		}

		return EMS_OK;
	} while (0);

do_apply:
	ems_l_trace("[dns] address: %s updated", str_text(&url->url));

	fw_url_set_free(fw, url, EMS_NO);

	if (url->addr) {
		ems_free(url->addr);
		url->addr   = NULL;
		url->n_addr = 0;
	}

	if (n_addr > 0 && addr){
		ems_int total;

		total = n_addr * sizeof(struct in_addr);

		url->addr = (struct in_addr *)ems_malloc(total);
		if (url->addr) {
			memcpy(url->addr, addr, total);
			url->n_addr = n_addr;
			fw_url_set_free(fw, url, EMS_YES);
		}
	}

	return EMS_OK;
}

static ems_int 
dns_update_url_addr(ems_fw *fw, dns_url *url, 
		ems_cchar *payload, dns_header *dns, ems_cchar *rsp)
{
	struct in_addr *addr = NULL; 
	ems_int         n_addr = 0;

	n_addr = ntohs(dns->ancount) + ntohs(dns->nscount) + ntohs(dns->arcount);
	ems_assert(n_addr > 0 && "never should be here");

	addr = (struct in_addr *)ems_malloc(n_addr * sizeof(struct in_addr));
	if (!addr)
		return EMS_ERR;

	ems_l_trace("[dns] current addr: %s, total n: %d", str_text(&url->url), n_addr);

	dns_get_all_address(payload, rsp, addr, &n_addr);

	if (n_addr > 0)
		dns_apply_new_addresss(fw, url, addr, n_addr);

	ems_flag_unset(url->flg, FLG_DNS_QUERY_EXPIRED);

	ems_free(addr);

	return EMS_OK;
}

static ems_int 
fw_dns_parse_and_filter(ems_fw *fw, ems_session *sess, dns_user *user, dns_header *dns)
{
	ems_buffer   *buf = &sess->buf_in;
	ems_cchar    *payload;
	dns_question  qust;
	dns_url      *url = NULL, *tmp;

	memset(&qust, 0, sizeof(qust));

	str_init(&qust.qname);

	payload = buf_rd(buf) + sizeof(dns_header);
	payload = fw_dns_get_question(payload, buf_rd(buf), &qust);

	ems_l_trace("[dns] question: %s, class: 0x%x, type: 0x%x", 
			str_text(&qust.qname), qust.qtype, qust.qclass);

	url = dns_find_url(fw, str_text(&qust.qname));

	if (url) {
		if (url->n_addr <= 0 ||
		    ems_flag_like(url->flg, FLG_DNS_QUERY_EXPIRED) ||
		    ems_flag_like(user->flg, FLG_DNS_QUERY_SELF)) 
		{
			dns_update_url_addr(fw, url, payload, dns, buf_rd(buf));
		}

		if (ems_flag_like(url->flg, FLG_DNS_IS_SUBDOMAIN)) {
			/* run LRU */
			ems_queue_remove(&url->entry);
			ems_queue_insert_tail(&fw->subdomain, &url->entry);
		}
	} else {
		if (ems_flag_like(emscorer()->flg, FLG_SUBDOMAIN_ENABLE) &&
			(tmp = fw_dns_url_select(fw, str_text(&qust.qname)))) {

			url = dns_url_new();
			if (url) {
				if (ems_flag_like(tmp->flg, FLG_DNS_IS_BLACKLIST))
					ems_flag_set(url->flg, FLG_DNS_IS_BLACKLIST);

				str_set(&url->url, str_text(&qust.qname));
				fw_dns_subdomain_append(fw, url);
				dns_update_url_addr(fw, url, payload, dns, buf_rd(buf));
			}
		}
	}

	str_uninit(&qust.qname);

	return EMS_OK;
}

static ems_int fw_dns_fwd_filter(ems_fw *fw, ems_session *sess)
{
	dns_header *dns = NULL;
	dns_user   *user = NULL;

	dns = (dns_header *)buf_rd(&sess->buf_in);
	ems_assert(DNS_QR(dns) && "must be a response");

	user = dns_find_user(fw, ntohs(dns->id));
	if (user) {
		ems_l_trace("[dns]user (%s:%d) dns replied", inet_ntoa(user->addr), user->port);

		ems_queue_remove(&user->entry);
		ems_timeout_cancel(&user->to);
		ems_hash_remove(&user->h_msg);

		ems_buffer_clear(&user->buf);
		ems_buffer_write(&user->buf, buf_rd(&sess->buf_in), buf_len(&sess->buf_in));

		if ((DNS_RCODE(dns) == 0) && (ntohs(dns->qdcount) > 0)) {
			if (    ntohs(dns->ancount) > 0 
				/*|| ntohs(dns->nscount) > 0 
				  || ntohs(dns->arcount) > 0
				 */ // not support ns and ar for currently 
			)
			{
				fw_dns_parse_and_filter(fw, sess, user, dns);
			}
		}

		if (ems_flag_like(user->flg, FLG_DNS_QUERY_SELF)) {
			dns_user_free(user);
			return EMS_OK;
		}

		ems_queue_insert_tail(&fw->out, &user->entry);
		fw_dns_server_set_write(fw, EMS_YES);
	}

	return EMS_OK;
}

static ems_int fw_dns_fwd_rsp(ems_fw *fw, ems_session *sess)
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
			ems_l_trace("[dns] recv error: %s", ems_lasterrmsg());
			return EMS_ERR;
		}
	}

	ems_buffer_seek_wr(&sess->buf_in, ret, EMS_BUFFER_SEEK_CUR);

	ret = fw_dns_fwd_filter(fw, sess);

	ems_buffer_clear(&sess->buf_in);

	goto again;

	return EMS_OK;
}

static ems_int fw_dns_fwd_req(ems_fw *fw, ems_session *sess)
{
	dns_user  *user = NULL;
	ems_queue *p, *q;
	ems_int    ret;

	ems_queue_foreach_safe(&fw->fwd, p, q) {
		user = ems_container_of(p, dns_user, entry);

		ems_assert(buf_len(&user->buf) > 0);
		ret = send(ems_sock_fd(&sess->sock), buf_rd(&user->buf), buf_len(&user->buf), 0);
		if (ret <= 0) {
			break;
		}

		ems_buffer_clear(&user->buf);
		ems_queue_remove(&user->entry);
		ems_queue_insert_tail(&fw->wait, &user->entry);
	}

	if (!ems_queue_empty(&fw->wait))
		fw_dns_client_set_read(fw, EMS_YES);

	if (ems_queue_empty(&fw->fwd))
		fw_dns_client_set_write(fw, EMS_NO);

	return EMS_OK;
}

static ems_void fw_dns_fwd_evt_cb(ems_session *sess, ems_int err, ems_int flg)
{
	ems_fw *fw = (ems_fw *)sess_cbarg(sess);

	if (err) {
		ems_l_trace("[dns] client dns err sess: %d", ems_sock_fd(&sess->sock));
		/* do reconnect */
		fw_dns_client_stop(fw);
		fw_dns_be_client(fw);
		return;
	}

	if (ems_flag_like(flg, EMS_EVT_READ)) {
		fw_dns_fwd_rsp(fw, sess);
	}

	if (ems_flag_like(flg, EMS_EVT_WRITE)) {
		fw_dns_fwd_req(fw, sess);
	}
}

ems_int fw_dns_be_client(ems_fw *fw)
{
	ems_session *sess;

	if (!fw->sess_dns)
		fw->sess_dns = ems_session_new();

	ems_assert(fw->sess_dns != NULL);
	sess = fw->sess_dns;

	if (dns_connect(sess) != EMS_OK) {
		ems_session_shutdown_and_destroy(sess);
		fw->sess_dns = NULL;
		return EMS_ERR;
	}

	sess_cbarg_set(sess, fw);
	sess_event_set(sess, EMS_EVT_READ, fw_dns_fwd_evt_cb);

	ems_l_trace("[dns] dns client (%d) %s", 
			ems_sock_fd(&sess->sock), ems_sock_addr(&sess->sock));

	return EMS_OK;
}

ems_int fw_dns_client_stop(ems_fw *fw)
{
	if (fw->sess_dns) {
		ems_session_shutdown_and_destroy(fw->sess_dns);
		fw->sess_dns = NULL;
	}

	return EMS_OK;
}

ems_int fw_dns_client_set_read(ems_fw *fw, ems_int rd)
{
	ems_session *sess = fw->sess_dns;
	ems_int evt;

	evt = 0;
	sess_event_cancel(sess);
	if (rd)
		evt |= EMS_EVT_READ;

	if (!ems_queue_empty(&fw->fwd))
		evt |= EMS_EVT_WRITE;

	if (evt)
		sess_event_set(sess, evt, fw_dns_fwd_evt_cb);

	return EMS_OK;
}

ems_int fw_dns_client_set_write(ems_fw *fw, ems_int wr)
{
	ems_session *sess = fw->sess_dns;
	ems_int evt;

	evt = 0;
	sess_event_cancel(sess);
	if (wr)
		evt |= EMS_EVT_WRITE;

	if (!ems_queue_empty(&fw->wait))
		evt |= EMS_EVT_READ;

	if (evt)
		sess_event_set(sess, evt, fw_dns_fwd_evt_cb);

	return EMS_OK;
}
