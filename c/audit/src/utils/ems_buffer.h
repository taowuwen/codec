
#ifndef EMS_BUFFER_HEADER____
#define EMS_BUFFER_HEADER____

#define EMS_BUFFER_1K			1024
#define EMS_BUFFER_2K			2048
#define EMS_BUFFER_4k			4096
#define EMS_BUFFER_8k			8192
#define EMS_BUFFER_16k			16384
#define EMS_BUFFER_DEFAULT_SIZE		EMS_BUFFER_4k

typedef struct _ems_buffer_s    ems_buffer;

struct _ems_buffer_s
{
	ems_char        *head;
	ems_char        *tail;
	ems_char        *rd;
	ems_char        *wr;
};


#define buf_head(buf) ((buf)->head)
#define buf_tail(buf) ((buf)->tail)
#define buf_rd(buf)   ((buf)->rd)
#define buf_wr(buf)   ((buf)->wr)

#define buf_size(buf)   abs(buf_tail(buf) - buf_head(buf))
#define buf_left(buf)	abs(buf_tail(buf) - buf_wr(buf))
#define buf_len(buf)    abs(buf_wr(buf)   - buf_rd(buf))
#define buf_trash(buf)  abs(buf_rd(buf)   - buf_head(buf))

#define EMS_BUFFER_SEEK_SET	1
#define EMS_BUFFER_SEEK_CUR	2
#define EMS_BUFFER_SEEK_END	3

ems_buffer  *ems_buffer_new(ems_uint sz);
ems_int      ems_buffer_init(ems_buffer *, ems_uint sz);
ems_void     ems_buffer_uninit(ems_buffer *);
ems_void     ems_buffer_destroy(ems_buffer *);

ems_int ems_buffer_seek_rd(ems_buffer *, ems_int off, ems_int pos);
ems_int ems_buffer_seek_wr(ems_buffer *, ems_int off, ems_int pos);

ems_int ems_buffer_refresh(ems_buffer *);

ems_int ems_buffer_read(ems_buffer *, ems_char *, ems_int len);
ems_int ems_buffer_prefetch(ems_buffer *, ems_char *, ems_int len);
ems_int ems_buffer_write(ems_buffer *, ems_cchar *, ems_int len);

ems_void ems_buffer_clear(ems_buffer *);

#define ems_buffer_append               ems_buffer_write
#define ems_buffer_append_str(buf, str)	ems_buffer_write(buf, str, strlen(str))
#define ems_buffer_drain(buf, len)      ems_buffer_seek_rd(buf, len, EMS_BUFFER_SEEK_CUR)
#define ems_buffer_reset		ems_buffer_clear

ems_int ems_buffer_increase(ems_buffer *, ems_uint sz);
#define ems_buffer_expand(buf) ems_buffer_increase(buf, EMS_BUFFER_4k)


#endif
