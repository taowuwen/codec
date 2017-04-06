
#include "ems_event.h"
#include "ems_main.h"
#include "evt_epoll.h"


static ems_driver_event *evt_drvs[] = 
{
	&evt_driver_epoll,
	NULL
};

static ems_driver_event *ems_event_load_driver(ems_uint evtid)
{
	ems_driver_event *drv;

	for (drv = evt_drvs[0]; drv != NULL; drv++) {
		if (drv->id == evtid)
			return drv;
	}

	return NULL;
}

ems_int ems_event_init(ems_event *evt, ems_uint evtid)
{
	ems_driver_event *drv;
	ems_assert(evt != NULL);

	memset(evt, 0, sizeof(ems_event));

	drv = ems_event_load_driver(evtid);
	if (!drv) 
		return EMS_ERR;

	ems_assert(drv->init && drv->uninit && drv->handle && drv->subscribe && drv->unsubscribe);

	ems_l_trace("event driver: %x: %s", drv->id, str_text(&drv->desc));

	if (drv->init(drv)) {
		ems_l_trace("event: %s init failed", str_text(&drv->desc));
		return EMS_ERR;
	}

	ems_queue_init(&evt->timeout);
	evt->run = YES;
	evt->drv = drv;

	return EMS_OK;
}

ems_int ems_event_done(ems_event *evt)
{
	ems_driver_event *drv;

	evt->run = NO;
	drv      = evt->drv;

	if (drv && drv->uninit) {
		drv->uninit(drv);
		evt->drv = NULL;
	}

	return EMS_OK;
}

ems_int ems_event_run(ems_event *evt, ems_event_handle_extra_cb proc)
{
	ems_driver_event *drv;
	struct timeval    tv;

	evt->run = YES;
	drv = evt->drv;

	ems_assert(drv && drv->handle && "never show up this line");

	while (evt->run) {

		if (proc) { 
			proc(evt); 
			if (!evt->run) 
				break;
		}

		ems_getboottime(&tv);
		ems_timeout_handle(&evt->timeout, &tv);

		if (evt->run)
			drv->handle(drv, ems_timeout_next(&evt->timeout, &tv));
	}

	return EMS_OK;
}


ems_int ems_event_add(ems_event *evt, 
		     ems_event_fd *fd, 
		     ems_uint flgs,
		     ems_event_cb cb)
{
	ems_uint fl;
	ems_driver_event *drv = evt->drv;

	if (!fd->reg && !(flgs & EMS_EVT_BLOCKING)) {
		fl  = fcntl(fd->fd, F_GETFL, 0);
		fl |= O_NONBLOCK;
		fcntl(fd->fd, F_SETFL, fl);
	}

	if (fd->reg) {
		if (fd->flg == flgs)
			return EMS_OK;
	}

	fd->cb = cb;
	if (drv->subscribe(drv, fd, flgs))
		return EMS_ERR;

	fd->reg = YES;
	fd->eof = NO;
	fd->flg = flgs;
	fd->error = NO;

	return EMS_OK;
}


ems_int ems_event_del(ems_event *evt, ems_event_fd *fd)
{
	ems_assert(evt && fd && "never show up this line");

	return evt->drv->unsubscribe(evt->drv, fd);
}

ems_int ems_event_end(ems_event *evt)
{
	return evt->run = NO;
}

ems_int ems_event_fd_init(ems_event_fd *fd)
{
	memset(fd, 0, sizeof(ems_event_fd));
	return EMS_OK;
}
