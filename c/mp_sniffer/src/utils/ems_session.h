
#ifndef EMS_SESSION_HEADER___
#define EMS_SESSION_HEADER___


typedef struct _ems_session_s   ems_session;

typedef ems_void (*sess_evt_cb)(ems_session *sess, ems_int st, ems_int flgs);
typedef ems_void (*sess_timeout_cb)(ems_session *sess, ems_timeout *to);


struct _ems_session_s
{
	ems_sock       sock;
	ems_void      *obj; /* ac, web, sync */
	ems_void      *rsrv;
	ems_uint       flg;

	ems_buffer     buf_in;
	ems_buffer     buf_out;
#define buf	buf_out

	ems_timeout     to;
	ems_event_fd    evt;

	ems_queue       entry;

	struct {
		sess_evt_cb     evt_cb;
		sess_timeout_cb timeout_cb;
		ems_uchar *cbarg;
	};
};



/*
 * session's control
 * */
ems_int      ems_session_init(ems_session *sess);
ems_void     ems_session_uninit(ems_session *sess);
ems_session *ems_session_new();
ems_void     ems_session_destroy(ems_session *sess);
ems_int      ems_session_shutdown_and_destroy(ems_session *sess);

ems_void sess_event(ems_event_fd *event, ems_int flg);
ems_void sess_timeout(ems_timeout *timeout);

#define sess_event_set(sess, flg, cb) do {\
	(sess)->evt_cb = cb;      \
	(sess)->evt.fd = ems_sock_fd(&(sess)->sock); \
	ems_event_add(eventer(), &(sess)->evt, flg|EMS_EVT_EDGE_TRIGGER, sess_event); \
}while(0)

#define sess_event_cancel(sess) \
	ems_event_del(eventer(), &(sess)->evt)

#define sess_timeout_set_ex(sess, msecs, cb, pos) \
do {\
	(sess)->timeout_cb = cb;  \
	ems_timeout_set(timeouter(), &(sess)->to, msecs, sess_timeout, pos); \
} while (0)

#define sess_timeout_set(sess, msecs, cb) \
		sess_timeout_set_ex((sess), msecs, cb, EMS_TIMEOUT_TAIL)

#define sess_timeout_set_sorted(sess, msecs, cb) \
		sess_timeout_set_ex((sess), msecs, cb, EMS_TIMEOUT_SORT)

#define sess_timeout_cancel(sess)	ems_timeout_cancel(&(sess)->to)

#define sess_cbarg_set(sess, arg) ((sess)->cbarg = (ems_uchar *)(arg))
#define sess_cbarg(sess)	  ((sess)->cbarg)


#define sess_recv(sess, buf) ems_sock_read(&(sess)->sock, buf)
#define sess_send(sess, buf) ems_sock_send(&(sess)->sock, buf)

#define sess_peer(sess)	     		(ems_session *)((sess)->rsrv)
#define sess_peer_set(self, sess)	((self)->rsrv=(ems_void *)(sess))

#define sess_parent(sess, type)		(type *)((sess)->obj)
#define sess_parent_set(sess, parent)	((sess)->obj = (ems_void *)(parent))

#define SESSION_FLAG_DIE_AFTER_SEND	(0x01 << 30)

#define sess_response_set(sess, rsp)	((sess)->rsrv = (ems_void *)(rsp))
#define sess_response(sess)		((sess)->rsrv)

#define sess_request_set		sess_response_set
#define sess_request			sess_response

#endif
