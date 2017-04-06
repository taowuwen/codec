
#ifndef EMS_EVENT_HANDLER__HEADER___AAAA
#define EMS_EVENT_HANDLER__HEADER___AAAA

#include "ems_common.h"
#include "ems_timer.h"
#include "ems_buffer.h"


#define EMS_EVT_READ		(1 << 0)
#define EMS_EVT_WRITE		(1 << 1)
#define EMS_EVT_EDGE_TRIGGER	(1 << 2)
#define EMS_EVT_BLOCKING	(1 << 3)

#define EVT_DRIVER_EPOLL	(1 << 0)
#define EVT_DRIVER_SELECT	(1 << 1)


typedef struct _ems_event_fd_s  ems_event_fd;
typedef struct _ems_event_s	ems_event;

typedef ems_void (*ems_event_cb)(ems_event_fd *evt, ems_int events);
typedef ems_void (*ems_event_handle_extra_cb)(ems_event *evt);

typedef struct _driver_event_s ems_driver_event;

struct _driver_event_s {
	ems_uint  id;
	ems_str   desc;
	ems_void *ctx;

	ems_int (*init)(ems_driver_event *drv);
	ems_int (*uninit)(ems_driver_event *drv);
	ems_int (*handle)(ems_driver_event *drv, ems_int timeout);
	ems_int (*subscribe)(ems_driver_event *drv, ems_event_fd *fd, ems_uint flg);
	ems_int (*unsubscribe)(ems_driver_event *drv, ems_event_fd *fd);
};

struct _ems_event_fd_s
{
	ems_int        fd;
	ems_int        eof;
	ems_int        reg;
	ems_uint       flg;
	ems_int        error;
	ems_event_cb  cb;
};

struct  _ems_event_s
{
	ems_queue	    timeout;
	ems_driver_event   *drv;
	ems_int             run;
};

ems_int ems_event_init(ems_event *evt, ems_uint evtid);
ems_int ems_event_done(ems_event *evt);
ems_int ems_event_run(ems_event *evt, ems_event_handle_extra_cb extra);
ems_int ems_event_end(ems_event *evt);

ems_int ems_event_add(ems_event *evt, 
		     ems_event_fd *fd, 
		     ems_uint flags,
		     ems_event_cb cb);
ems_int ems_event_del(ems_event *evt, ems_event_fd *fd);

ems_int ems_event_fd_init(ems_event_fd *fd);


#endif
