
#ifndef FWD_EVENT_HEADER___
#define FWD_EVENT_HEADER___

#include <sys/epoll.h>
#include <sys/wait.h>

#define FWD_EVT_READ		(1 << 0)
#define FWD_EVT_WRITE		(1 << 1)
#define FWD_EVT_EDGE_TRIGGER	(1 << 2)
#define FWD_EVT_BLOCKING	(1 << 3)

#define FWD_EVT_SIZE		20

typedef struct _fwd_event_fd_s  fwd_event_fd;
typedef struct _fwd_timeout_s   fwd_timeout;
typedef struct _fwd_event_s	fwd_event;

typedef dt_void (*fwd_event_cb)(fwd_event_fd *evt, dt_int events);
typedef dt_void (*fwd_timeout_cb)(fwd_timeout *to);
typedef dt_void (*fwd_event_handle_extra_cb)(fwd_event *evt);

struct _fwd_event_fd_s
{
	dt_int        fd;
	dt_int        eof;
	dt_int        reg;
	dt_uint       flags;
	dt_int        error;
	fwd_event_cb  cb;
};

struct _fwd_timeout_s
{
	dt_queue        list;
	dt_int          pending;
	fwd_timeout_cb  cb;
	struct timeval  time;
};


struct  _fwd_event_s
{
	dt_queue	    timeout;
	dt_int              fd;
	dt_int              run;
	dt_int              cur_fd;
	dt_int              cur_nfds;
	dt_int              n_evt;
	struct epoll_event *evt;
};


dt_int fwd_event_init(fwd_event *evt);
dt_int fwd_event_done(fwd_event *evt);
dt_int fwd_event_run(fwd_event *evt, fwd_event_handle_extra_cb extra);
dt_int fwd_event_end(fwd_event *evt);

dt_int fwd_event_add(fwd_event *evt, 
		     fwd_event_fd *fd, 
		     dt_uint flags,
		     fwd_event_cb cb);
dt_int fwd_event_del(fwd_event *evt, fwd_event_fd *fd);

dt_int fwd_event_fd_init(fwd_event_fd *fd);



/*
	
*/

#define FWD_TIMEOUT_TAIL	0
#define FWD_TIMEOUT_SORT	1
#define FWD_TIMEOUT_HEAD	2

dt_int fwd_timeout_init(fwd_timeout *to);

dt_int fwd_timeout_set(fwd_event *evt, 
		       fwd_timeout *to, 
		       dt_int msecs,
		       fwd_timeout_cb cb,
		       dt_uint pos);

dt_int fwd_timeout_cancel(fwd_timeout *to);


#endif
