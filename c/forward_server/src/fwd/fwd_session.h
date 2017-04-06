
#ifndef FWD_SESSION_HEADER____
#define FWD_SESSION_HEADER____


dt_void      fwd_session_destroy(fwd_session *);
fwd_session *fwd_session_create(dt_cchar *ticket);

dt_int  fwd_session_init(fwd_session *, dt_cchar *ticket);
dt_void fwd_session_uninit(fwd_session *);

dt_int  fwd_session_attach(fwd_session *, fwd_child *);
dt_int  fwd_session_detach(fwd_session *);

dt_int fwd_session_changestatus(fwd_session *, fwd_status);
dt_int fwd_session_status(fwd_session *);

dt_int fwd_session_timeout_set(fwd_session *sess, dt_uint to);
dt_int fwd_session_timeout_cancel(fwd_session *sess);

#define fwd_session_user1(sess)	((sess)->user1)
#define fwd_session_user2(sess)	((sess)->user2)

#define fwd_session_user1_set(sess, user)  (sess->user1 = user)
#define fwd_session_user2_set(sess, user)  (sess->user2 = user)

dt_int fwd_session_shutdown(fwd_session *sess);

fwd_user *fwd_session_next_user(fwd_session *sess, fwd_user *);

dt_void fwd_session_print(fwd_session *, fwd_user *, fwd_transinfo *);


#endif
