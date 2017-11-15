
#include "audit.h"
#include "ems_core.h"
#include "ems_client.h"
#include "out.h"

#include <sys/types.h>
#include <sys/stat.h>


typedef struct _e6wifi_core_s {
	ems_timeout 	to;
	ems_str	        fl;
	ems_uint        flg;
	ems_processid	pid;

	struct /* for ftp info */{
		ems_str  addr;
		ems_int  port;
		ems_str  user;
		ems_str  pass;
		ems_str  home;
	} ftp;

	struct /* for policy control */{
		ems_uint    max_fl_sz;
		ems_uint    max_tm_upload;
		ems_int     city_code;
	} policy;

} e6wifi_core;


static ems_void e6wifi_load_cfg(e6wifi_core *e6)
{
	ems_cfg   *cfg;

	cfg = emscfg();

	/* ftp info */
	e6->ftp.port = ems_atoi(cfg_get(cfg, CFG_out_e6wifi_ftp_port));
	if (e6->ftp.port <= 0)
		e6->ftp.port = 21;

	str_set(&e6->ftp.addr, cfg_get(cfg, CFG_out_e6wifi_ftp_addr));
	str_set(&e6->ftp.user, cfg_get(cfg, CFG_out_e6wifi_ftp_user));
	str_set(&e6->ftp.pass, cfg_get(cfg, CFG_out_e6wifi_ftp_pass));
	str_set(&e6->ftp.home, cfg_get(cfg, CFG_out_e6wifi_ftp_home));

	e6->policy.max_fl_sz = ems_atoi(cfg_get(cfg, CFG_out_e6wifi_policy_max_fl_sz));

	/* for default, max file size == 10M*/
	if (e6->policy.max_fl_sz <= 0)
		e6->policy.max_fl_sz = EMS_BUFFER_1K * EMS_BUFFER_1K * 10;

	/* for default, max timeout for upload: 30min */
	e6->policy.max_tm_upload = ems_atoi(cfg_get(cfg, CFG_out_e6wifi_policy_max_tm_upload));
	if (e6->policy.max_tm_upload <= 0)
		e6->policy.max_tm_upload = 1800;
}

static ems_int e6wifi_init(out_plugin *plg)
{
	e6wifi_core  *e6 = NULL;

	ems_l_trace("16wifi init");

	e6 = (e6wifi_core *)ems_malloc(sizeof(e6wifi_core));
	if (!e6)
		return EMS_ERR;

	memset(e6, 0, sizeof(e6wifi_core));

	ems_timeout_init(&e6->to);
	e6->pid = 0;
	e6->flg = 0;

	str_init(&e6->fl);
	str_init(&e6->ftp.addr);
	str_init(&e6->ftp.user);
	str_init(&e6->ftp.pass);
	str_init(&e6->ftp.home);

	e6wifi_load_cfg(e6);

	plg->ctx = (ems_void *)e6;

	return EMS_OK;
}

static ems_int e6wifi_upload_file(e6wifi_core *e6);

static ems_void e6wifi_timeout_cb(ems_timeout *timeout)
{
	struct stat s;
	ems_int     st;
	e6wifi_core *e6 = ems_container_of(timeout, e6wifi_core, to);

	ems_l_trace("e6wifi timer triger, time to send msgs");

	if (stat(str_text(&e6->fl), &s))  goto out;

	if (e6->pid > 0) {
		switch(waitpid(e6->pid, &st, WNOHANG)) {
		case 0:
			ems_l_trace("%ld ftp upload not finished, try next time", e6->pid);
			if (s.st_size >= e6->policy.max_fl_sz)
				out_sendmsg(id_out_16wifi, id_out_file, A_AUDIT_FILE_FULL, NULL);
			goto out;

		default:
#ifdef DEBUG
			if (WIFEXITED(st)) {
				ems_l_trace("upload success: %ld exited, status=%d", 
						e6->pid, WEXITSTATUS(st));
			} else if (WIFSIGNALED(st)) {
				ems_l_trace("upload failed maybe process: %ld killed by signal=%d",
						e6->pid, WTERMSIG(st));
			}
#endif
			e6->pid = 0;
			break;
		}
	}

	if (s.st_size > 0) {
		out_sendmsg(id_out_16wifi, id_out_file, A_AUDIT_FILE_TRUNCATE, NULL);
		/* update timers here */
		ems_timeout_cancel(&e6->to);
		ems_timeout_insert_tail(timeouter(), 
			&e6->to, e6->policy.max_tm_upload * 1000, e6wifi_timeout_cb);
		e6wifi_upload_file(e6);
	}

out:
	ems_timeout_insert_tail(timeouter(), 
		&e6->to, e6->policy.max_tm_upload * 1000, e6wifi_timeout_cb);
}

