
#include "fwd.h"
#include "fwd_master.h"

#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef struct {
	dt_int           run;
	dt_int           cpu;
	dt_int           n_pid;
	dt_processid    *pid;
	dt_str		 fwd;
	dt_str		 cm;
	dt_int           cm_port;
} fwd_master;

static jmp_buf	jmpaddr;

static dt_void 
fwd_sighandler(dt_int sig)
{
	fprintf(stdout, "signal got %d\n", sig);
	longjmp(jmpaddr,1);
}

static dt_int 
fwd_master_init(fwd_master *fm)
{
	memset(fm, 0, sizeof(fwd_master));

	fm->cm_port = 0;
	fm->run     = YES;
	fm->cpu     = fwd_cpucore();
	fm->n_pid   = fm->cpu / 2; 

	if (fm->n_pid <= 0)
		fm->n_pid = 1;

	str_init(&fm->cm);
	str_init(&fm->fwd);
	str_set(&fm->fwd, "fwd");
	fm->pid = (dt_processid *)dt_malloc(sizeof(dt_processid) * fm->n_pid);

	if (!fm->pid)
		return FWD_ERR;

	memset(fm->pid, 0, sizeof(dt_processid) * fm->n_pid);
	return FWD_OK;
}


static dt_void 
fwd_child(fwd_master *fm)
{
	dt_int fd;
	dt_char *argv[10];
	dt_char buf[16];

	setsid();
	umask(0);
	fd = open("/dev/null", O_RDWR);
	if (fd > 0) {
		dup2(fd, STDIN_FILENO);
		dup2(fd, STDOUT_FILENO);
		close(fd);
	}

	snprintf(buf, sizeof(buf), "%d", fm->cm_port);

	argv[0] = (dt_char *)str_text(&fm->fwd);
	argv[1] = "-s";
	argv[2] = (dt_char *)str_text(&fm->cm);
	argv[3] = "-p";
	argv[4] = buf;
	argv[5] = NULL;

	if (execv(str_text(&fm->fwd), argv) == -1) {
		fprintf(stderr, "execl failed: %s:%s",
			str_text(&fm->fwd), fwd_lasterrstr());
	}

	exit(0);
}


static dt_processid 
fwd_spawn_fwd(fwd_master *fm)
{
	dt_processid pid;

	pid = fork();

	switch (pid) {

	case -1:
		fprintf(stderr, "fork() failed: %s", fwd_lasterrstr());
		return FWD_ERR;

	case 0:
		fwd_child(fm);
		break;
	}

	return pid;
}


static dt_int 
fwd_master_spawn(fwd_master *fm)
{
	dt_int i;

	for (i = 0; i < fm->n_pid; i++) {
		fm->pid[i] = fwd_spawn_fwd(fm);
		dt_sleep(1);

		if (fm->pid[i] <= 0)
			return FWD_ERR;

		fprintf(stdout, "new process: %ld started\n", fm->pid[i]);
		fflush(stdout);
	}

	return FWD_OK;
}

static dt_int
fwd_master_uninit(fwd_master *fm)
{
	dt_int i;

	fm->run = NO;
	if (fm->pid) {

		fprintf(stdout, "stopping....\n");

		for (i = 0; i < fm->n_pid; i++) {

			if (fm->pid[i] > 0) {

				if (kill(fm->pid[i], SIGTERM) == -1) {
					fprintf(stderr, "kill %ld error: %s\n",
								fm->pid[i], fwd_lasterrstr());

					switch(fwd_lasterr()) {

					case EPERM:
						fm->pid[i] = 0;
						break;

					default:
						break;
					}
				}
			}
		}

		
		fprintf(stdout, "wait for stopping....\n");

		i = 3;

		while ( i > 0) {
			fprintf(stdout, ".%d", i);
			fflush(stdout);
			i--;
			dt_sleep(1);
		}
		fprintf(stdout, "\n");

		for (i = 0; i < fm->n_pid; i++) {

			if (fm->pid[i] > 0) {

				switch(waitpid(fm->pid[i], NULL, WNOHANG)) {
				case -1:
					fprintf(stderr, "waitpid for %ld error: %s\n", 
								fm->pid[i], fwd_lasterrstr());
					break;

				case 0:
					fprintf(stdout, "force kill %ld\n", fm->pid[i]);
					kill(fm->pid[i], SIGKILL);
					waitpid(fm->pid[i], NULL, 0);
					break;

				default:
					fprintf(stdout, "process %ld exit normally\n", fm->pid[i]);
					break;
				}
			}
		}

		dt_free(fm->pid);
		fm->pid = NULL;
	}

	str_uninit(&fm->fwd);
	str_uninit(&fm->cm);

	return FWD_OK;
}

