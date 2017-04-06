
#include "fwd.h"
#include "fwd_main.h"
#include "fwd_cm.h"
#include "fwd_bind.h"
#include "fwd_sock.h"
#include "fwd_child.h"
#include "fwd_session.h"
#include "fwd_user.h"
#include "fwd_logger.h"

#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/resource.h>

typedef struct {
	dt_int		 cpucore;
	dt_int           pagesize;

	dt_int		 n_child;
	fwd_child       *child;

	fwd_hash         hash; // for session ticket
	dt_queue         user; // for tmp users

	dt_queue         sess;// for tmp sessions come from EST
	dt_mtx		 mtx; 

	fwd_bind         bind;
	fwd_cm           cm;
	fwd_event        evt;
} fwd_main;


/*
for global var
*/

static jmp_buf	jmpaddr;
static fwd_main *_gfm = NULL;

static dt_void 
fwd_sighandler(dt_int sig)
{
	longjmp(jmpaddr,1);
}


static dt_int
fwd_main_init(fwd_main *fm)
{
	dt_int i;
	memset(fm, 0, sizeof(fwd_main));

	fm->cpucore  = fwd_cpucore();
	fm->pagesize = fwd_pagesize();

	fm->n_child = fm->cpucore;
	if (fm->n_child <= 0)
		fm->n_child = 1;

	fm->child = (fwd_child *)
		    dt_malloc(sizeof(fwd_child) * fm->n_child);

	if (!fm->child)
		return FWD_ERR;

	memset(fm->child, 0, sizeof(fwd_child) * fm->n_child);

	for (i = 0; i < fm->n_child; i++)
		fwd_child_init(&fm->child[i]);

	if (fwd_hash_init(&fm->hash, FWD_HASH_BASE_SIZE) != FWD_OK) {
		dt_free(fm->child);
		fm->child = NULL;
		return FWD_ERR;
	}

	dt_queue_init(&fm->user);
	dt_queue_init(&fm->sess);
	dt_mtx_init(fm->mtx);


	if (fwd_bind_init(&fm->bind) != FWD_OK) {
		exit(-1);
	}

	fwd_cm_init(&fm->cm);

	if (fwd_event_init(&fm->evt) != FWD_OK) {
		dt_free(fm->child);
		fm->child = NULL;
		fwd_hash_uninit(&fm->hash);
		return FWD_ERR;
	}

	return FWD_OK;
}

static dt_int 
fwd_main_clear_hash(fwd_hash_fd *fd, dt_void *args)
{
	fwd_session *sess = dt_container_of(fd, fwd_session, hash);

	fwd_session_destroy(sess);
	sess = NULL;

	return FWD_OK;
}

static dt_int
fwd_main_uninit(fwd_main *fm)
{
	dt_int i;

	fwd_event_end(&fm->evt);
	fwd_bind_uninit(&fm->bind);
	fwd_cm_uninit(&fm->cm);

	if (fm->child) {
		for (i = 0; i < fm->n_child; i++)
			fwd_child_uninit(&fm->child[i]);
		
		dt_free(fm->child);
		fm->child = NULL;
	}


	dt_queue_clean(&fm->sess);
	dt_queue_clean(&fm->user);

	fwd_hash_walk(&fm->hash, fwd_main_clear_hash, NULL);
	fwd_hash_uninit(&fm->hash);

	fwd_event_done(&fm->evt);
	dt_mtx_destroy(fm->mtx);

	return FWD_ERR;
}

/*
1. change user's status into accepted
2. append user onto timeout list
3. set user event, read only
4. done
*/
dt_int fwd_main_user_in(fwd_user *user)
{
	dt_assert(user != NULL);

	fwd_user_changestatus(user, ST_ACCEPTED);

	return OK;
}

dt_int fwd_main_ticket_in(dt_cchar *ticket)
{
	dt_int       ret;
	fwd_session *sess;

	sess = fwd_session_create(ticket);
	if (!sess)
		return FWD_ERR;

	fwd_logger_info(sess, NULL, "new ticket pair created");

	ret = fwd_hash_insert(&_gfm->hash, &sess->hash);

	if (ret == FWD_OK)
		fwd_session_changestatus(sess, ST_INITED);
	else
		fwd_session_destroy(sess);

	return ret;
}

