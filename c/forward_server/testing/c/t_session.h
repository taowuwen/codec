
#ifndef TEST_SESSION_HEADER____TESTING____
#define TEST_SESSION_HEADER____TESTING____

t_session *t_session_create();
dt_int    t_session_init();

dt_int    t_session_uninit(t_session *sess);
dt_void   t_session_destroy(t_session *sess);

dt_int    t_session_changestatus(t_session *sess, t_session_status st);
dt_int	  t_session_shutdown(t_session *sess, dt_int detach, dt_int release);


#define session_self(sess)		((sess)->user1)
#define	session_peer(sess)		((sess)->user2)
#define session_peer_set(sess, user) 	(session_peer(sess) = (user))
#define session_self_set(sess, user) 	(session_self(sess) = (user))

#define session_base(sess)		((sess)->base)
#define session_attach(sess, base)	(session_base(sess) = (base))

#define session_detach(sess) do {	\
	dt_queue_remove(&(sess)->list);	\
	session_attach(sess, NULL);	\
}while(0)

#define session_status(sess)		((sess)->st)

#define session_uuid(sess)		(str_text(&(sess)->uuid))
#define session_uuid_set(sess, id)	(str_set(&(sess)->uuid, (id)))


dt_int	t_session_timeout_set(t_session *sess, dt_uint timeout);
dt_int  t_session_timeout_cancel(t_session *sess);

#endif
