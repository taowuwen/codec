
#if (DT_USE_SELECT || DT_USE_EPOLL)
#include "uhttpd.h"
#include "uhttpd-utils.h"

#ifdef HAVE_TLS
#include "uhttpd-tls.h"
#endif

#include "dt_uhttpd_utils.h"

static int dt_fillbuffer_chunk(struct client *cl, const char *data, int len)
{
	int  clen;

	cl->httpbuf.wr  = cl->httpbuf.buf;
	cl->httpbuf.rd  = cl->httpbuf.buf;
	cl->httpbuf.len = 0;

	if (len > 0) {
		clen = snprintf(cl->httpbuf.wr,UH_LIMIT_MSGHEAD, "%X\r\n", len);
		cl->httpbuf.len += clen;
		cl->httpbuf.wr  += clen;

		memcpy(cl->httpbuf.wr, data, len);
		cl->httpbuf.len += len;
		cl->httpbuf.wr  += len;

		clen = snprintf(cl->httpbuf.wr, 4, "%s", "\r\n");
		cl->httpbuf.len += clen;
		cl->httpbuf.wr  += clen;

	} else {
		clen = snprintf(cl->httpbuf.wr, UH_LIMIT_MSGHEAD, "0\r\n\r\n");
		cl->httpbuf.len += clen;
		cl->httpbuf.wr  += clen;
	}

	return cl->httpbuf.len;
}

static int 
dt_getfilecontent(struct client *cl, char *buf, int len)
{
	if (cl->content_left == 0)
	{
		cl->content_left--;
		return 0;
	}

	if (cl->content_left < 0)
		return -1;

	if (cl->content_left < len)
		len = cl->content_left;

	//D("read: %d, left: %lld\n", len, cl->content_left);
	cl->content_left -= len;

	return read(cl->file, buf, len);
}


int dt_fillbuffer(struct client *cl)
{
	int         len;
	static char buf[UH_LIMIT_MSGHEAD];/* for save space and speed up*/
	struct http_request *req;

	if (cl->httpbuf.len > 0)
		return OK;

	req = &cl->request;

	len = dt_getfilecontent(cl, buf, UH_LIMIT_MSGHEAD);
	if (len < 0)
		return ERR;

	if ((req != NULL) && (req->version > UH_HTTP_VER_1_0)) {
		dt_fillbuffer_chunk(cl, buf, len);
	} else {
		memcpy(cl->httpbuf.buf, buf, len);
		cl->httpbuf.len = len;
		cl->httpbuf.rd  = cl->httpbuf.buf;
		cl->httpbuf.wr  = cl->httpbuf.rd + len;
	}

	return OK;
}

static int 
dt_tcp_send_lowlevel(struct client *cl, const char *buf, int len)
{
	if (cl && cl->fd.fd > 0)
		return write(cl->fd.fd, buf, len);
		//return send(cl->fd.fd, buf, len, 0);
	return ERR;
}

static int dt_raw_send(  struct client *cl, 
			 const char    *buf,
			 int            len,
			 int (*wfn) (struct client *, const char *, int))
{

	ssize_t rv;

	rv = wfn(cl, buf, len);

	if (rv < 0)
		return -errno;

	return rv;
}


int dt_http_send(struct client *cl, const char *buf, int len)
{
#ifdef HAVE_TLS
	if (cl->tls)
		return dt_raw_send(cl, buf, len, cl->server->conf->tls_send);
#endif
	return dt_raw_send(cl, buf, len, dt_tcp_send_lowlevel);
}

int dt_uhttpd_write(struct client *cl)
{
	int rtn;

	if (dt_fillbuffer(cl) != OK)
		return ERR;

	if (cl->httpbuf.len > 0) {
		rtn = dt_http_send(cl, cl->httpbuf.rd, cl->httpbuf.len);
		if (rtn > 0) {
			cl->httpbuf.rd  += rtn;
			cl->httpbuf.len -= rtn;
			return OK;
		}

		switch (rtn) {
			case -EINTR:
			case -EAGAIN:
#ifdef WIN32
			case -EWOULDBLOCK:
#endif
				return OK;
		}

		D("session[%d] down: [%d]%s\n",cl->fd.fd, -rtn, strerror(-rtn));
	}

	return ERR;
}
#endif
