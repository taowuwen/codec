
#include "fwd_main.h"
#include "fwd_child.h"
#include "fwd_session.h"
#include "fwd_user.h"
#include "fwd_logger.h"


dt_int fwd_child_init(fwd_child *fc)
{
	dt_assert(fc != NULL);

	memset(fc, 0, sizeof(fwd_child));

	dt_queue_init(&fc->sess);
	dt_queue_init(&fc->wait);
	dt_mtx_init(fc->mtx);

	fc->n_sess = fc->n_wait = 0;
	fc->tid = 0;

	if (fwd_event_init(&fc->evt) != OK) {
		fwd_child_uninit(fc);
		return FWD_ERR;
	}

	fc->st = FWD_CHILD_STOPPED;
	fc->inited = YES;
	return FWD_OK;
}

static dt_int 
fwd_child_handle(fwd_child *fc, fwd_session *sess)
{
	dt_assert(fc && sess);

	fwd_session_attach(sess, fc);
	fwd_session_changestatus(sess, ST_EST);

	fc->n_sess++;
	dt_queue_insert_tail(&fc->sess, &sess->list);

	return FWD_OK;
}

static dt_void 
fwd_child_callback(fwd_event *event)
{
	dt_queue    *p;
	fwd_session *sess;
	fwd_child   *fc = fwd_container_of(event, fwd_child, evt);

	if (fc->st == FWD_CHILD_STOPPED) {
		fwd_event_end(&fc->evt);
		return;
	}

	dt_mtx_lock(fc->mtx);

	if (dt_queue_empty(&fc->wait)) {
		if (dt_queue_empty(&fc->sess)) {
			log_trace("[0x%llx]: all session finished, exit", fc->tid);
			fc->st = FWD_CHILD_STOPPED;

			dt_assert((fc->n_sess == 0) && (fc->n_wait == 0));
			fwd_event_end(&fc->evt);
		}
	} else {
		while (!dt_queue_empty(&fc->wait)) {

			p = dt_queue_head(&fc->wait);
			dt_queue_remove(p);

			sess = dt_queue_data(p, fwd_session, list);
			fwd_child_handle(fc, sess);
		}

		fc->n_wait = 0;
	}

	dt_mtx_unlock(fc->mtx);
}


static dt_int fwd_child_main(fwd_child *fc)
{
	return fwd_event_run(&fc->evt, fwd_child_callback);
}


static dt_void *fwd_child_thread_entry(dt_threadarg arg)
{
	fwd_child *fc = (fwd_child *)arg;

	fc->st = FWD_CHILD_RUN;

	log_info("child thread: 0x%lx started(tid: %ld(0x%lx))", fc->tid, dt_gettid(), dt_gettid());
	fwd_child_main(fc);
	log_info("child thread: 0x%lx stopped(tid: %ld(0x%lx))", fc->tid, dt_gettid(), dt_gettid());
	fc->st = FWD_CHILD_STOPPED;

	return NULL;
}


dt_int fwd_child_start(fwd_child *fc)
{
	if (!fc->inited)
		return FWD_ERR;

	fwd_child_stop(fc);

	return dt_threadcreate(&fc->tid, fwd_child_thread_entry, (dt_threadarg)fc);
}

dt_int fwd_child_stop(fwd_child *fc)
{
	if (!fc->inited)
		return FWD_ERR;

	if (fc->st == FWD_CHILD_EXITED)
		return FWD_OK;

	fc->st = FWD_CHILD_STOPPED;

	if (fc->tid > 0) {
		dt_threadjoin(fc->tid);
		fc->tid = 0;
	}

	return FWD_OK;
}

static void fwd_child_shutdown_session(fwd_child *fc)
{
	fwd_session *sess;
	dt_queue *n;

	while (!dt_queue_empty(&fc->sess)) {
		n = dt_queue_head(&fc->sess);

		sess = dt_queue_data(n, fwd_session, list);
		fwd_session_shutdown(sess);
		fwd_session_changestatus(sess, ST_ERROR);
	}

	dt_queue_clear(&fc->wait, fwd_session, list, fwd_session_shutdown);
}


dt_int fwd_child_uninit(fwd_child *fc)
{
	if (!fc)
		return FWD_ERR;

	log_trace("child: %p exiting ... ", fc);

	fwd_child_stop(fc);

	fc->st = FWD_CHILD_EXITED;
	fc->inited = NO;

	fwd_child_shutdown_session(fc);

	dt_assert(fc->n_wait == 0);
	dt_assert(fc->n_sess == 0);

	fwd_event_done(&fc->evt);

	dt_mtx_destroy(fc->mtx);

	log_trace("child: %p exit normally", fc);

	return FWD_OK;
}


dt_int fwd_child_total_sessions(fwd_child *fc)
{
	dt_assert(fc && fc->n_sess >= 0 && fc->n_wait >= 0);
	return (fc->n_sess + fc->n_wait);
}

dt_int fwd_child_session_attach(fwd_child *fc, fwd_session *sess)
{
	dt_assert(fc && sess);

	dt_queue_remove(&sess->list);

	dt_mtx_lock(fc->mtx);
	dt_queue_insert_tail(&fc->wait, &sess->list);
	fc->n_wait++;
	fwd_session_attach(sess, fc);

	dt_mtx_unlock(fc->mtx);
	
	return FWD_OK;
}

dt_int fwd_child_session_detach(fwd_child *fc, fwd_session *sess)
{
	dt_assert(fc && sess && dt_validblock(sess));

	fc->n_sess--;

	dt_queue_remove(&sess->list);

	return FWD_OK;
}

dt_int fwd_child_timeout_set(fwd_child      *fc,
			     fwd_timeout    *to, 
			     dt_int          msecs, 
			     fwd_timeout_cb  cb,
			     dt_uint         pos)
{
	dt_assert(fc && "call fwd_session_attach first");
	return fwd_timeout_set(&fc->evt, to, msecs, cb, pos);
}

dt_int fwd_child_timeout_cancel(fwd_timeout *to)
{
	return fwd_timeout_cancel(to);
}

dt_int fwd_child_event_set(fwd_child    *fc,
			   fwd_event_fd *fd,
			   dt_uint       flags,
			   fwd_event_cb  cb)
{
	dt_assert(fc && fd);

	if (!(fc && fd))
		return FWD_ERR_INVALID_ARG;

	return fwd_event_add(&fc->evt, fd, flags, cb);
}

dt_int fwd_child_event_cancel(fwd_child *fc, fwd_event_fd *fd)
{
	dt_assert(fc && fd);
	
	if (!(fc && fd))
		return FWD_ERR_INVALID_ARG;

	return fwd_event_del(&fc->evt, fd);
}
