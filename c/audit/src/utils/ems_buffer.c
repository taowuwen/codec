
#include "ems_common.h"
#include "ems_buffer.h"

ems_buffer *ems_buffer_new(ems_uint sz)
{
	ems_buffer *buf = NULL;

	buf = (ems_buffer *)ems_malloc(sizeof(ems_buffer));

	if (buf) {

		if (ems_buffer_init(buf, sz) != EMS_OK) {
			ems_free(buf);
			buf = NULL;
		}
	}

	return buf;
}

ems_int ems_buffer_init(ems_buffer *buf, ems_uint sz)
{
	ems_assert(buf != NULL);

	if (!buf)
		return EMS_ERR;

	buf_head(buf) = buf_tail(buf) = buf_rd(buf) = buf_wr(buf) = NULL;

	buf_head(buf) = (ems_char *)ems_malloc(sizeof(ems_char) * sz);

	if (!buf_head(buf))
		return EMS_ERR;

	buf_tail(buf) = buf_head(buf) + sz;
	buf_wr(buf)   = buf_rd(buf) = buf_head(buf);

	return EMS_OK;
}

ems_void ems_buffer_uninit(ems_buffer *buf)
{
	ems_assert(buf != NULL);
	if (buf) {
		if (buf_head(buf))
			ems_free(buf_head(buf));
		buf_head(buf) = buf_tail(buf) = buf_rd(buf) = buf_wr(buf) = NULL;
	}
}

ems_void ems_buffer_destroy(ems_buffer *buf)
{
	if (buf) {
		ems_buffer_uninit(buf);
		ems_free(buf);
	}
}

ems_int ems_buffer_seek_rd(ems_buffer *buf, ems_int off, ems_int pos)
{
	ems_assert(buf != NULL);

	if (!buf)
		return EMS_ERR;

	switch(pos) {

	case EMS_BUFFER_SEEK_SET:
		if ((off < 0) || (off > (buf_len(buf) + buf_trash(buf))))
			return EMS_ERR;

		buf_rd(buf) = buf_head(buf) + off;
		break;

	case EMS_BUFFER_SEEK_CUR:
		if ((off > buf_len(buf)) || (off < -buf_trash(buf))) {
			return EMS_ERR;
		}

		buf_rd(buf) = buf_rd(buf) + off;
		break;

	case EMS_BUFFER_SEEK_END:
		if ((off > 0) || (off < - (buf_len(buf) + buf_trash(buf)))) {
			return EMS_ERR;
		}

		buf_rd(buf) = buf_wr(buf) + off;
		break;

	default:
		ems_assert(0 && "arg error");
		return EMS_ERR;
	}

	return EMS_OK;
}

ems_int ems_buffer_seek_wr(ems_buffer *buf, ems_int off, ems_int pos)
{
	ems_assert(buf != NULL);

	if (!buf)
		return EMS_ERR;

	switch(pos) {

	case EMS_BUFFER_SEEK_SET:

		if (off < 0 || off > (buf_len(buf) + buf_left(buf)))
			return EMS_ERR;

		buf_wr(buf) = buf_rd(buf) + off;

		break;

	case EMS_BUFFER_SEEK_CUR:

		if (off < -buf_len(buf) || off > buf_left(buf))
			return EMS_ERR;

		buf_wr(buf) = buf_wr(buf) + off;

		break;

	case EMS_BUFFER_SEEK_END:

		if (off > 0 || (off < - (buf_len(buf) + buf_left(buf))))
			return EMS_ERR;

		buf_wr(buf) = buf_tail(buf) + off;

		break;

	default:
		ems_assert(0 && "arg error");

	}

	return EMS_OK;
}

ems_int ems_buffer_refresh(ems_buffer *buf)
{
	ems_int len;

	ems_assert(buf && "arg error");

	if (!buf)
		return EMS_ERR;

	if (buf_trash(buf) > 0) {

		len = buf_len(buf);

		ems_assert(buf_head(buf) != buf_rd(buf));
		memcpy(buf_head(buf), buf_rd(buf), len);
		buf_rd(buf) = buf_head(buf);
		buf_wr(buf) = buf_rd(buf) + len;

		#ifdef DEBUG
		memset(buf_wr(buf), Garbage, buf_left(buf));
		#endif
	}

	ems_assert(buf_trash(buf) == 0);

	return EMS_OK;
}

static ems_int 
buffer_fetch(ems_buffer *buf, ems_char *dest, ems_int len, ems_int drain)
{
	if (!(buf && dest && len > 0))
		return EMS_ERR;

	if (buf_len(buf) < len)
		len =  buf_len(buf);

	memcpy(dest, buf_rd(buf), len);

	if (drain) {
	#ifdef DEBUG
		ems_int ret = 
	#endif

		ems_buffer_seek_rd(buf, len, EMS_BUFFER_SEEK_CUR);

	#ifdef DEBUG
		ems_assert((ret == EMS_OK) && "bug in ems_buffer_seek_rd");
	#endif
	}

	return len;
}


ems_int ems_buffer_read(ems_buffer *buf, ems_char *dest, ems_int len)
{
	ems_assert(buf && dest && len > 0);

	return buffer_fetch(buf, dest, len, 1);
}

ems_int ems_buffer_prefetch(ems_buffer *buf, ems_char *dest, ems_int len)
{
	ems_assert(buf && dest && len > 0);

	return buffer_fetch(buf, dest, len, 0);
}

ems_int ems_buffer_write(ems_buffer *buf, ems_cchar *src, ems_int len)
{
#ifdef DEBUG
	ems_int ret;
#endif
	ems_assert(buf && src && len >= 0);

	if (!(buf && src && len >= 0))
		return EMS_ERR;

	if (len > buf_left(buf))
		len = buf_left(buf);

	memcpy(buf_wr(buf), src, len);

#ifdef DEBUG
	ret =
#endif
	ems_buffer_seek_wr(buf, len, EMS_BUFFER_SEEK_CUR);
#ifdef DEBUG
	ems_assert((ret == EMS_OK) && "bug in ems_buffer_seek_wr");
#endif

	return len;
}


ems_void ems_buffer_clear(ems_buffer *buf)
{
	ems_assert(buf && "invlaid arg");

	buf_rd(buf) = buf_wr(buf) = buf_head(buf);
#ifndef DEBUG
	memset(buf_rd(buf), Garbage,  buf_left(buf));
#endif
}

ems_int ems_buffer_increase(ems_buffer *buf, ems_uint inc)
{
	ems_uint total;
	ems_char *tmp = NULL;
	ems_uint len;
	ems_assert(buf);

	ems_buffer_refresh(buf);

	if (buf_left(buf) > inc)
		return EMS_OK;

	len   = buf_len(buf);
	total = buf_size(buf) + inc; 

	tmp = (ems_char *)ems_realloc(buf_head(buf), total);
	if (tmp) {
		buf_head(buf) = buf_rd(buf) = tmp;
		buf_tail(buf) = buf_head(buf) + total;
		buf_wr(buf)   = buf_rd(buf) + len;

		return EMS_OK;
	}

	return EMS_ERR;
}

