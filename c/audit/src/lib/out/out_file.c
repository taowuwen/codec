
#include "audit.h"
#include "ems_core.h"
#include "ems_client.h"
#include "out.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


typedef struct _out_file_s {
	ems_int     fd;
	ems_str     fl;
	ems_uint    flg;
	out_plugin  *plg;
	audit_status st;
} out_file_core;

static ems_int file_init(out_plugin *plg)
{
	out_file_core *fl = NULL;

	ems_l_trace("out file init");

	fl = (out_file_core *)ems_malloc(sizeof(out_file_core));
	if (!fl) 
		return EMS_ERR;

	memset(fl, 0, sizeof(out_file_core));

	fl->fd  = 0;
	str_init(&fl->fl);
	fl->plg = plg;
	fl->st  = st_stop;
	fl->flg = 0;

	plg->ctx = (ems_void *)fl;

	return EMS_OK;
}

static ems_int out_file_init(out_file_core *fl)
{
	ems_char buf[512] = {0};

	fl->st = st_init;

	if (fl->fd > 0) {
		close(fl->fd);
		fl->fd = 0;
	}

	snprintf(buf, sizeof(buf), "/tmp/audit_file_%u.log", (ems_uint)time(NULL));

	fl->fd = open(buf, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	if (fl->fd <= 0) {
		fl->st = st_error;
		return EMS_ERR;
	}

	str_set(&fl->fl, buf);

	fl->st = st_normal;

	return EMS_OK;
}


static ems_int out_file_start(out_file_core *fl)
{
	out_plugin *plg = fl->plg;

	out_load_plugins(&plg->entry_post, id_out_file);

	if (!ems_queue_empty(&plg->entry_post))
		out_plug_broadcast(&plg->entry_post, A_AUDIT_START, NULL);

	/* open files goes from here */
	/* start --> init --> normal --> update --> normal --> error ...  stop*/
	fl->st = st_start;

	if (out_file_init(fl) != EMS_OK) {
		fl->st = st_stop;
		return EMS_ERR;
	}

	return EMS_OK;
}

static ems_int out_file_stop(out_file_core *fl)
{
	out_plugin *plg = fl->plg;

	if (!ems_queue_empty(&plg->entry_post)) {
		out_plug_broadcast(&plg->entry_post, A_AUDIT_STOP, NULL);
		out_unload_plugins(&plg->entry_post);
	}

	fl->st = st_stop;

	if (fl->fd) {
		close(fl->fd);
		fl->fd = 0;
	}

	str_uninit(&fl->fl);

	return EMS_OK;
}

static ems_int output_into_file(out_file_core *fl, ems_buffer *buf)
{
	ems_int total = 0;
	ems_int rtn;

	while (buf_len(buf) > 0 ) {

		rtn = write(fl->fd, buf_rd(buf), buf_len(buf));
		if (rtn == 0) {
			break;
		} else if (rtn == -1) {
			ems_l_trace("write into %s, failed: %s", 
					str_text(&fl->fl), ems_lasterrmsg());
			return EMS_ERR;
		}
		else {
			total += rtn;
			ems_buffer_seek_rd(buf, rtn, EMS_BUFFER_SEEK_CUR);
		}
	}

	/* if we are on disk, we should make it a nonblock mode, but not now */
	return total;
}

static ems_int out_file_update(out_file_core *fl, ems_buffer *buf)
{
	switch(fl->st) {
	case st_stop:
		return EMS_ERR;

	case st_error:
		if (out_file_init(fl) != EMS_OK) 
			return EMS_ERR;
		break;

	case st_normal:
		break;

	default:
		return EMS_ERR;
	}

	ems_assert(fl->st == st_normal);

	if (buf_len(buf) > 0) {
		if (ems_flag_unlike(fl->flg, FLG_FILE_FULL)) {
			if (output_into_file(fl, buf) <= 0) {
				fl->st = st_error;
				return EMS_ERR;
			}
		}

		return out_plug_broadcast(&fl->plg->entry_post, A_AUDIT_LOG, (ems_void *)str_text(&fl->fl));
	}

	return EMS_OK;
}

static ems_int out_file_truncate(out_file_core *fl)
{
	ems_assert(fl);
	if (fl->fd > 0) {
		close(fl->fd); /* flush buffered into file */
		fl->fd = 0;
	}

	fl->st = st_error;
	ems_flag_unset(fl->flg, FLG_FILE_FULL);
	return EMS_OK;
}

static ems_int out_file_full(out_file_core *fl)
{
	ems_flag_set(fl->flg, FLG_FILE_FULL);
	return EMS_OK;
}

static ems_int file_process(out_plugin *plg, ems_uint evt, ems_void *arg)
{
	out_file_core *fl = (out_file_core *)plg->ctx;
	ems_l_trace("out file process: evt: %d(0x%x)", evt, evt);

	switch(evt) {

	case A_AUDIT_START:
		ems_l_trace("out file start");
		return out_file_start(fl);

	case A_AUDIT_STOP:
		ems_l_trace("out file stop");
		return out_file_stop(fl);

	case A_AUDIT_LOG:
		return out_file_update(fl, (ems_buffer *)arg);

	case A_AUDIT_FILE_TRUNCATE:
		ems_l_trace("out, file truncate");
		return out_file_truncate(fl);

	case A_AUDIT_FILE_FULL:
		ems_l_trace("out, file full");
		return out_file_full(fl);

	default:
		break;
	}

	return EMS_OK;
}

static ems_int file_uninit(out_plugin *plg)
{
	out_file_core *fl = (out_file_core *)plg->ctx;

	ems_l_trace("out file uninit");

	ems_assert(fl && fl->fd == 0);
	str_uninit(&fl->fl);
	fl->plg = NULL;
	ems_assert(fl->st == st_stop);

	plg->ctx = NULL;

	ems_free(fl);

	return EMS_OK;
}


out_plugin out_file = {
	.id    = id_out_file,
	.mount = id_out,
	.desc  = ems_string("file"),
	.ctx   = NULL,
	.init  = file_init,
	.process = file_process,
	.uninit  = file_uninit,
};


