#include "ems_core.h"
#include "ems_client.h"
#include "ems_fw.h"
#include "ems_dns.h"

static ems_void dns_fwd_timeout(ems_timeout *timeout)
{
	dns_user *user = ems_container_of(timeout, dns_user, to);

	ems_l_trace("[dns]---->>>>>>>> [DNS]TIMEOUT for user(%s:%d) <<<<<<<<----", 
			inet_ntoa(user->addr), user->port);

	ems_queue_remove(&user->entry);
	ems_hash_remove(&user->h_msg);
	dns_user_free(user);
}


static ems_void dns_schedule_timeout_cb(ems_session *sess, ems_timeout *to)
{
	ems_fw *fw = (ems_fw *)sess_cbarg(sess);

	fw_dns_query_triger(fw);
	sess_timeout_set_sorted(sess, 10000, dns_schedule_timeout_cb);
}

static ems_int dns_bind(ems_session *sess)
{
	int fd, rc, opt = 1;
	struct sockaddr_in addr;
	ems_sock *sock = &sess->sock;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		return EMS_ERR;

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
	{
		ems_l_trace("[dns]setsockopt err: %s", ems_lasterrmsg());
		close(fd);
		return EMS_ERR;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port   = htons((ems_ushort)ems_sock_port(sock));
	addr.sin_addr.s_addr = htonl(0);

	if ((rc = bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in))) == -1)
	{
		ems_l_trace("[dns]err bind: %s", ems_lasterrmsg());
		close(fd);
		return EMS_ERR;
	}

	ems_sock_setaddr(sock, "0.0.0.0");
	ems_sock_setfd(sock, fd);
	return EMS_OK;
}

static  ems_int fw_dns_push_fwd(ems_fw *fw, dns_user *user)
{
	ems_assert(fw && user);

	ems_queue_insert_tail(&fw->fwd, &user->entry);
	ems_hash_insert(&fw->hash_msg,  &user->h_msg);

	user->ctx = (ems_void *)fw;
	ems_timeout_set(timeouter(), &user->to, 6000, dns_fwd_timeout, EMS_TIMEOUT_SORT);

	return EMS_OK;
}

/*
   1. push msgs onto queue
   2. set fwd write evt
 */

static ems_int 
fw_dns_fwd(ems_fw *fw, ems_session *sess, struct sockaddr_in *from)
{
	dns_header *dns = NULL;
	dns_user   *user = NULL;

	dns = (dns_header *)buf_rd(&sess->buf_in);

	user = dns_find_user(fw, ntohs(dns->id));
	if (user) {
		ems_l_trace("[dns]resend dns request to user: (%s: %d): 0x%x", 
				inet_ntoa(user->addr), user->port, ntohs(dns->id));
		ems_queue_remove(&user->entry);
		ems_buffer_clear(&user->buf);
		ems_timeout_cancel(&user->to);
	} else {
		user = dns_user_new();
		if (!user)
			return EMS_OK;

		memcpy(&user->addr, &from->sin_addr, sizeof(struct in_addr));
		user->port = ntohs(from->sin_port);

		ems_l_trace("[dns] new request: (%s:%d) id: 0x%x",  
				inet_ntoa(user->addr), user->port, ntohs(dns->id));
		ems_hash_fd_set_key(&user->h_msg, ems_hash_key(0xffff & ntohs(dns->id)));
	}

	ems_buffer_write(&user->buf, buf_rd(&sess->buf_in), buf_len(&sess->buf_in));

	fw_dns_push_fwd(fw, user);
	fw_dns_client_set_write(fw, EMS_YES);

	return EMS_OK;
}


static ems_int fw_dns_request(ems_fw *fw, ems_session *sess)
{
	socklen_t  len;
	struct sockaddr_in from;
	int ret;
	ems_buffer  *buf = &sess->buf_in;

	ems_assert(fw->sess_bind == sess);

	len = sizeof(from);
again:
	ret = recvfrom(ems_sock_fd(&sess->sock), buf_wr(buf), buf_left(buf), 0,
			(struct sockaddr *)&from, &len);

	if (ret <= 0) {
		return EMS_OK;
	}

	ems_buffer_seek_wr(buf, ret, EMS_BUFFER_SEEK_CUR);

	fw_dns_fwd(fw, sess, &from);

	ems_buffer_clear(buf);

	goto again;

	return EMS_OK;
}