static dt_int
fwd_master_run(fwd_master *fm)
{
	dt_int i;
	dt_int st;

	while (fm->run) {

		for (i = 0; (i < fm->n_pid) && fm->run; i++) {
			if (fm->pid[i] <= 0) {
				fm->pid[i] = fwd_spawn_fwd(fm);
				continue;
			}
			
			switch(waitpid(fm->pid[i], &st, WNOHANG)) {

			case -1:
				fprintf(stderr, "waitpid error: %s\n", fwd_lasterrstr());
				break;

			case 0:
				//fprintf(stdout, "no process exist yet\n");
				break;

			default:

				if (WIFEXITED(st)) {
					fprintf(stdout, "process: %ld exited, status=%d\n",
							fm->pid[i], WEXITSTATUS(st));
				} else if (WIFSIGNALED(st)) {
					fprintf(stdout, "process: %ld killed by signal=%d\n",
							fm->pid[i], WTERMSIG(st));
				}

				if (WIFEXITED(st) || WIFSIGNALED(st))
					fm->pid[i] = 0;

				break;
			}
		}

		dt_sleep(1);
	}

	return FWD_OK;
}

static dt_int
fwd_master_getargs(fwd_master *fm, dt_int argc, dt_char **argv)
{
	dt_int c;

	if (argc < 5) {
		fprintf(stderr, "usage: %s -s server -p port\n", argv[0]);
		return FWD_ERR;
	}

	while ((c = getopt(argc, argv, "s:p:")) != -1) {
		switch(c) {
		case 's':
			str_set(&fm->cm, optarg);
			break;
		case 'p':
			fm->cm_port = atoi(optarg);
			break;
		default:
			fprintf(stderr, "usage: %s -s server -p port\n", argv[0]);
			return FWD_ERR;
		}
	}

	if (str_len(&fm->cm) <= 0 || fm->cm_port <= 0) {
		fprintf(stderr, "usage: %s -s server -p port\n", argv[0]);
		return FWD_ERR;
	}

	return FWD_OK;
}

static dt_int 
fwd_master_main(dt_int argc, dt_char **argv)
{
	fwd_master fm;
	dt_int     ret = FWD_ERR;

	if (fwd_master_init(&fm) != FWD_OK)
		return ERR;

	do {
		if (fwd_master_getargs(&fm, argc, argv) != FWD_OK)
			break;

		if (fwd_master_spawn(&fm) != FWD_OK)
			break;

		if (!setjmp(jmpaddr))
			ret = fwd_master_run(&fm);
		else
			fprintf(stderr, "got signal, do exit\n");
	} while (0);

	fwd_master_uninit(&fm);

	return ret;
}


dt_int 
main(dt_int argc, dt_char **argv)
{
	fwd_start_memory_trace(YES);

	signal(SIGINT,  fwd_sighandler);
	signal(SIGABRT, fwd_sighandler);
	signal(SIGKILL, fwd_sighandler);
	signal(SIGSTOP, fwd_sighandler);
	signal(SIGTERM, fwd_sighandler);

	log_set(LOG_TRACE, "/tmp/fwd_master.log");

	fwd_master_main(argc, argv);
	fwd_stop_memory_trace();
}
