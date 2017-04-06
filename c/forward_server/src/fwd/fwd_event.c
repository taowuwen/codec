
#include "fwd.h"
#include "fwd_event.h"


#ifndef EPOLLRDHUP
#define EPOLLRDHUP	0x2000
#endif

static dt_void 
fwd_timeout_clear(fwd_event *evt)
{
	dt_queue *n, *cur;

	dt_queue_foreach_safe(&evt->timeout, cur, n) {
		dt_queue_remove(cur);
	}
}

static dt_int
fwd_event_register(fwd_event *evt, fwd_event_fd *fd, dt_uint flags)
{
	struct epoll_event ev;
	int op = fd->reg ? EPOLL_CTL_MOD: EPOLL_CTL_ADD;

	memset(&ev, 0, sizeof(struct epoll_event));

	if (flags & FWD_EVT_READ)
		ev.events |= (EPOLLIN | EPOLLRDHUP);

	if (flags & FWD_EVT_WRITE)
		ev.events |= EPOLLOUT;

	if (flags & FWD_EVT_EDGE_TRIGGER)
		ev.events |= EPOLLET;

	ev.data.fd  = fd->fd;
	ev.data.ptr = fd;

	return epoll_ctl(evt->fd, op, fd->fd, &ev);
}

static dt_void
fwd_event_handle_timeouts(fwd_event *evt, struct timeval *tv)
{
	dt_queue     *p;
	fwd_timeout  *to;

	while (!dt_queue_empty(&evt->timeout)) {

		p = dt_queue_head(&evt->timeout);
		to = dt_queue_data(p, fwd_timeout, list);

		if (fwd_time_diff(&to->time, tv) > 0)
			break;

		fwd_timeout_cancel(to);
		if (to->cb)
			to->cb(to);
	}
}

#define EVENT_TICK	1000

static dt_int
fwd_event_next_timeout(fwd_event *evt, struct timeval *tv)
{
	dt_queue    *p;
	fwd_timeout *to;
	dt_int       diff;

	/* for check normally: instead of return -1: but EVENT_TICK */
	if (dt_queue_empty(&evt->timeout))
		return EVENT_TICK; // return -1;
	
	p  = dt_queue_head(&evt->timeout);
	to = dt_queue_data(p, fwd_timeout, list);

	diff = fwd_time_diff(&to->time, tv);
	if (diff <= 0)
		return 0;

	return diff > EVENT_TICK? EVENT_TICK:diff;
}

static dt_void 
fwd_event_handle_evt(fwd_event *evt, dt_int timeout)
{
	dt_int  nfds, n;
	dt_uint flg, events;

	fwd_event_fd *fd = NULL;

	nfds = epoll_wait(evt->fd, evt->evt, evt->n_evt, timeout);
	evt->cur_nfds = nfds;

	for (n = 0; n < nfds; n++) {

		flg    = 0;
		fd     = evt->evt[n].data.ptr;
		events = evt->evt[n].events;

		if (!fd)
			continue;

		evt->cur_fd = n;

		if (events & (EPOLLERR | EPOLLHUP)) {
			fd->error = YES;
			log_err("epoll error: %s", fwd_lasterrstr());
			fwd_event_del(evt, fd);
		}

		if (!(events & (EPOLLRDHUP | EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP)))
			continue;

		if (events & EPOLLRDHUP)
			fd->eof = YES;

		if (events & EPOLLIN)
			flg |= FWD_EVT_READ;

		if (events & EPOLLOUT)
			flg |= FWD_EVT_WRITE;

		if (fd->cb)
			fd->cb(fd, flg);
	}
}

dt_int fwd_event_init(fwd_event *evt)
{
	dt_assert(evt != NULL);
	memset(evt, 0, sizeof(fwd_event));

	evt->fd = epoll_create1(EPOLL_CLOEXEC);
	if (evt->fd <= 0) {
		log_err("epoll_create1 failed: %s", fwd_lasterrstr());
		return FWD_ERR;
	}

	dt_queue_init(&evt->timeout);

	evt->n_evt = FWD_EVT_SIZE;

	evt->evt = (struct epoll_event *)
			dt_malloc(sizeof(struct epoll_event) * evt->n_evt);
	if (!evt->evt) {
		close(evt->fd);
		return FWD_ERR;
	}

	evt->run      = YES;
	evt->cur_fd   = 0;
	evt->cur_nfds = 0;

	return FWD_OK;
}

