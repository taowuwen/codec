
#include "ems_core.h"
#include "ems_cmd.h"


static ems_int cmd_connect(ems_sock *sock)
{
	ems_int      fd;
	socklen_t    len;
	struct sockaddr_in addr;

	memset(&addr, 0, sizeof(addr));

	if (ems_gethostbyname(ems_sock_addr(sock), &addr) != OK) {
		ems_l_trace("gethostbyname err for %s : %s",
				ems_sock_addr(sock), ems_lasterrmsg());
		return EMS_ERR;
	}

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd <= 0)
		return EMS_ERR;

	ems_setsock_rw_timeout(fd, 5000);

	addr.sin_family = AF_INET;
	addr.sin_port   = htons(ems_sock_port(sock));

	len = sizeof(struct sockaddr_in);

	if (connect(fd, (struct sockaddr *)&addr, len)) {
		close(fd);
		ems_l_trace("connect to: %s:%d failed: %s", 
				ems_sock_addr(sock), ems_sock_port(sock),
				ems_lasterrmsg());
		return EMS_ERR;
	}

	ems_sock_setfd(sock, fd);

	return EMS_OK;
}

static ems_int cmd_send(ems_sock *sock, ems_buffer *buf)
{
	ems_int rtn;
	ems_int total = buf_len(buf);

	rtn = ems_sock_send(sock, buf);

	if (rtn != total)
		return EMS_ERR;

	ems_buffer_refresh(buf);
	return EMS_OK;
}

static ems_int cmd_recv_n(ems_sock *sock, ems_buffer *buf, ems_uint len)
{
	ems_char *p;
	ems_int  ret, fd;
	ems_uint left = len;

	if (ems_sock_fd(sock) <= 0)
		return EMS_ERR;

	fd   = ems_sock_fd(sock);
	p    = buf_wr(buf);
	left = len;

	while (left > 0) {
		ret = recv(fd, p, left, 0);

		if (ret <= 0) {
			ems_l_trace("sess: %d recv failed: %s", fd, ems_lasterrmsg());
			ems_sock_close(sock);
			break;
		}

		left -= ret;
		p    += ret;
	}

	ret = abs(p - buf_wr(buf));
	ems_buffer_seek_wr(buf, ret, EMS_BUFFER_SEEK_CUR);

	return ret;
}

static ems_int cmd_recv(ems_sock *sock, ems_buffer *buf)
{
	ems_int      left;
	ems_response rsp;

	if (cmd_recv_n(sock, buf, SIZE_RESPONSE) != SIZE_RESPONSE) 
	{
		return EMS_ERR;
	}

	ems_buffer_prefetch(buf, (ems_char *)rsp.val, SIZE_RESPONSE);

	rsp.tag.val = ntohl(rsp.tag.val);
	rsp.len     = ntohl(rsp.len);

	left = rsp.len - SIZE_RESPONSE;

	ems_l_trace("tag: %x, len: %d, left: %d", rsp.tag.val, rsp.len, left);

	if (left > 0) {

		if (buf_left(buf) <= left)
			ems_buffer_increase(buf, left);

		if (cmd_recv_n(sock, buf, left) != left) 
			return EMS_ERR;
	}

	return EMS_OK;
}

