

#ifndef FWD_USER_HEADER_FOR_USERS___
#define FWD_USER_HEADER_FOR_USERS___


fwd_user *fwd_user_create();
dt_void   fwd_user_destroy(fwd_user *);

dt_int    fwd_user_init(fwd_user *);
dt_int    fwd_user_uninit(fwd_user *);

dt_int    fwd_user_attach(fwd_session *, fwd_user *);
dt_int    fwd_user_detach(fwd_user *);

dt_int    fwd_user_changestatus(fwd_user *, fwd_status);

dt_int  fwd_user_event_set(fwd_user *, dt_int flgs);
dt_int  fwd_user_event_cancel(fwd_user *);

dt_int  fwd_user_timeout_set(fwd_user *, dt_int timeout);
dt_int  fwd_user_timeout_cancel(fwd_user *);

#define fwd_user_status(user)  ((user)->st)
#define fwd_user_session(user) ((user)->sess)
#define fwd_user_peer(user)    ((user)->peer)

#endif