static ems_int fw_dns_response(ems_fw *fw, ems_session *sess)
{
	dns_user  *user = NULL;
	ems_queue *p, *q;
	int        ret;
	struct sockaddr_in from;

	memset(&from, 0, sizeof(from));

	ems_queue_foreach_safe(&fw->out, p, q) {
		user = ems_container_of(p, dns_user, entry);

		ems_assert(buf_len(&user->buf) > 0);

		memcpy(&from.sin_addr, &user->addr, sizeof(user->addr));
		from.sin_port = htons(user->port);

		ret = sendto(ems_sock_fd(&sess->sock),
				buf_rd(&user->buf), 
				buf_len(&user->buf),
				0, 
				(struct sockaddr *)&from, 
				sizeof(struct sockaddr_in));

		if (ret <= 0)
			break;

		ems_buffer_clear(&user->buf);
		ems_queue_remove(&user->entry);
		dns_user_free(user);
	}

	if (ems_queue_empty(&fw->out))
		fw_dns_server_set_write(fw, EMS_NO);

	return EMS_OK;
}

static ems_void fw_dns_evt_cb(ems_session *sess, ems_int err, ems_int flg)
{
	ems_fw *fw = (ems_fw *)sess_cbarg(sess);

	if (err) {
		ems_l_trace("[dns] server dns err sess: %d", ems_sock_fd(&sess->sock));

		fw_dns_server_stop(fw);
		fw_dns_be_server(fw);
		return;
	}

	if (ems_flag_like(flg, EMS_EVT_READ)) {
		fw_dns_request(fw, sess);
	}

	if (ems_flag_like(flg, EMS_EVT_WRITE)) {
		fw_dns_response(fw, sess);
	}
}

ems_int fw_dns_be_server(ems_fw *fw)
{
	ems_session *sess;
	ems_int      dst_port;

	if (!fw->sess_bind)
		fw->sess_bind = ems_session_new();

	sess = fw->sess_bind;

	dst_port = 9300;
again:
	ems_sock_setport(&sess->sock, dst_port);

	if (dns_bind(sess) != EMS_OK) {
		dst_port++;
		if (dst_port <= 9320)
			goto again;

		ems_session_shutdown_and_destroy(sess);
		fw->sess_bind = NULL;
		return EMS_ERR;
	}

	sess_cbarg_set(sess, fw);
	sess_event_set(sess, EMS_EVT_READ, fw_dns_evt_cb);

	/* requery, set expired */
	sess_timeout_set_sorted(sess, 10000, dns_schedule_timeout_cb);

	ems_l_trace("[dns] dns server (%d) %s", 
			ems_sock_fd(&sess->sock), ems_sock_addr(&sess->sock));

	return EMS_OK;
}

ems_int fw_dns_server_stop(ems_fw *fw)
{
	if (fw->sess_bind) {
		ems_session_shutdown_and_destroy(fw->sess_bind);
		fw->sess_bind = NULL;
	}

	return EMS_OK;
}

ems_int fw_dns_server_set_write(ems_fw *fw, ems_int wr)
{
	ems_session *sess = fw->sess_bind;
	ems_int  evt = 0;

	evt |= EMS_EVT_READ;
	if (wr)
		evt |= EMS_EVT_WRITE;

	sess_event_set(sess, evt, fw_dns_evt_cb);

	return EMS_OK;
}

static ems_int dns_set_question(ems_buffer *buf, ems_cchar *url)
{
	ems_cchar *p, *q;
	ems_int   len = 0;

	while (*url) {
		q = url;
		p = strchr(q, '.');

		if (!p) {
			len = strlen(q);
			ems_buffer_write(buf, (ems_cchar *)&len, 1);
			ems_buffer_write(buf, q, len);
			break;
		}

		if (p) {
			url = p + 1;

			len = abs(p - q);
			ems_buffer_write(buf, (ems_cchar *)&len, 1);
			ems_buffer_write(buf, q, len);
		}
	}

	len = 0;
	ems_buffer_write(buf, (ems_cchar *)&len, 1);

	return EMS_OK;
}

