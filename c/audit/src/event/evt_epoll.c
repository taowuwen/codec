
#include "ems_common.h"
#include "evt_epoll.h"

#include <sys/epoll.h>
#include <sys/wait.h>


#ifndef EPOLLRDHUP
#define EPOLLRDHUP	0x2000
#endif

#define EMS_EVT_SIZE		1024


typedef struct evt_epoll_s evt_epoll;


struct evt_epoll_s {
	ems_int             fd;
	ems_int             cur_fd;
	ems_int             cur_nfds;
	ems_int             n_evt;
	struct epoll_event *evt;
};

static ems_int evt_del(evt_epoll *evt, ems_event_fd *fd)
{
	ems_int i;

	ems_assert(evt && fd);

	if (!fd->reg)
		return EMS_OK;

	for (i = evt->cur_fd + 1; i < evt->cur_nfds; i++) {
		if (evt->evt[i].data.ptr != fd)
			continue;

		evt->evt[i].data.ptr = NULL;
	}

	fd->reg = NO;
	fd->flg = 0;

	return epoll_ctl(evt->fd, EPOLL_CTL_DEL, fd->fd, NULL);
}


static ems_int evt_init(ems_driver_event *drv)
{
	evt_epoll *evt = NULL;
	ems_assert(drv != NULL);

	evt = (evt_epoll *)ems_malloc(sizeof(evt_epoll));
	if (!evt)
		return EMS_ERR;

	evt->fd = epoll_create1(EPOLL_CLOEXEC);
	if (evt->fd <= 0) {
		ems_free(evt);
		return EMS_ERR;
	}

	evt->n_evt = EMS_EVT_SIZE;

	evt->evt = (struct epoll_event *)
			ems_malloc(sizeof(struct epoll_event) * evt->n_evt);
	if (!evt->evt) {
		close(evt->fd);
		ems_free(evt);
		return EMS_ERR;
	}

	evt->cur_fd   = 0;
	evt->cur_nfds = 0;
	drv->ctx      = (ems_void *)evt;

	return EMS_OK;
}

static ems_int evt_uninit(ems_driver_event *drv)
{
	evt_epoll *evt = (evt_epoll *)drv->ctx;

	evt->cur_fd   = 0;
	evt->cur_nfds = 0;

	if (evt->fd > 0) {
		close(evt->fd);
		evt->fd = 0;
	}

	if (evt->evt) {
		ems_free(evt->evt);
		evt->evt = NULL;
	}

	ems_free(evt);
	drv->ctx = NULL;

	return EMS_OK;
}

static ems_int evt_handle(ems_driver_event *drv, ems_int timeout)
{
	ems_uint     flg, events;
	ems_event_fd *fd = NULL;
	evt_epoll *evt = (evt_epoll *)drv->ctx;

	ems_assert(drv && drv->ctx && "never show up this line");


	evt->cur_nfds = epoll_wait(evt->fd, evt->evt, evt->n_evt, timeout);

	for (evt->cur_fd = 0; evt->cur_fd < evt->cur_nfds; evt->cur_fd++) {

		flg    = 0;
		fd     = evt->evt[evt->cur_fd].data.ptr;
		events = evt->evt[evt->cur_fd].events;

		if (!fd)
			continue;

		if (events & (EPOLLERR | EPOLLHUP)) {
			fd->error = YES;
			evt_del(evt, fd);
		}

		if (!(events & (EPOLLRDHUP | EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP)))
			continue;

		if (events & EPOLLRDHUP)
			fd->eof = YES;

		if (events & EPOLLIN)
			flg |= EMS_EVT_READ;

		if (events & EPOLLOUT)
			flg |= EMS_EVT_WRITE;

		if (fd->cb)
			fd->cb(fd, flg);
	}

	return EMS_OK;
}


static ems_int 
evt_subscribe(ems_driver_event *drv, ems_event_fd *fd, ems_uint flg)
{
	evt_epoll *evt;
	struct epoll_event ev;
	int op = fd->reg ? EPOLL_CTL_MOD: EPOLL_CTL_ADD;

	evt = (evt_epoll *)drv->ctx;

	memset(&ev, 0, sizeof(struct epoll_event));

	if (flg & EMS_EVT_READ)
		ev.events |= (EPOLLIN | EPOLLRDHUP);

	if (flg & EMS_EVT_WRITE)
		ev.events |= EPOLLOUT;

	if (flg & EMS_EVT_EDGE_TRIGGER)
		ev.events |= EPOLLET;

	ev.data.fd  = fd->fd;
	ev.data.ptr = fd;

	return epoll_ctl(evt->fd, op, fd->fd, &ev);
}

static ems_int 
evt_unsubscribe(ems_driver_event *drv, ems_event_fd *fd)
{
	evt_epoll *evt;

	ems_assert(drv && drv->ctx && "never show up this line");
	evt = drv->ctx;

	return evt_del(evt, fd);
}


ems_driver_event evt_driver_epoll = {
	.id   = EVT_DRIVER_EPOLL,
	.desc = ems_string("epoll"),
	.ctx  = NULL,

	.init   = evt_init,
	.uninit = evt_uninit,
	.handle = evt_handle,
	.subscribe = evt_subscribe,
	.unsubscribe = evt_unsubscribe
};