static ems_int e6wifi_start(e6wifi_core *e6)
{
	ems_l_trace("e6wifi start, ftp(%s:%s@%s:%d:%s), max_fl_size: %d, max_tm_upload: %d",
			str_text(&e6->ftp.user),
			str_text(&e6->ftp.pass),
			str_text(&e6->ftp.addr),
			e6->ftp.port,
			str_text(&e6->ftp.home),
			e6->policy.max_fl_sz,
			e6->policy.max_tm_upload);

	ems_flag_set(e6->flg, FLG_RUN);

	ems_timeout_insert_tail(timeouter(), 
		&e6->to, e6->policy.max_tm_upload * 1000, e6wifi_timeout_cb);

	return EMS_OK;
}

static ems_int e6wifi_stop(e6wifi_core *e6)
{
	ems_l_trace("e6wifi stop");
	ems_timeout_cancel(&e6->to);
	ems_flag_unset(e6->flg, FLG_RUN);


	if (e6->pid > 0) {
		switch(waitpid(e6->pid, NULL, WNOHANG)) {
		case 0:
			ems_l_trace("child %ld did not exist yet, kill it", e6->pid);
			kill(e6->pid, SIGKILL);
			waitpid(e6->pid, NULL, 0);
			break;

		default:
			ems_l_trace("e6wifi upload already exit");
			break;
		}
		e6->pid = 0;
	}

	return EMS_OK;
}

static ems_void 
e6wifi_installer(e6wifi_core *e6, ems_cchar *cmd)
{
#ifndef DEBUG
	ems_int fd;
#endif
	ems_char *argv[10];

	setsid();
	umask(0);

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
	argv[0] = (ems_char *)cmd;
	argv[1] = (ems_char *)str_text(&e6->fl);
	argv[2] = (ems_char *)str_text(&emscfg()->fl);
	argv[3] = NULL;

#if 0
	argv[2] = (ems_char *)str_text(&e6->ftp.user);
	argv[3] = (ems_char *)str_text(&e6->ftp.pass);
	argv[4] = (ems_char *)str_text(&e6->ftp.addr);
	argv[5] = (ems_char *)ems_itoa(e6->ftp.port);
	argv[6] = (ems_char *)str_text(&e6->ftp.home);
	argv[7] = NULL;
#endif

	if (execv(cmd, argv) == -1)
		ems_l_trace("exec cmd: %s failed: %s", cmd, ems_lasterrmsg());

	exit(0);
}


static ems_int e6wifi_upload_file(e6wifi_core *e6)
{
	ems_processid pid;
	struct  stat  st;

	ems_cchar   *setup = cfg_get(emscfg(), CFG_out_e6wifi_ftp_sh);
	if (!setup)
		setup = "./16wifi_upload.sh";

	if (stat(setup, &st)) {
		ems_l_trace("upload file: %s missing", setup);
		remove(str_text(&e6->fl));
		return EMS_ERR;
	}

	if (!(st.st_mode & S_IXUSR))
		chmod(setup, st.st_mode | S_IXUSR);

	pid = fork();

	switch (pid) {
	case -1:
		ems_l_trace("fork() failed: %s", ems_lasterrmsg());
		return EMS_ERR;

	case 0:
		e6wifi_installer(e6, setup);
		break;
	}

	e6->pid = pid;

	return EMS_OK;
}

static ems_int e6wifi_evt_log(e6wifi_core *e6, ems_cchar *fl)
{
	struct stat s;
	ems_int     st;
	
	ems_l_trace("e6wifi log file: %s", fl);

	if (stat(fl, &s)) {
		ems_l_trace("e6wifi stat error on : %s, %s", fl, ems_lasterrmsg());
		return EMS_ERR;
	}

	if (str_len(&e6->fl) <= 0 || strcmp(str_text(&e6->fl), fl)) {
		str_set(&e6->fl, fl);
	}

	if (e6->pid > 0) {
		switch(waitpid(e6->pid, &st, WNOHANG)) {
		case 0:
			ems_l_trace("%ld ftp upload not finished, try next time", e6->pid);
			if (s.st_size >= e6->policy.max_fl_sz)
				out_sendmsg(id_out_16wifi, id_out_file, A_AUDIT_FILE_FULL, NULL);
			return EMS_OK;

		default:
#ifdef DEBUG
			if (WIFEXITED(st)) {
				ems_l_trace("upload success: %ld exited, status=%d", 
						e6->pid, WEXITSTATUS(st));
			} else if (WIFSIGNALED(st)) {
				ems_l_trace("upload failed maybe: %ld killed by signal=%d",
						e6->pid, WTERMSIG(st));
			}
#endif
			e6->pid = 0;
			break;
		}
	}

	if (s.st_size >= e6->policy.max_fl_sz) {
		out_sendmsg(id_out_16wifi, id_out_file, A_AUDIT_FILE_TRUNCATE, NULL);
		return e6wifi_upload_file(e6);
	}

	return EMS_OK;
}