static ems_int fw_dns_create_user(ems_fw *fw, dns_url *url, ems_short id)
{
	dns_header *dns  = NULL;
	dns_user   *user = NULL;
	ems_short   tmp  = 0;

	user = dns_user_new();
	if (!user)
		return EMS_ERR;

	dns = (dns_header *)buf_rd(&user->buf);

	ems_flag_set(user->flg, FLG_DNS_QUERY_SELF);

	user->addr.s_addr = inet_addr("127.0.0.1");
	user->port = 0;

	dns->id      = htons(id);
	dns->flg     = htons(0x0100);
	dns->qdcount = htons(0x0001);
	dns->ancount = htons(0);
	dns->nscount = htons(0);
	dns->arcount = htons(0);

	ems_buffer_seek_wr(&user->buf, sizeof(dns_header), EMS_BUFFER_SEEK_CUR);
	
	dns_set_question(&user->buf, str_text(&url->url));
	tmp = htons(0x0001);
	ems_buffer_write(&user->buf, (ems_cchar *)&tmp, sizeof(tmp));
	ems_buffer_write(&user->buf, (ems_cchar *)&tmp, sizeof(tmp));

	ems_l_trace("[dns] triger query: %s", str_text(&url->url));

	ems_hash_fd_set_key(&user->h_msg, ems_hash_key(0xffff & ntohs(dns->id)));

	fw_dns_push_fwd(fw, user);

	return EMS_OK;
}

static ems_int fw_dns_url_info_upt(dns_url *url)
{
	static ems_char buf[512];
	ems_cchar *addr, *mask;

	addr = str_text(&url->url);

	mask = strchr(addr, '/');
	if (mask) {
		mask++;

		if (*mask) {
			ems_int len = abs(mask - addr) - 1;
			if (len >= sizeof(buf))
				len = sizeof(buf) - 1;

			memset(buf, 0, sizeof(buf));
			memcpy(buf, addr, len);

			url->mask = ems_atoi(mask);
			if (url->mask < 0 || url->mask > 32) {
				ems_l_trace("[dns] invalid netmask: %d", url->mask);
				return EMS_ERR;
			}

			ems_flag_set(url->flg, FLG_URL_IS_IPADDRESS);
			str_set(&url->url, buf);
		}
	}

	return EMS_OK;
}

ems_int fw_dns_query_triger(ems_fw *fw)
{
	dns_url    *url  = NULL;
	ems_queue  *p;
	struct in_addr   addr;
	ems_short   id;
	id = rand() % 35000;

	ems_queue_foreach(&fw->whitelist, p) {
		url = ems_container_of(p, dns_url, entry);

		if (ems_flag_like(url->flg, FLG_URL_IS_IPADDRESS))
			continue;

		if (url->addr) {
			if (ems_flag_unlike(url->flg, FLG_URL_IS_IPADDRESS)) {
				ems_flag_set(url->flg, FLG_DNS_QUERY_EXPIRED); /* COW */
			}
		} else {

			if (fw_dns_url_info_upt(url) != EMS_OK)
				continue;

			addr.s_addr = inet_addr(str_text(&url->url));
			if (addr.s_addr == INADDR_NONE) {
				if (ems_flag_unlike(url->flg, FLG_URL_IS_IPADDRESS)) {
					fw_dns_create_user(fw, url, id++);
				}
			} else {
				url->addr = (struct in_addr *)ems_malloc(sizeof(addr));
				if (url->addr) {
					memcpy(url->addr, &addr, sizeof(addr));
					url->n_addr = 1;
					ems_flag_set(url->flg, FLG_URL_IS_IPADDRESS);
					fw_url_set_free(fw, url, EMS_YES);
				}
			}
		}
	}

	ems_queue_foreach(&fw->subdomain, p) {
		url = ems_container_of(p, dns_url, entry);
		ems_flag_set(url->flg, FLG_DNS_QUERY_EXPIRED);
	}

	if (!ems_queue_empty(&fw->fwd))
		fw_dns_client_set_write(fw, EMS_YES);

	return EMS_OK;
}


ems_int fw_dns_subdomain_append(ems_fw *fw, dns_url *url)
{
	ems_l_trace("[dns] append url: %s", str_text(&url->url));

	ems_flag_set(url->flg, FLG_DNS_IS_SUBDOMAIN);
	ems_hash_fd_set_key(&url->h_url, str_text(&url->url));
	ems_hash_insert(&fw->hash_url, &url->h_url);

	fw->n_subdomain++;
	ems_queue_insert_tail(&fw->subdomain, &url->entry);

	if (fw->n_subdomain > MAX_SUB_DOMAIN) {
		fw->n_subdomain--;

		url = ems_container_of(ems_queue_head(&fw->subdomain), dns_url, entry);
		ems_queue_remove(&url->entry);
		ems_hash_remove(&url->h_url);
		fw_url_set_free(fw, url, EMS_NO);
		ems_l_trace("[dns] LRU triger remove url: %s", str_text(&url->url));
		dns_url_free(url);
	}

	return EMS_OK;
}

