
#include "t_main.h"
#include "t_worker.h"
#include "t_session.h"


static dt_void t_worker_shutdown_all(t_worker *worker)
{
	dt_queue  *p;

	while (!dt_queue_empty(&worker->sess)) {
		p = dt_queue_head(&worker->sess);
		t_session_destroy(dt_container_of(p, t_session, list));
	}
}

static dt_int worker_spawn_one(t_worker *worker)
{
	t_session *sess;

	sess = t_session_create();

	dt_assert(sess && "sure about memory error???");
	if (sess) {
		worker->left++;
		dt_queue_insert_tail(&worker->sess, &sess->list);
		session_attach(sess, worker);
		t_session_changestatus(sess, ST_SESSION_INIT);
	}
	return T_OK;
}

static dt_int worker_start_sessions(t_worker *worker)
{
	dt_int 	   n;

	for (n = 0; n < worker->n_sess; n++) {
		worker_spawn_one(worker);

		if (!t_test(T_FLG_ALL_CONNECT_THE_SAME_TIME)) {
			worker->n_sess--;
			return T_OK;
		}
	}

	return T_OK;
}

static dt_void worker_cb(fwd_event *evt)
{
	t_worker *worker = dt_container_of(evt, t_worker, event);

	if (!worker->run)
		fwd_event_end(evt);

	if (!t_test(T_FLG_ALL_CONNECT_THE_SAME_TIME)) {
		if (worker->n_sess > 0) {
			worker_spawn_one(worker);
			worker->n_sess--;
		}
	}

	if (dt_queue_empty(&worker->sess)) {
		log_info("no more user on child: 0x%llx", worker->tid);
		fwd_event_end(evt);
	}
}


static dt_int t_worker_main(t_worker *worker)
{
	worker_start_sessions(worker);

	worker->run = YES;
	worker->st  = FWD_CHILD_RUN;

	while (worker->run && !dt_queue_empty(&worker->sess)) 
	{
		fwd_event_run(&worker->event, worker_cb);
	}


	return T_OK;
}


static dt_void *worker_thread(dt_threadarg arg)
{
	t_worker *worker = (t_worker *)arg;

	log_info("child: %llx started", worker->tid);
	t_worker_main(worker);
	log_info("child: %llx stopped", worker->tid);

	return NULL;
}


dt_int  t_worker_init(t_worker *worker)
{
	dt_assert(worker && "invalid arg");

	memset(worker, 0, sizeof(t_worker));

	worker->st = FWD_CHILD_STOPPED;
	worker->run = NO;

	worker->left  = 0;
	worker->n_sess = 0;
	dt_queue_init(&worker->sess);

	worker->tid = 0;
	worker->spawn = NO;

	fwd_event_init(&worker->event);

	return T_OK;
}

dt_void t_worker_uninit(t_worker *worker)
{
	dt_assert(worker);

	if (!worker)
		return;

/*
 * just wait all work thread stopped, do not kill them
 * just let them exit by itself.
 * */
	if (worker->spawn && worker->tid > 0) {
		log_info("---->>> wait for child %llx stopped", worker->tid);
		dt_threadjoin(worker->tid);
		worker->tid = 0;
	}

	t_worker_stop(worker);

	t_worker_shutdown_all(worker);

	fwd_event_done(&worker->event);
}


dt_int  t_worker_start(t_worker *worker, dt_int spawn, dt_int sess)
{
	log_trace("worker start: %p, spawn? %s, session to handle: %d",
				worker, spawn?"yes":"no", sess);

	worker->n_sess = sess;
	worker->spawn  = spawn;
	if (spawn) {
		if (dt_threadcreate(&worker->tid, worker_thread, worker)) {
			log_err("create thread failed: %s", t_lasterrstr());
			return T_ERR;
		}
	} else {
		worker->tid = dt_gettid();
		log_info("master: 0x%llx started", dt_gettid());
		t_worker_main(worker);
		log_info("master: 0x%llx stopped", dt_gettid());
	}

	return T_OK;
}

dt_int  t_worker_stop(t_worker *worker)
{
	dt_assert(worker);

	if (worker->st != FWD_CHILD_RUN)
		return T_OK;


	worker->st  = FWD_CHILD_STOPPED;
	worker->run = NO;

	if (worker->spawn && worker->tid > 0)
		dt_threadjoin(worker->tid);

	worker->tid   = 0;
	worker->spawn = NO;

	return T_OK;
}
