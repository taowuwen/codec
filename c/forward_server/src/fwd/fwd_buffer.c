
#include "fwd_main.h"
#include "fwd_buffer.h"

fwd_buffer *fwd_buffer_new(dt_uint sz)
{
	fwd_buffer *buf = NULL;

	buf = (fwd_buffer *)dt_malloc(sizeof(fwd_buffer));

	if (buf) {

		if (fwd_buffer_init(buf, sz) != FWD_OK) {
			dt_free(buf);
			buf = NULL;
		}
	}

	return buf;
}

dt_int fwd_buffer_init(fwd_buffer *buf, dt_uint sz)
{
	dt_assert(buf != NULL);

	if (!buf)
		return FWD_ERR;

	buf_head(buf) = buf_tail(buf) = buf_rd(buf) = buf_wr(buf) = NULL;

	buf_head(buf) = (dt_char *)dt_malloc(sizeof(dt_char) * sz);

	if (!buf_head(buf))
		return FWD_ERR;

	buf_tail(buf) = buf_head(buf) + sz;
	buf_wr(buf)   = buf_rd(buf) = buf_head(buf);

	return FWD_OK;
}

dt_void fwd_buffer_uninit(fwd_buffer *buf)
{
	dt_assert(buf != NULL);
	if (buf) {
		if (buf_head(buf))
			dt_free(buf_head(buf));
		buf_head(buf) = buf_tail(buf) = buf_rd(buf) = buf_wr(buf) = NULL;
	}
}

dt_void fwd_buffer_destroy(fwd_buffer *buf)
{
	if (buf) {
		fwd_buffer_uninit(buf);
		dt_free(buf);
	}
}

dt_int fwd_buffer_seek_rd(fwd_buffer *buf, dt_int off, dt_int pos)
{
	dt_assert(buf != NULL);

	if (!buf)
		return FWD_ERR;

	switch(pos) {

	case FWD_BUFFER_SEEK_SET:
		if ((off < 0) || (off > (buf_len(buf) + buf_trash(buf))))
			return FWD_ERR;

		buf_rd(buf) = buf_head(buf) + off;
		break;

	case FWD_BUFFER_SEEK_CUR:
		if ((off > buf_len(buf)) || (off < -buf_trash(buf))) {
			return FWD_ERR;
		}

		buf_rd(buf) = buf_rd(buf) + off;
		break;

	case FWD_BUFFER_SEEK_END:
		if ((off > 0) || (off < - (buf_len(buf) + buf_trash(buf)))) {
			return FWD_ERR;
		}

		buf_rd(buf) = buf_wr(buf) + off;
		break;

	default:
		dt_assert(0 && "arg error");
		return FWD_ERR;
	}

	return FWD_OK;
}

dt_int fwd_buffer_seek_wr(fwd_buffer *buf, dt_int off, dt_int pos)
{
	dt_assert(buf != NULL);

	if (!buf)
		return FWD_ERR;

	switch(pos) {

	case FWD_BUFFER_SEEK_SET:

		if (off < 0 || off > (buf_len(buf) + buf_left(buf)))
			return FWD_ERR;

		buf_wr(buf) = buf_rd(buf) + off;

		break;

	case FWD_BUFFER_SEEK_CUR:

		if (off < -buf_len(buf) || off > buf_left(buf))
			return FWD_ERR;

		buf_wr(buf) = buf_wr(buf) + off;

		break;

	case FWD_BUFFER_SEEK_END:

		if (off > 0 || (off < - (buf_len(buf) + buf_left(buf))))
			return FWD_ERR;

		buf_wr(buf) = buf_tail(buf) + off;

		break;

	default:
		dt_assert(0 && "arg error");

	}

	return FWD_OK;
}

dt_int fwd_buffer_refresh(fwd_buffer *buf)
{
	dt_int len;

	dt_assert(buf && "arg error");

	if (!buf)
		return FWD_ERR;

	if (buf_trash(buf) > 0) {

		len = buf_len(buf);

		dt_assert(buf_head(buf) != buf_rd(buf));
		memcpy(buf_head(buf), buf_rd(buf), len);
		buf_rd(buf) = buf_head(buf);
		buf_wr(buf) = buf_rd(buf) + len;

		#ifdef DEBUG
		memset(buf_wr(buf), Garbage, buf_left(buf));
		#endif
	}

	dt_assert(buf_trash(buf) == 0);

	return FWD_OK;
}

static dt_int 
buffer_fetch(fwd_buffer *buf, dt_char *dest, dt_int len, dt_int drain)
{
	if (!(buf && dest && len > 0))
		return FWD_ERR;

	if (buf_len(buf) < len)
		len =  buf_len(buf);

	memcpy(dest, buf_rd(buf), len);

	if (drain) {
	#ifdef DEBUG
		dt_int ret = 
	#endif

		fwd_buffer_seek_rd(buf, len, FWD_BUFFER_SEEK_CUR);

	#ifdef DEBUG
		dt_assert((ret == FWD_OK) && "bug in fwd_buffer_seek_rd");
	#endif
	}

	return len;
}


