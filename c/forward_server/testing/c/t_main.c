
#include "t_main.h"
#include "t_worker.h"
#include "fwd_sock.h"

#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/resource.h>

typedef struct {
	dt_int		 total_session;

	t_sock	 	 cm; /* for address */
	struct sockaddr_in cm_addr;
	
	dt_int		 cpucore;
	dt_int		 pagesize;

	dt_uint		 flgs;

	dt_int		 n_hb; /* for heartbeat times */
	dt_int		 n_retry; /* for retry times get "tickets" */
	off_t		 sz_fl; /* size for random contents. */


	dt_mtx		 mtx;
	dt_uint		 n_success;
	dt_uint		 n_failed;
	dt_int		 n_fwd;
	dt_str		 fwd_arg;

	dt_int		 n_worker;
	t_worker	*worker;
} t_master;


/*
for global var
*/

static jmp_buf	jmpaddr;
static t_master *_gtm = NULL;

static dt_void 
t_sighandler(dt_int sig)
{
	longjmp(jmpaddr,1);
}

static dt_int t_reset_rlimit()
{
	struct rlimit rl, old; 

	if (getrlimit(RLIMIT_CORE, &rl) == 0) {
		log_trace("getrlimit RLIMIT_CORE (cur: %ld, max: %ld)", rl.rlim_cur, rl.rlim_max);
		rl.rlim_cur = rl.rlim_max;
		if (setrlimit(RLIMIT_CORE, &rl))
			log_warn("set RLIMIT_CORE failed, try run this by root: %s", t_lasterrstr());
	} else
		log_warn("get RLIMIT_CORE failed, try run this by root! error: %s", t_lasterrstr());

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

static dt_int t_master_init(t_master *tm)
{
	dt_int i;
	dt_assert(tm != NULL);

	memset(tm, 0, sizeof(t_master));

	tm->cpucore  = fwd_cpucore();
	tm->pagesize = fwd_pagesize();

	dt_mtx_init(tm->mtx);
	tm->n_success = 0;
	tm->n_failed  = 0;

	tm->n_worker = tm->cpucore;
	if (tm->n_worker <= 0)
		tm->n_worker = 1;

	tm->worker = (t_worker *)dt_malloc(sizeof(t_worker) * tm->n_worker);
	if (!tm->worker)
		return T_ERR;

	for (i = 0; i < tm->n_worker; i++)
		t_worker_init(&tm->worker[i]);

	fwd_sock_init(&tm->cm);

	tm->n_hb    = 1;
	tm->n_retry = 1;
	tm->sz_fl   = 0x1000;
	tm->n_fwd   = 1;
	str_init(&tm->fwd_arg);

	return T_OK;
}


static dt_void t_master_uninit(t_master *tm)
{
	dt_int i;

	dt_assert(tm != NULL && "invalid args");

	if (tm->worker) {

		for (i = 0; i < tm->n_worker; i++)
			t_worker_uninit(&tm->worker[i]);

		dt_free(tm->worker);
	}

	fwd_sock_clear(&tm->cm);

	log_info(">>>>>*****[ total success: %u, failed: %u ]*****<<<<<<", tm->n_success, tm->n_failed);

	dt_mtx_destroy(tm->mtx);
	tm->n_success = 0;
	tm->n_failed  = 0;
	str_clear(&tm->fwd_arg);
}


static dt_void print_usage(dt_cchar *proc)
{
	printf("usage: %s -s server -p port -n users [af:r:h:]\n"
			"-a .send all the connection the same time[default no]\n"
			"-r size. send random contents with size of \"size\"[default: no]\n"
			"-h n,  send heartbeat for n times[default: 1]\n"
			"-t n,  retry to get ticket[default: 1]\n"
			"-f fwd reg times for one user\n"
			"-g fetch pages e.g. index.html\n"
			"-c recv all data(256k max for default) \n"
			, proc);
}

static dt_int
t_master_getargs(t_master *tm, dt_int argc, dt_char **argv)
{
	dt_int c;

	if (argc < 5) {
		print_usage(argv[0]);
		return T_ERR;
	}

	while ((c = getopt(argc, argv, "s:p:n:af:r:h:t:g:c")) != -1) {

		switch(c) {
		case 'g':
			str_set(&tm->fwd_arg, optarg);
			break;

		case 's':
			fwd_sock_setaddr(&tm->cm, optarg);
			break;
		case 'p':
			fwd_sock_setport(&tm->cm, atoi(optarg));
			break;

		case 'n':
			tm->total_session = atoi(optarg);
			break;

		case 'a':
			tm->flgs |= T_FLG_ALL_CONNECT_THE_SAME_TIME;
			break;

		case 'r':
			tm->sz_fl = strtol(optarg, NULL, 16);
			tm->flgs |= T_FLG_SEND_FILE;
			break;

		case 'c':
			tm->flgs |= T_FLG_RECV_ALL;
			break;

		case 'h':
			tm->n_hb  = atoi(optarg);
			break;

		case 't':
			tm->n_retry = atoi(optarg);
			break;

		case 'f':
			tm->n_fwd   = atoi(optarg);
			break;

		default:
			print_usage(argv[0]);
			return T_ERR;
		}
	}

	if (	   fwd_strlen(fwd_sock_addr(&tm->cm)) <= 0 
		|| fwd_sock_port(&tm->cm) <= 0
		|| tm->total_session <= 0
		) {
		print_usage(argv[0]);
		return T_ERR;
	}

	memset(&tm->cm_addr, 0, sizeof(tm->cm_addr));

	printf("\ncm server: %s:%d\n"
		  "heartbeat: %d\n"
		  "random file size: 0x%lx\n"
		  "retry times: %d\n"
		  "connect the same time? %s\n"
		  "user: %d\n"
		  "fwd times: %d\n",
			fwd_sock_addr(&tm->cm), 
			fwd_sock_port(&tm->cm),
			tm->n_hb,
			tm->sz_fl,
			tm->n_retry,
			t_test(T_FLG_ALL_CONNECT_THE_SAME_TIME)?"yes":"no",
			tm->total_session,
			tm->n_fwd
			);

	if (dt_gethostbyname(fwd_sock_addr(&tm->cm), &tm->cm_addr) != T_OK) {
		printf("gethostbyname failed: %s", t_lasterrstr());
		return T_ERR;
	}

	return T_OK;
}


static dt_int t_master_run_workers(t_master *tm)
{
	dt_int 	i;
	dt_int  user;

	user = tm->total_session / tm->n_worker;

	if (user > 0)  {
		/* start from the second worker */
		for (i = 1; i < tm->n_worker; i++)
			t_worker_start(&tm->worker[i], YES, user);

		user = tm->total_session - user * (tm->n_worker -1);
	} else
		user = tm->total_session;

	return t_worker_start(&tm->worker[0], NO, user);
}

dt_int t_master_entry(t_master *tm, dt_int argc, dt_char **argv)
{
	if (t_master_init(tm) != T_OK)
		return T_ERR;

	do {
		if (t_master_getargs(tm, argc, argv) != OK)
			break;

		if (!setjmp(jmpaddr))
			t_master_run_workers(tm);
		else {
			log_err("got signal, do exit");
		}
	} while (0);

	t_master_uninit(tm);

	return T_OK;
}

extern dt_int t_logger_init();
extern dt_void t_logger_uninit();

dt_int main(dt_int argc, dt_char **argv)
{
	dt_char log[1024];
	dt_int ret = 0;


	fwd_start_memory_trace(YES);

	signal(SIGINT,  t_sighandler);
	signal(SIGABRT, t_sighandler);
	signal(SIGKILL, t_sighandler);
	signal(SIGSTOP, t_sighandler);
	signal(SIGTERM, t_sighandler);

	//snprintf(log, 1024, "/tmp/t_fwd_cm%d.log", dt_getpid());
	snprintf(log, 1024, "/tmp/t_fwd_cm.log");

#ifdef DEBUG
	log_set(LOG_TRACE, log);
#else
	log_set(LOG_INFO, log);
#endif
	t_logger_init();

	t_reset_rlimit();
	srand(time(NULL));

	do {
		_gtm = (t_master *)dt_malloc(sizeof(t_master));
		if (!_gtm)
			break;
		
		ret = t_master_entry(_gtm, argc, argv);

	} while (0);

	if (_gtm) {
		dt_free(_gtm);
		_gtm = NULL;
	}

	t_logger_uninit();
	fwd_stop_memory_trace();

	return ret;
}

dt_cchar *t_main_cm_server()
{
	dt_assert(_gtm && "bug");

	return fwd_sock_addr(&_gtm->cm);
}

dt_int t_main_cm_port()
{
	dt_assert(_gtm && "bug");

	return fwd_sock_port(&_gtm->cm);
}

struct sockaddr_in *t_main_cm_addr()
{
	return &_gtm->cm_addr;
}

dt_int t_test(dt_uint flg)
{
	return (_gtm->flgs & flg);
}

dt_int t_main_hb()
{
	return _gtm->n_hb;
}

dt_int t_main_retry()
{
	return _gtm->n_retry;
}

off_t  t_main_file_size()
{
	return _gtm->sz_fl;
}

dt_int t_main_success_inc()
{
	dt_mtx_lock(_gtm->mtx);
	_gtm->n_success++;
	dt_mtx_unlock(_gtm->mtx);

	return _gtm->n_success;
}

dt_int t_main_failed_inc()
{
	dt_mtx_lock(_gtm->mtx);
	_gtm->n_failed++;
	dt_mtx_unlock(_gtm->mtx);

	return _gtm->n_failed;
}

dt_int t_main_fwd_times()
{
	return _gtm->n_fwd;
}

dt_cchar *t_main_fwd_args()
{
	return str_text(&_gtm->fwd_arg);
}

