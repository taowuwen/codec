
#ifndef FWD_LISTENNER_STARTED____
#define FWD_LISTENNER_STARTED____


dt_int fwd_bind_init(fwd_bind *);
dt_int fwd_bind_uninit(fwd_bind *);
dt_int fwd_bind_start(fwd_bind *);
dt_int fwd_bind_stop(fwd_bind *);
dt_int fwd_bind_setevent(fwd_bind *);

#endif
