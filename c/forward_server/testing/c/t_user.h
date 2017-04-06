
#ifndef TESTING_FWD_CM_USER_HEADER___
#define TESTING_FWD_CM_USER_HEADER___


t_user  *t_user_create();
dt_void  t_user_destroy(t_user *);
dt_int   t_user_init(t_user *user);
dt_void  t_user_uninit(t_user *user);


#define user_session(user)	((user)->sess)
#define user_attach(user, sess)	(user_session(user) = (sess))
#define user_detach(user)	(user_attach(user, NULL))

#define user_peer(user)			((user)->peer)
#define user_peer_set(user, peer)	(user_peer(user) = (peer))


/*
for both self and peer's opt
*/
dt_int t_user_cm_connect(t_user *user);
dt_int t_user_cm_handle_msg(t_user *user);
dt_int t_user_fwd_change_status(t_user *user, t_fwd_status st);

typedef dt_void (*user_timeout_func_cb)(t_timeout *to);
typedef dt_void (*user_event_func_cb)(t_event_fd *evt, dt_int flgs);
typedef dt_int  (*user_timeout_func)(t_user *user);
typedef dt_int  (*user_event_func)(t_user *user, dt_int flgs);


dt_int t_user_timeout_set(t_user *, t_timeout *to, dt_uint delay, user_timeout_func_cb cb);
dt_int t_user_timeout_cancel(t_timeout *to);

dt_int t_user_event_set(t_user *, t_event_fd *evt, dt_int flgs, user_event_func_cb cb);
dt_int t_user_event_cancel(t_user *, t_event_fd *evt);

#define user_cm_timeout_set(user, to, cb)  t_user_timeout_set(user, &user->cm_to, to, cb)
#define user_cm_timeout_cancel(user)	   t_user_timeout_cancel(&user->cm_to)
#define user_fwd_timeout_set(user, to, cb) t_user_timeout_set(user, &user->fwd_to, to, cb)
#define user_fwd_timeout_cancel(user)	   t_user_timeout_cancel(&user->fwd_to)

#define user_cm_event_set(user, flgs, cb) do {			\
	user->cm_evt.fd = fwd_sock_fd(&user->cm);		\
	t_user_event_set(user, &user->cm_evt, flgs, cb);	\
} while(0)
#define user_cm_event_cancel(user)	  t_user_event_cancel(user, &user->cm_evt)

#define user_fwd_event_set(user, flgs, cb) do {			\
	user->fwd_evt.fd = fwd_sock_fd(&user->fwd);		\
	t_user_event_set(user, &user->fwd_evt, flgs, cb);	\
} while(0)
#define user_fwd_event_cancel(user)	  t_user_event_cancel(user, &user->fwd_evt)

#define user_cm_status(user)		((user)->cm_st)
#define user_fwd_status(user)		((user)->fwd_st)

#define user_ticket(user)		(str_text(&(user)->ticket))
#define user_ticket_set(user, str)	(str_set(&(user)->ticket, str))
#define user_is_self(user)		(user == session_self(user_session(user)))

#define user_retry(user)		((user)->n_retry)
#define user_retry_set(user, n)		((user)->n_retry = (n))

/*
for msgs
*/

#define MSG_CM_P2P_REG		0x0005
#define MSG_CM_FWD_REQUEST	0x1502
#define MSG_FWD_REQUEST		0x1504


/*
for msgs self
*/

#define T_MSG_BASE		0xaabb

#define T_MSG_HELLO		(T_MSG_BASE + 0x01)
#define T_MSG_FILE		(T_MSG_BASE + 0x02)
#define T_MSG_EXIT		(T_MSG_BASE + 0x03)


#endif