static ems_int e6wifi_evt_ftp(e6wifi_core *e6, json_object *req)
{
	ems_cfg   *cfg = emscfg();
	ems_str addr, user, pass, home;
	ems_int port, rtn;

	ems_assert(req != NULL && json_object_is_type(req, json_type_object));

	str_init(&addr);
	str_init(&user);
	str_init(&pass);
	str_init(&home);
	do {
		rtn = EMS_ERR;

		ems_json_get_string_def(req, "addr",     &addr, NULL); 
		if (str_len(&addr) <= 0) break;

		ems_json_get_int_def   (req, "port",      port, 21);
		if (port <= 0) break;

		ems_json_get_string_def(req, "username", &user, NULL);
		if (str_len(&user) <= 0) break;

		ems_json_get_string_def(req, "password", &pass, NULL);
		if (str_len(&pass) <= 0) break;

		ems_json_get_string_def(req, "home"    , &home, NULL);
		if (str_len(&home) <= 0) break;

		e6->ftp.port = port;
		str_cpy(&e6->ftp.addr, &addr);
		str_cpy(&e6->ftp.user, &user);
		str_cpy(&e6->ftp.pass, &pass);
		str_cpy(&e6->ftp.home, &home);

		ems_l_trace("current ftp info(%s:%s@%s:%d%s)", 
				str_text(&e6->ftp.user), str_text(&e6->ftp.pass),
				str_text(&e6->ftp.addr), e6->ftp.port,
				str_text(&e6->ftp.home));

		cfg_set(cfg, CFG_out_e6wifi_ftp_addr, str_text(&addr));
		cfg_set(cfg, CFG_out_e6wifi_ftp_user, str_text(&user));
		cfg_set(cfg, CFG_out_e6wifi_ftp_pass, str_text(&pass));
		cfg_set(cfg, CFG_out_e6wifi_ftp_home, str_text(&home));
		cfg_set(cfg, CFG_out_e6wifi_ftp_port, ems_itoa(port));

		cfg_write(cfg);

		rtn = EMS_OK;
	} while (0);

	str_uninit(&addr);
	str_uninit(&user);
	str_uninit(&pass);
	str_uninit(&home);

	return rtn;
}


static ems_int e6wifi_process(out_plugin *plg, ems_uint evt, ems_void *arg)
{
	e6wifi_core *e6 = (e6wifi_core *)plg->ctx;

	ems_l_trace("16wifi process, evt: %d", evt);

	switch(evt) {

	case A_AUDIT_START:
		return e6wifi_start(e6);

	case A_AUDIT_STOP:
		return e6wifi_stop(e6);

	default:
		break;
	}

	if (ems_flag_unlike(e6->flg, FLG_RUN)) {
		ems_l_trace("e6wifi not start yet");
		return EMS_ERR;
	}

	switch(evt) {
	case A_AUDIT_LOG:
		return e6wifi_evt_log(e6, (ems_cchar *)arg);

	case MSG_E6WIFI_FTP:
		return e6wifi_evt_ftp(e6, (json_object *)arg);

	default:
		break;
	}

	return EMS_OK;
}

static ems_int e6wifi_uninit(out_plugin *plg)
{
	e6wifi_core *e6 = (e6wifi_core *)plg->ctx;
	ems_l_trace("16wifi uninit");

	str_uninit(&e6->fl);
	str_uninit(&e6->ftp.addr);
	str_uninit(&e6->ftp.user);
	str_uninit(&e6->ftp.pass);
	str_uninit(&e6->ftp.home);

	ems_assert(e6->pid == 0);
	ems_free(e6);

	plg->ctx = NULL;

	return EMS_OK;
}


out_plugin out_16wifi = {
	.id    = id_out_16wifi,
	.mount = id_out_file,
	.desc  = ems_string("16wifi"),
	.ctx   = NULL,
	.init  = e6wifi_init,
	.process = e6wifi_process,
	.uninit  = e6wifi_uninit,
};

