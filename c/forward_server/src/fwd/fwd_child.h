
#ifndef FWD_SERVER_CHILD_HEADER___
#define FWD_SERVER_CHILD_HEADER___

dt_int fwd_child_init(fwd_child *);
dt_int fwd_child_start(fwd_child *);
dt_int fwd_child_uninit(fwd_child *);
dt_int fwd_child_stop(fwd_child *);

dt_int fwd_child_total_sessions(fwd_child *);
dt_int fwd_child_session_attach(fwd_child *, fwd_session *);
dt_int fwd_child_session_detach(fwd_child *, fwd_session *);

dt_int fwd_child_timeout_set(fwd_child      *ch,
			     fwd_timeout    *to, 
			     dt_int          msecs, 
			     fwd_timeout_cb  cb,
			     dt_uint         pos);

dt_int fwd_child_timeout_cancel(fwd_timeout *to);


dt_int fwd_child_event_set(fwd_child    *fc,
			   fwd_event_fd *fd,
			   dt_uint       flags,
			   fwd_event_cb  cb);

dt_int fwd_child_event_cancel(fwd_child *fc, fwd_event_fd *fd);
				


#endif
