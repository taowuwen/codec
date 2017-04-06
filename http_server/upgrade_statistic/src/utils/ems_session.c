
#include "ems_main.h"
#include "ems_session.h"

/*
 * session's area
 * */

ems_int ems_session_init(ems_session *sess)
{
	ems_assert(sess && "never show up this line");

	memset(sess, 0, sizeof(ems_session));

	ems_sock_init(&sess->sock);
	sess->obj  = NULL;
	sess->rsrv = NULL;

	ems_buffer_init(&sess->buf_in,  EMS_BUFFER_1K);
	ems_buffer_init(&sess->buf_out, EMS_BUFFER_1K);

	ems_timeout_init(&sess->to);
	ems_event_fd_init(&sess->evt);

	ems_queue_init(&sess->entry);

	sess->evt_cb = NULL;
	sess->timeout_cb = NULL;
	sess->cbarg  = NULL;

	return EMS_OK;
}

ems_void ems_session_uninit(ems_session *sess)
{
	ems_assert(sess && "never show up this line");
	
	ems_sock_clear(&sess->sock);
	sess->rsrv = NULL;

	ems_buffer_uninit(&sess->buf_in);
	ems_buffer_uninit(&sess->buf_out);

#ifdef DEBUG
	ems_assert(!sess->to.pending && !sess->evt.reg);

	if (sess->to.pending)
		ems_l_warn("session's timer did not cancel... seg fault");

	if (sess->evt.reg)
		ems_l_warn("session's event did not cancel... seg fault");
#endif
	sess->evt_cb = NULL;
	sess->timeout_cb = NULL;
	sess->cbarg  = NULL;
}

ems_session *ems_session_new()
{
	ems_session *sess = NULL;

	sess = (ems_session *)ems_malloc(sizeof(ems_session));
	if (sess)
		ems_session_init(sess);

	return sess;
}


ems_void ems_session_destroy(ems_session *sess)
{
	ems_session_uninit(sess);
	ems_free(sess);
}

ems_int ems_session_shutdown_and_destroy(ems_session *sess)
{
	ems_assert(sess);

	ems_l_trace("session(%d) [%s: %d] down", 
			ems_sock_fd(&sess->sock),
			ems_sock_addr(&sess->sock),
			ems_sock_port(&sess->sock));

	sess_event_cancel(sess);
	sess_timeout_cancel(sess);
	ems_sock_close(&sess->sock);

	ems_session_destroy(sess);
	return EMS_OK;
}

ems_void sess_event(ems_event_fd *event, ems_int flg)
{
	ems_session *sess = ems_container_of(event, ems_session, evt);
	ems_assert(sess->evt_cb);
	sess->evt_cb(sess, event->error, flg);
}

ems_void sess_timeout(ems_timeout *timeout)
{
	ems_session *sess = ems_container_of(timeout, ems_session, to);
	ems_assert(sess->timeout_cb);
	sess->timeout_cb(sess, timeout);
}