dns_url *fw_dns_url_select(ems_fw *fw, ems_cchar *key)
{
	dns_url   *url;
	ems_queue *p;

	ems_queue_foreach(&fw->whitelist, p) {
		url = ems_container_of(p, dns_url, entry);

#if 1
		if (ems_flag_like(url->flg, FLG_URL_IS_IPADDRESS) || url->n_addr <= 0) 
		{
			continue;
		}
#else
		if (ems_flag_like(url->flg, FLG_URL_IS_IPADDRESS))
		{
			continue;
		}
#endif

		if (strstr(key, str_text(&url->url)))
			return url;
	}

	return NULL;
}


ems_int fw_url_in_whitelist(ems_fw *fw, ems_cchar *key)
{
	dns_url *url = NULL, *tmp;
#if 0
	struct addrinfo *res, hint, *cur;
	ems_int   n_addr, i;
#endif

	url = dns_find_url(fw, key);
	if (url) {
		if (ems_flag_like(url->flg, FLG_DNS_IS_SUBDOMAIN)) {
			/* run LRU */
			ems_queue_remove(&url->entry);
			ems_queue_insert_tail(&fw->subdomain, &url->entry);

			return EMS_YES;
		}

		return EMS_NO;
	}

	if (ems_flag_like(emscorer()->flg, FLG_SUBDOMAIN_ENABLE) &&
	    (tmp = fw_dns_url_select(fw, key))) 
	{
		url = dns_url_new();
		if (!url)
			return EMS_NO;

		if (ems_flag_like(tmp->flg, FLG_DNS_IS_BLACKLIST))
			ems_flag_set(url->flg, FLG_DNS_IS_BLACKLIST);

		str_set(&url->url, key);
		fw_dns_subdomain_append(fw, url);
#if 1
		{
			fw_dns_create_user(fw, url, rand()%65535);
			fw_dns_client_set_write(fw, EMS_YES);
		}
#else
		memset(&hint, 0, sizeof(hint));

		hint.ai_family   = AF_INET;
		hint.ai_socktype = SOCK_DGRAM | SOCK_STREAM;
		hint.ai_flags    = AI_PASSIVE;

		if (getaddrinfo(str_text(&url->url), NULL, &hint, &res)) 
			return EMS_NO;

		n_addr = 0;
		for (cur = res; cur != NULL; cur = cur->ai_next)
			n_addr++;

		url->addr =(struct in_addr *)ems_malloc(sizeof(struct in_addr) * n_addr);
		if (!url->addr) {
			freeaddrinfo(res);
			return EMS_NO;
		}

		url->n_addr = n_addr;

		i = 0;
		for (cur = res; cur != NULL; cur = cur->ai_next) 
		{
			memcpy( &url->addr[i], 
				&((struct sockaddr_in *)(cur->ai_addr))->sin_addr, 
				 sizeof(struct in_addr));
			i++;
		}

		freeaddrinfo(res);
		fw_url_set_free(fw, url, EMS_YES);
#endif
		return EMS_YES;
	}

	return EMS_NO;
}

ems_int fw_url_whitelist_insert(ems_fw *fw, ems_cchar *white)
{
	dns_url   *url;
	struct in_addr   addr;

	url = dns_url_new();
	if (!url)
		return EMS_NO;

	str_set(&url->url, white);
	fw_dns_subdomain_append(fw, url);

	addr.s_addr = inet_addr(str_text(&url->url));
	if (addr.s_addr == INADDR_NONE) {
		fw_dns_create_user(fw, url, rand()%65535);
		fw_dns_client_set_write(fw, EMS_YES);
	} else {
		url->addr = (struct in_addr *)ems_malloc(sizeof(addr));
		if (url->addr) {
			memcpy(url->addr, &addr, sizeof(addr));
			url->n_addr = 1;
			ems_flag_set(url->flg, FLG_URL_IS_IPADDRESS);
			fw_url_set_free(fw, url, EMS_YES);
		}
	}

	return EMS_OK;
}