dt_int fwd_main_session_fired(fwd_session *sess)
{
	fwd_cm_ticket_fired(&_gfm->cm, str_text(&sess->ticket));
	fwd_session_destroy(sess);

	return FWD_OK;
}

dt_int fwd_main_session_down(fwd_session *sess)
{
	dt_mtx_lock(_gfm->mtx);

	dt_queue_insert_tail(&_gfm->sess, &sess->list);

	dt_mtx_unlock(_gfm->mtx);

	return FWD_OK;
}

fwd_session *fwd_main_session_get(dt_cchar *ticket)
{
	fwd_session *sess;
	fwd_hash_fd *fd = fwd_hash_find(&_gfm->hash, ticket);

	sess = NULL;
	if (fd)
		sess = dt_container_of(fd, fwd_session, hash);

	return sess;
}

fwd_child *main_choose_child(fwd_main *fm)
{
	dt_int     n_sess, i, id, total;
	fwd_child *fc;

	dt_assert(fm && fm->child);

	fc = &fm->child[0];
	n_sess = fwd_child_total_sessions(fc);
	id     = 0;

	for (i = 1; i < fm->n_child; i++) {
		fc = &fm->child[i];

		total = fwd_child_total_sessions(fc);

		if (total < n_sess) {
			n_sess = total;
			id = i;
		}
	}

	dt_assert(id >= 0 && id < fm->n_child);

	if (id >= 0 && id < fm->n_child)
		return &fm->child[id];

	return NULL;
}

dt_int fwd_main_session_release_ctrl(fwd_session *sess)
{
	fwd_child *fc;

	/* dispacth control to child */

	fc = main_choose_child(_gfm);
	if (!fc)
		return FWD_ERR;

	fwd_child_session_attach(fc, sess);

	dt_assert(fc->st != FWD_CHILD_EXITED && "bug");

	if (fc->st == FWD_CHILD_STOPPED)
		return fwd_child_start(fc);

	return FWD_OK;
}

dt_int fwd_main_event_set(fwd_event_fd *fd, 
                          dt_uint flags,
			  fwd_event_cb cb)
{
	dt_assert(_gfm != NULL && "bug");

	return fwd_event_add(&_gfm->evt,fd, flags, cb);
}

dt_int fwd_main_event_cancel(fwd_event_fd *fd)
{
	return fwd_event_del(&_gfm->evt, fd);
}


dt_int fwd_main_timeout_set(fwd_timeout   *to,
			    dt_int         msecs,
			    fwd_timeout_cb cb,
			    dt_uint        pos)
{
	return fwd_timeout_set(&_gfm->evt, to, msecs, cb, pos);
}

dt_int fwd_main_timeout_cancel(fwd_timeout *to)
{
	return fwd_timeout_cancel(to);
}

dt_cchar *fwd_main_bind_addr()
{
	return fwd_sock_addr(&_gfm->bind.sock);
}

dt_int  fwd_main_bind_port()
{
	return fwd_sock_port(&_gfm->bind.sock);
}

static dt_int
fwd_main_getargs(fwd_main *fm, dt_int argc, dt_char **argv)
{
	dt_int c;

	if (argc < 5) {
		log_err("usage: %s -s server -p port\n", argv[0]);
		return FWD_ERR;
	}

	while ((c = getopt(argc, argv, "s:p:")) != -1) {

		switch(c) {

		case 's':
			fwd_sock_setaddr(&fm->cm.sock, optarg);
			break;
		case 'p':
			fwd_sock_setport(&fm->cm.sock, atoi(optarg));
			break;
		default:
			log_err("usage: %s -s server -p port\n", argv[0]);
			return FWD_ERR;
		}
	}

	if (fwd_strlen(fwd_sock_addr(&fm->cm.sock)) <= 0 || fwd_sock_port(&fm->cm.sock)<= 0) {
		log_err("usage: %s -s server -p port\n", argv[0]);
		return FWD_ERR;
	}

	return FWD_OK;
}