dt_int fwd_buffer_read(fwd_buffer *buf, dt_char *dest, dt_int len)
{
	dt_assert(buf && dest && len > 0);

	return buffer_fetch(buf, dest, len, 1);
}

dt_int fwd_buffer_prefetch(fwd_buffer *buf, dt_char *dest, dt_int len)
{
	dt_assert(buf && dest && len > 0);

	return buffer_fetch(buf, dest, len, 0);
}

dt_int fwd_buffer_write(fwd_buffer *buf, dt_cchar *src, dt_int len)
{
#ifdef DEBUG
	dt_int ret;
#endif
	dt_assert(buf && src && len >= 0);

	if (!(buf && src && len >= 0))
		return FWD_ERR;

	if (len > buf_left(buf))
		len = buf_left(buf);

	memcpy(buf_wr(buf), src, len);

#ifdef DEBUG
	ret =
#endif
	fwd_buffer_seek_wr(buf, len, FWD_BUFFER_SEEK_CUR);
#ifdef DEBUG
	dt_assert((ret == FWD_OK) && "bug in fwd_buffer_seek_wr");
#endif

	return len;
}


dt_void fwd_buffer_clear(fwd_buffer *buf)
{
	dt_assert(buf && "invlaid arg");

	buf_rd(buf) = buf_wr(buf) = buf_head(buf);
#ifndef DEBUG
	memset(buf_rd(buf), Garbage,  buf_left(buf));
#endif
}

dt_int fwd_buffer_pack( fwd_buffer         *buffer, 
			dt_uint             tag, 
			dt_int              st, 
			fwd_buffer_pack_cb  cb, 
			dt_cchar           *arg)
{
	dt_int       totallen;
	dt_int      *len;
	dt_char     *v, *wr;
	fwd_request *req = NULL;

	wr = buf_wr(buffer);

	if (buf_left(buffer) <= sizeof(fwd_request) + INTSIZE)
		return FWD_BUFFER_FULL;

	req = (fwd_request *)wr;
	len = (dt_int  *)   (wr + sizeof(fwd_request));
	v   = (dt_char *)   (wr + sizeof(fwd_request) + INTSIZE);

	if (cb) {
		if (cb(v, buf_left(buffer) - sizeof(fwd_request) -INTSIZE, arg) != FWD_OK) {
			log_warn("buffer full : 0x%x, st: %d, give up", tag, st);
			return FWD_BUFFER_FULL;
		}
	}
	else
		v = NULL;

	/* if the protocol not json, u should change this 
	   with a better way to get the excatly len.
	*/
	*len = fwd_strlen(v);
	if (*len > 0)
		totallen = sizeof(fwd_request) + INTSIZE + *len;
	else
		totallen = sizeof(fwd_request);

	fwd_buffer_seek_wr(buffer, totallen, FWD_BUFFER_SEEK_CUR);

	req->len = htons(totallen);
	req->ver = htons(FWD_VERSION);
	req->cmd = htons(tag);
	req->rsv = htons(st); /* if msg is rsp, rsrv gonna be used*/
	if (*len > 0)
		*len = htonl(*len);

	log_trace("[buf pack] tag: 0x%x, totallen: %d", tag, totallen);

	return FWD_OK;
}

dt_int fwd_buffer_unpack(fwd_buffer   *buffer,
			 fwd_request  *header,
			 dt_int       *l_val,
			 dt_char      **val)
{
	dt_int       l_buf;
	dt_char      *p, *buf;
	fwd_request  *rsp;

	dt_assert(buffer && header);

	rsp = (fwd_request *)buf_rd(buffer);

	if (buf_len(buffer) < sizeof(fwd_request))
		return FWD_CONTINUE;

	if (ntohs(rsp->len) > buf_len(buffer))
		return FWD_CONTINUE;

	header->len = ntohs(rsp->len);
	header->ver = ntohs(rsp->ver);
	header->cmd = ntohs(rsp->cmd);
	header->rsv = ntohs(rsp->rsv);

	l_buf = 0;
	buf   = NULL;
	if (ntohs(rsp->len) >= sizeof(fwd_request) + INTSIZE) {
		p = buf_rd(buffer) + sizeof(fwd_request);
		getword(p, l_buf);

		if (l_buf > (header->len - sizeof(fwd_request) - INTSIZE)) {
			return FWD_ERR;
		}

		buf = (dt_char *)dt_malloc(l_buf + 2);
		if (!buf)
			return FWD_ERR;

		memcpy(buf, p, l_buf);
		buf[l_buf] = '\0';
	}
#ifdef DEBUG
	else
		dt_assert(ntohs(rsp->len) == sizeof(fwd_request));
#endif

	if (l_val)
		*l_val = l_buf;

	if (val)
		*val = buf;
	else
		dt_free(buf);

	log_trace("[buf unpack] tag: 0x%x, len: 0x%x, ver: 0x%x, st(rsv): 0x%x, content: %d(0x%x)",
				header->cmd, header->len, header->ver, header->rsv, l_buf, l_buf);
	fwd_buffer_drain(buffer, header->len);

	return FWD_OK;
}
