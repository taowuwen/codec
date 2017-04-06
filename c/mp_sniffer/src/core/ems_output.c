
#include "ems_core.h"
#include <sys/types.h>
#include <sys/stat.h>

#define out_timeout_set(out, msecs, cb) \
	ems_timeout_set(timeouter(), &out->to, msecs, cb, EMS_TIMEOUT_SORT)

#define out_timeout_cancel(out) ems_timeout_cancel(&out->to)


#define SNIFFER_UPLOADING_CMD	"/usr/sbin/sniffer_uploading"

/* currently do nothing for now */
static ems_int output_status_start(ems_output *out)
{
	ems_assert(out);
	ems_timeout_init(&out->to);

	return output_change_status(out, st_continue);
}


static ems_int out_child_uploading(ems_output *out)
{
	ems_do_capture *cap = NULL;
#ifndef DEBUG
	ems_int fd;
#endif
	ems_char *argv[10];

	setsid();
	umask(0);
	ems_logger_reset(logger());

#ifndef DEBUG
	fd = open("/dev/null", O_RDWR);
	if (fd > 0) {
		dup2(fd, STDIN_FILENO);
		dup2(fd, STDOUT_FILENO);
		close(fd);
	}
#endif
	/*
	   arg lists:
	   fl ftp(user, pass, addr, port, home) 
	 */
	cap = out->curfl->inf;
	ems_assert(cap != NULL);

	/* set env types */
	switch(cap->server.type) {
	case SERVER_TYPE_FTP:
		{
			/* server type */
			setenv("SERVER_TYPE", "ftp", 1);

			/* ftp params goes here */
			setenv("FTP_URL", str_text(&cap->server.ftp.url), 1);
		}
		break;

	default:
		exit(1);
		break;
	}
	/* set server extra params */
	setenv("FILE_COMPRESS",  ems_itoa(cap->server.compress), 1);

	/* uploading file info */
	setenv("UPLOADING_ROOT", str_text(&out->curfl->path), 1);
	setenv("UPLOADING_FILE", str_text(&out->curfl->name), 1);

	argv[0] = (ems_char *)SNIFFER_UPLOADING_CMD;
	argv[1] = NULL;

	if (execv(SNIFFER_UPLOADING_CMD, argv) == -1)
		fprintf(stderr, "exec cmd: %s failed: %s", 
				SNIFFER_UPLOADING_CMD, ems_lasterrmsg());

	exit(-1);
}

static ems_int output_do_uploading(ems_output *out)
{
	pid_t pid;

	pid = fork();

	switch (pid) {
	case -1:
		ems_l_error("fork failed: %s", ems_lasterrmsg());
		return ERR_EVT_MEMORY_ERROR;

	case 0:
		out_child_uploading(out);
		break;
	}

	out->pid = pid;

	return EMS_OK;
}

static ems_int output_status_continue(ems_output *out)
{
	ems_sniffer_file *fl;
	ems_int           ret;

	ems_assert(out);

	if (ems_queue_empty(&out->list_files))
		return output_change_status(out, st_paused);

	fl = ems_container_of(ems_queue_head(&out->list_files), 
			ems_sniffer_file, output_entry);

	ems_queue_remove(&fl->output_entry);

	/* mark uploading for do capture module, which could stop current
	 * routing*/
	ems_flag_set(fl->flg, FLG_UPLOADING);

	out->curfl = fl;

	ret = output_do_uploading(out);

	if (ret == EMS_OK)
		return output_change_status(out, st_process);

	out->lasterr = ret;
	output_change_status(out, st_notify);

	return ret;
}

static ems_int output_status_paused(ems_output *out)
{
	/* nothing to do, just wait uploading event */
	return EMS_OK;
}

static ems_void out_timeout_cb(ems_timeout *timeout)
{
	ems_int     st;
	ems_output *out = ems_container_of(timeout, ems_output, to);

	ems_assert(out->st == st_process && out->pid > 0);

	out->lasterr = 0;

	switch (waitpid(out->pid, &st, WNOHANG)) {
	case 0:
		ems_l_trace("%ld uploading not finished yet", out->pid);
		out_timeout_set(out, 1000, out_timeout_cb); 
		break;

	default:
		ems_l_trace("output cmd(%d) exit code: 0x%x, status: %d",
				out->pid, st, WEXITSTATUS(st));

		out->lasterr = WEXITSTATUS(st);
		out->pid = 0;
		output_change_status(out, st_notify);
		break;
	}
}

static ems_int output_status_process(ems_output *out)
{
	/* set timeout only, nothing else need to do */
	/* check it every min */
	out_timeout_set(out, 1000, out_timeout_cb); 

	return EMS_OK;
}

static ems_int output_status_notify(ems_output *out)
{
	ems_char fl[512];
	struct  stat st;

	ems_l_trace("output notify, lasterr: 0x%x", out->lasterr);

	ems_assert(out->curfl != NULL);

	snprintf(fl, sizeof(fl), "%s/%s", 
		str_text(&out->curfl->path), str_text(&out->curfl->name));

	if (!stat(fl, &st)) {
		ems_l_warn("file: %s did not deleted, remove it", fl);
		remove(fl);
	}

	cap_file_upload_finished(out->curfl->inf, out->curfl, out->lasterr);

	output_change_status(out, st_continue);

	return EMS_OK;
}

static ems_int output_status_stop(ems_output *out)
{
	out_timeout_cancel(out);

	/* stop current processing or uploading */
	if (out->pid > 0) {
		/* exec "sniffer stop" cmd to terminate all the process that
		 * uploading */
		kill(out->pid, SIGKILL);
		waitpid(out->pid, NULL, 0);
		out->pid = 0;
	}

	out->curfl = NULL;

	ems_queue_clean(&out->list_files);

	return EMS_OK;
}


ems_int output_change_status(ems_output *out, ems_status st)
{
	ems_l_trace("[output] change status from %s into %s", 
			ems_status_str(out->st), ems_status_str(st));

	if (out->st == st)
		return EMS_OK;

	out->st = st;

	switch(st) {
	case st_start:
		return output_status_start(out);

	case st_continue:
		return output_status_continue(out);

	case st_paused:
		return output_status_paused(out);

	case st_process:
		return output_status_process(out);

	case st_notify:
		return output_status_notify(out);

	case st_stopped:
		return output_status_stop(out);

	default:
		ems_assert(0);
		break;
	}

	return EMS_OK;
}

ems_int output_upload_file(ems_output *out, ems_sniffer_file *fl)
{
	ems_assert(out && fl);
	ems_assert(out->st != st_start && out->st != st_stopped);

	ems_l_trace("output, append file %s/%s",
			str_text(&fl->path), str_text(&fl->name));
	ems_queue_insert_tail(&out->list_files, &fl->output_entry);

	if (out->st == st_paused)
		return output_change_status(out, st_continue);

	return EMS_OK;
}