static dt_void main_event_cb(fwd_event *event)
{
	dt_queue    *p;
	fwd_session *sess;
	fwd_main    *fm;

	fm = dt_container_of(event, fwd_main, evt);

	dt_mtx_lock(fm->mtx);

	while (!dt_queue_empty(&fm->sess)) {

		p = dt_queue_head(&fm->sess);
		dt_queue_remove(p);

		sess = dt_queue_data(p, fwd_session, list);
		fwd_session_changestatus(sess, ST_INITED);
	}

	dt_mtx_unlock(fm->mtx);
}

dt_int fwd_main_entry(fwd_main *fm, dt_int argc, dt_char **argv)
{
	if (fwd_main_init(fm) != FWD_OK)
		return FWD_ERR;

	do {
		if (fwd_main_getargs(fm, argc, argv) != OK)
			break;

		log_trace("[main]bind start...");
		if (fwd_bind_start(&fm->bind) != FWD_OK)
			break;

		log_trace("[main]start cm ...");
		if (fwd_cm_start(&fm->cm) != FWD_OK)
			break;

		fwd_bind_setevent(&fm->bind);

		if (!setjmp(jmpaddr))
			fwd_event_run(&fm->evt, main_event_cb);
		else {
			fwd_event_end(&fm->evt);
			log_err("got signal, do exit\n");
		}
	} while (0);

	fwd_main_uninit(fm);

	return FWD_OK;
}


static dt_int fwd_reset_rlimit()
{
	struct rlimit rl, old; 

	if (getrlimit(RLIMIT_CORE, &rl) == 0) {
		log_trace("getrlimit RLIMIT_CORE (cur: %ld, max: %ld)", rl.rlim_cur, rl.rlim_max);
		rl.rlim_cur = rl.rlim_max;
		if (setrlimit(RLIMIT_CORE, &rl))
			log_warn("set RLIMIT_CORE failed, try run this by root: %s", fwd_lasterrstr());
	} else
		log_warn("get RLIMIT_CORE failed, try run this by root! error: %s", fwd_lasterrstr());

	getrlimit(RLIMIT_NOFILE, &old);
	log_trace("getrlimit RLIMIT_NOFILE (cur: %ld, max: %ld)", old.rlim_cur, old.rlim_max);

	rl.rlim_cur = rl.rlim_max = FWD_MAX_NOFILE;
	if (setrlimit(RLIMIT_NOFILE, &rl) == 0) {
		getrlimit(RLIMIT_NOFILE, &rl);
		log_trace("getrlimit RLIMIT_NOFILE (cur: %ld, max: %ld)", rl.rlim_cur, rl.rlim_max);
	} else {

		if (errno == EPERM) {

			rl.rlim_cur = rl.rlim_max = old.rlim_max;
			if (setrlimit(RLIMIT_NOFILE, &rl) == 0) {
				getrlimit(RLIMIT_NOFILE, &rl);
				log_trace("getrlimit RLIMIT_NOFILE (cur: %ld, max: %ld)", rl.rlim_cur, rl.rlim_max);
			} else
				log_warn("sysrlimit RLIMIT_NOFILE failed: %s", strerror(errno));

		} else
			log_warn("sysrlimit RLIMIT_NOFILE failed: %s", strerror(errno));
	}

	return 0;
}

dt_int main(dt_int argc, dt_char **argv)
{
	dt_char log[1024];
	dt_int ret = 0;

	fwd_start_memory_trace(YES);

	signal(SIGINT,  fwd_sighandler);
	signal(SIGABRT, fwd_sighandler);
	signal(SIGKILL, fwd_sighandler);
	signal(SIGSTOP, fwd_sighandler);
	signal(SIGTERM, fwd_sighandler);

	snprintf(log, 1024, "/tmp/fwd_child_%d.log", dt_getpid());

#ifdef DEBUG
	log_set(LOG_TRACE, log);
#else
	log_set(LOG_INFO, log);
#endif

	fwd_reset_rlimit();

	do {
		_gfm = (fwd_main *)dt_malloc(sizeof(fwd_main));
		if (!_gfm)
			break;
		
		ret = fwd_main_entry(_gfm, argc, argv);

	} while (0);

	if (_gfm) {
		dt_free(_gfm);
		_gfm = NULL;
	}

	log_info("child %d exit\n", dt_getpid());
	fwd_stop_memory_trace();

	return ret;
}


