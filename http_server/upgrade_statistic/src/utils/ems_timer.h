
#ifndef EMS_TIMEOUT_TIMER_HEADER___
#define EMS_TIMEOUT_TIMER_HEADER___

#define TIMEOUT_TICK	1000

typedef struct _ems_timeout_s   ems_timeout;
typedef ems_void (*ems_timeout_cb)(ems_timeout *to);


struct _ems_timeout_s
{
	ems_queue        entry;
	ems_int          pending;
	ems_timeout_cb  cb;
	struct timeval  time;
};


#define EMS_TIMEOUT_TAIL	0
#define EMS_TIMEOUT_SORT	1
#define EMS_TIMEOUT_HEAD	2

ems_int ems_timeout_init(ems_timeout *to);

ems_int ems_timeout_set(
		ems_queue      *list,
		ems_timeout    *to, 
		ems_int         msecs,
		ems_timeout_cb  cb,
		ems_uint        pos);

ems_int ems_timeout_cancel(ems_timeout *to);

#define ems_timeout_insert_tail(l, to, msecs, cb) \
	ems_timeout_set(l, to, msecs, cb, EMS_TIMEOUT_TAIL)

/*
 * never use this. if you really know what exactly you are doing
 * this may import bug for your programs
 * */
#define ems_timeout_insert_head(l, to, msecs, cb)  \
	ems_timeout_set(l, to, msecs, cb, EMS_TIMEOUT_HEAD)

#define ems_timeout_insert_sorted(l, to, msecs, cb)  \
	ems_timeout_set(l, to, msecs, cb, EMS_TIMEOUT_SORT)

#define ems_timeuot_insert	ems_timeout_insert_sorted
#define ems_timeout_append	ems_timeout_insert_tail

ems_int ems_time_diff(struct timeval *t1, struct timeval *t2);
ems_void ems_timeout_handle(ems_queue *timeout, struct timeval *tv);
ems_int ems_timeout_next(ems_queue *timeout, struct timeval *tv);

#endif