dt_int fwd_event_done(fwd_event *evt)
{
	evt->run      = NO;
	evt->cur_fd   = 0;
	evt->cur_nfds = 0;

	if (evt->fd > 0) {
		close(evt->fd);
		evt->fd = 0;
	}

	if (evt->evt) {
		dt_free(evt->evt);
		evt->evt = NULL;
	}

	fwd_timeout_clear(evt);

	return FWD_OK;
}

dt_int fwd_event_run(fwd_event *evt, fwd_event_handle_extra_cb proc)
{
	struct timeval tv;

	evt->run = YES;
	while (evt->run) {

		if (proc) {
			proc(evt);
		
			if (!evt->run) break;
		}

		gettimeofday(&tv, NULL);
		fwd_event_handle_timeouts(evt, &tv);

		if (!evt->run) break;

		fwd_event_handle_evt(evt, fwd_event_next_timeout(evt, &tv));
	}

	return FWD_OK;
}


dt_int fwd_event_add(fwd_event *evt, 
		     fwd_event_fd *fd, 
		     dt_uint flags,
		     fwd_event_cb cb)
{
	dt_uint fl;

	if (!fd->reg && !(flags & FWD_EVT_BLOCKING)) {
		fl  = fcntl(fd->fd, F_GETFL, 0);
		fl |= O_NONBLOCK;
		fcntl(fd->fd, F_SETFL, fl);
	}

	if (fd->reg) {
		if (fd->flags == flags)
			return FWD_OK;
	}

	fd->cb = cb;
	if (fwd_event_register(evt, fd, flags) < 0)
		return FWD_ERR;

	fd->reg = YES;
	fd->eof = NO;
	fd->flags = flags;
	fd->error = NO;

	return FWD_OK;
}


dt_int fwd_event_del(fwd_event *evt, fwd_event_fd *fd)
{
	dt_int i;

	dt_assert(evt && fd);

	if (!fd->reg)
		return FWD_OK;

	for (i = evt->cur_fd + 1; i < evt->cur_nfds; i++) {
		if (evt->evt[i].data.ptr != fd)
			continue;

		evt->evt[i].data.ptr = NULL;
	}

	fd->reg = NO;
	fd->flags = 0;

	return epoll_ctl(evt->fd, EPOLL_CTL_DEL, fd->fd, NULL);
}

static dt_int 
fwd_timeout_add(fwd_event *evt, fwd_timeout *to, dt_uint pos)
{
	if (to->pending)
		return FWD_ERR;

	to->pending = YES;

	switch (pos) {

	case FWD_TIMEOUT_SORT:
		{
			dt_queue     *p;
			fwd_timeout *tmp;


			dt_queue_foreach(&evt->timeout, p) {
				tmp = dt_queue_data(p, fwd_timeout, list);

				if (fwd_time_diff(&tmp->time, &to->time) > 0)
					break;
			}

			if (!p)
				p = &evt->timeout;

			dt_queue_insert_tail(p, &to->list);
		}
		break;

	case FWD_TIMEOUT_HEAD:
		dt_queue_insert_head(&evt->timeout, &to->list);
		break;

	default:
		dt_queue_insert_tail(&evt->timeout, &to->list);
		break;
	}

	return FWD_OK;
}

dt_int fwd_timeout_init(fwd_timeout *to)
{
	memset(to, 0, sizeof(fwd_timeout));
	to->pending = NO;
	return FWD_OK;
}

dt_int fwd_timeout_set(fwd_event *evt, 
		       fwd_timeout *to, 
		       dt_int msecs,
		       fwd_timeout_cb cb,
		       dt_uint pos)
{
	struct timeval *t = &to->time;

	if (to->pending)
		fwd_timeout_cancel(to);

	gettimeofday(&to->time, NULL);

	t->tv_sec  += msecs / 1000;
	t->tv_usec += (msecs % 1000) * 1000;

	if (t->tv_usec > 1000000) {
		t->tv_sec++;
		t->tv_usec %= 1000000;
	}
	to->cb = cb;

	return fwd_timeout_add(evt, to, pos);
}

dt_int 
fwd_timeout_cancel(fwd_timeout *to)
{
	if (!to->pending)
		return FWD_ERR;

	dt_queue_remove(&to->list);
	to->pending = NO;

	return FWD_OK;
}

dt_int fwd_event_end(fwd_event *evt)
{
	return evt->run = NO;
}

dt_int fwd_event_fd_init(fwd_event_fd *fd)
{
	memset(fd, 0, sizeof(fwd_event_fd));
	return FWD_OK;
}
