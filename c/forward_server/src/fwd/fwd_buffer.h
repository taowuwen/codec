
#ifndef FWD_BUFFER_HEADER____
#define FWD_BUFFER_HEADER____

#define buf_head(buf) ((buf)->head)
#define buf_tail(buf) ((buf)->tail)
#define buf_rd(buf)   ((buf)->rd)
#define buf_wr(buf)   ((buf)->wr)

#define buf_size(buf)   abs(buf_tail(buf) - buf_head(buf))
#define buf_left(buf)	abs(buf_tail(buf) - buf_wr(buf))
#define buf_len(buf)    abs(buf_wr(buf)   - buf_rd(buf))
#define buf_trash(buf)  abs(buf_rd(buf)   - buf_head(buf))

#define FWD_BUFFER_SEEK_SET	1
#define FWD_BUFFER_SEEK_CUR	2
#define FWD_BUFFER_SEEK_END	3

fwd_buffer *fwd_buffer_new(dt_uint sz);
dt_int      fwd_buffer_init(fwd_buffer *, dt_uint sz);
dt_void     fwd_buffer_uninit(fwd_buffer *);
dt_void     fwd_buffer_destroy(fwd_buffer *);

dt_int fwd_buffer_seek_rd(fwd_buffer *, dt_int off, dt_int pos);
dt_int fwd_buffer_seek_wr(fwd_buffer *, dt_int off, dt_int pos);

dt_int fwd_buffer_refresh(fwd_buffer *);

dt_int fwd_buffer_read(fwd_buffer *, dt_char *, dt_int len);
dt_int fwd_buffer_prefetch(fwd_buffer *, dt_char *, dt_int len);
dt_int fwd_buffer_write(fwd_buffer *, dt_cchar *, dt_int len);

dt_void fwd_buffer_clear(fwd_buffer *);

#define fwd_buffer_append               fwd_buffer_write
#define fwd_buffer_append_str(buf, str)	fwd_buffer_write(buf, str, strlen(str))
#define fwd_buffer_drain(buf, len)      fwd_buffer_seek_rd(buf, len, FWD_BUFFER_SEEK_CUR)
#define fwd_buffer_reset		fwd_buffer_clear


typedef dt_int (*fwd_buffer_pack_cb)(dt_char *buf, dt_int len, dt_cchar *arg);

dt_int fwd_buffer_pack(fwd_buffer *, dt_uint tag, dt_int st, fwd_buffer_pack_cb, dt_cchar *arg);

dt_int fwd_buffer_unpack(fwd_buffer *, fwd_request *req, dt_int *l_value, dt_char **value);
				 

#endif
