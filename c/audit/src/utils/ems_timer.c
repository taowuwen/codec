
#include "ems_common.h"
#include "ems_timer.h"
#include <time.h>

ems_int ems_time_diff(struct timeval *t1, struct timeval *t2)
{
	return
		(t1->tv_sec - t2->tv_sec) * 1000 +
		(t1->tv_usec - t2->tv_usec) / 1000;
}

static ems_int 
ems_timeout_add(ems_queue *timeout, ems_timeout *to, ems_uint pos)
{
	if (to->pending)
		return EMS_ERR;

	to->pending = YES;

	switch (pos) {

	case EMS_TIMEOUT_SORT:
		{
			ems_queue     *p;
			ems_timeout *tmp;

			ems_queue_foreach(timeout, p) {
				tmp = ems_queue_data(p, ems_timeout, entry);

				if (ems_time_diff(&tmp->time, &to->time) > 0)
					break;
			}

			if (!p)
				p = timeout;

			ems_queue_insert_tail(p, &to->entry);
		}
		break;

	case EMS_TIMEOUT_HEAD:
		ems_queue_insert_head(timeout, &to->entry);
		break;

	default:
		ems_queue_insert_tail(timeout, &to->entry);
		break;
	}

	return EMS_OK;
}

ems_int ems_timeout_init(ems_timeout *to)
{
	memset(to, 0, sizeof(ems_timeout));
	to->pending = NO;
	return EMS_OK;
}

ems_int ems_timeout_set(
		ems_queue      *timeout,
		ems_timeout    *to, 
		ems_int         msecs,
		ems_timeout_cb  cb,
		ems_uint        pos)
{
	struct timeval *t = &to->time;

	if (to->pending)
		ems_timeout_cancel(to);

	ems_getboottime(&to->time);

	t->tv_sec  += msecs / 1000;
	t->tv_usec += (msecs % 1000) * 1000;

	if (t->tv_usec > 1000000) {
		t->tv_sec++;
		t->tv_usec %= 1000000;
	}
	to->cb = cb;

	return ems_timeout_add(timeout, to, pos);
}

ems_int 
ems_timeout_cancel(ems_timeout *to)
{
	if (!to->pending)
		return EMS_ERR;

	ems_queue_remove(&to->entry);
	to->pending = NO;

	return EMS_OK;
}

ems_void
ems_timeout_handle(ems_queue *timeout, struct timeval *tv)
{
	ems_queue    *p, *q;
	ems_timeout  *to;

	ems_queue_foreach_safe(timeout, p, q) {
		to = ems_queue_data(p, ems_timeout, entry);

		if (ems_time_diff(&to->time, tv) > 0)
			break;

		ems_timeout_cancel(to);
		if (to->cb)
			to->cb(to);
	}
}


ems_int ems_timeout_next(ems_queue *timeout,  struct timeval *tv)
{
	ems_queue    *p;
	ems_timeout *to;
	ems_int       diff;

	/* for check normally: instead of return -1: but TIMEOUT_TICK */
	if (ems_queue_empty(timeout))
		return TIMEOUT_TICK; // return -1;
	
	p  = ems_queue_head(timeout);
	to = ems_queue_data(p, ems_timeout, entry);

	diff = ems_time_diff(&to->time, tv);
	if (diff <= 0)
		return 0;

	return diff > TIMEOUT_TICK? TIMEOUT_TICK:diff;
}

ems_int ems_getboottime(struct timeval *tm)
{
	struct timespec tms;

	ems_assert(tm != NULL);

	clock_gettime(CLOCK_MONOTONIC, &tms);

	tm->tv_sec  = tms.tv_sec;
	tm->tv_usec = tms.tv_nsec / 1000;

	return 0;
}