static ems_int cmd_parse_rsp(ems_buffer *buf)
{
	json_object  *root;
	ems_response *rsp;
	ems_int       rtn;

	rsp = (ems_response *)buf_rd(buf);

	rsp->tag.val = ntohl(rsp->tag.val);
	rsp->len     = ntohl(rsp->len);
	rsp->st      = ntohl(rsp->st);

	root = NULL;
	if (rsp->len > SIZE_RESPONSE) {
		ems_assert(buf_len(buf) >= rsp->len);
		ems_int     len;
		ems_char    *p, ch;

		p = (ems_char *)(buf_rd(buf) + SIZE_RESPONSE);
		getword(p, len);

		ch = p[len]; // backup
		p[len] = '\0';
		root = ems_json_tokener_parse(p);
		p[len] = ch; // restore
	}

	ems_l_trace("\033[01;34m[sync]<rsp tag: 0x%x, st: %d ctx: %s> \033[00m",
			rsp->tag.val, rsp->st, root?json_object_to_json_string(root):"no ctx");
	rtn = rsp->st;

	if (root) {
		ems_cchar *ctx = json_object_to_json_string(root);
		fprintf(stdout, "%s\n", ctx);
		FILE *fp = fopen(JSON_RESULT, "w");

		if (fp) {
			if (ctx)
				fwrite(ctx, strlen(ctx), 1, fp);

			fclose(fp);
		}

		json_object_put(root);
	}

	ems_buffer_seek_rd(buf, rsp->len, EMS_BUFFER_SEEK_CUR);
	ems_buffer_refresh(buf);
	return rtn;

}

ems_int exec_cmd(ems_int cmd, json_object *root)
{
	ems_sock     sock;
	ems_buffer   buf;
	ems_int      rtn;
	ems_cchar   *ctx;

	ems_l_trace("\033[00;31m[sync]<req: tag: 0x%x ctx: %s> \033[00m",
			cmd, root?json_object_to_json_string(root):"no ctx");

	ems_sock_init(&sock);
	ems_buffer_init(&buf, EMS_BUFFER_1K);

	do {
		rtn = EMS_OK;
		ctx = NULL;

		ems_sock_setaddr(&sock, EMS_ADDR);
		ems_sock_setport(&sock, EMS_PORT);

		if (cmd_connect(&sock) != EMS_OK) {
			rtn = MSG_ST_CONNECT_EMS_FAILED;
			break;
		}

		if (root)
			ctx = json_object_to_json_string(root);

		ems_pack_req(cmd, ctx, ems_strlen(ctx), &buf);

		rtn = cmd_send(&sock, &buf);
		if (rtn != EMS_OK) break;

		rtn = cmd_recv(&sock, &buf);
		if (rtn != EMS_OK) break;

		rtn = cmd_parse_rsp(&buf);

	} while (0);

	ems_buffer_uninit(&buf);
	ems_sock_close(&sock);
	ems_sock_clear(&sock);

	return rtn;
}

ems_int cmd_parse_cmd(ems_int argc, ems_char **argv, json_object *req)
{
	ems_int i;
	ems_char buf[1024], *key, *val;

	for (i = 1; i < argc; i++) {
		snprintf(buf, sizeof(buf), "%s", argv[i]);

		key = buf;
		val = strchr(buf, '=');

		if (!val)
			continue;
		*val++ = '\0';

		if (strlen(key) <= 0 || strlen(val) <= 0)
			continue;

		json_object_object_add(req, key, json_object_new_string(val));
	}

	ems_l_trace("parse_cmd result: %s", json_object_to_json_string(req));

	return EMS_OK;
}

struct json_object *json_parse_from_file(ems_cchar *fl) 
{
	struct json_tokener *tok;
	struct json_object  *jobj;
	enum   json_tokener_error jerr;
	ems_buffer  buff;
	ems_int     len, size;
	FILE       *fp = NULL;
	ems_char   *buf;

	fp = fopen(fl, "r");
	if (!fp)
		return NULL;

	tok = json_tokener_new();
	if (!tok) {
		fclose(fp);
		return NULL;
	}

	ems_buffer_init(&buff, EMS_BUFFER_1K);

	buf  = buf_wr(&buff);
	size = buf_left(&buff);

	jobj = NULL;
	do {
		len = fread(buf, 1, size, fp);
		if (len <= 0)
			break;

		jobj = json_tokener_parse_ex(tok, buf, len);

	} while ((jerr = json_tokener_get_error(tok)) == json_tokener_continue);

	json_tokener_free(tok);
	ems_buffer_uninit(&buff);
	fclose(fp);

	return jobj;
}

