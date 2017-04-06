
#include "ems_core.h"

#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>

static jmp_buf	jmpaddr;

ems_cchar *ems_popen_get(ems_cchar *fmt, ...)
{
	static ems_char res[512];
	FILE   *fp;
	va_list args;
	ems_buffer *buf = core_buffer();

	ems_char *p = buf_wr(buf);
	ems_int   l = buf_left(buf);

	va_start(args, fmt);
	vsnprintf(p, l, fmt, args);
	va_end(args);

	memset(res, 0, sizeof(res));
	fp = popen(p, "r");
	if (!fp)
		return NULL;

	fgets(res, sizeof(res), fp);

	ems_trim(res);

	pclose(fp);

	ems_l_trace("\033[00;32m cmd: %s, result: %s\033[00m", p, res);
	return res;
}

ems_int ems_systemcmd(ems_cchar *fmt, ...)
{
	va_list args;
	ems_buffer *buf = core_buffer();

	ems_char *p = buf_wr(buf);
	ems_int   l = buf_left(buf);

	va_start(args, fmt);
	vsnprintf(p, l, fmt, args);
	va_end(args);

	ems_l_trace("\033[00;31m >>>>>>>: %s \033[00m", p);

	if (system(p) != EMS_OK)
		return errno;

	return EMS_OK;
}

static ems_void ems_sighandler(ems_int sig)
{
	fprintf(stdout, "signal got %d\n", sig);
	longjmp(jmpaddr,1);
}

json_object *ems_json_tokener_parse_ex(ems_cchar *str, ems_int lstr)
{
	struct json_tokener* tok;
	struct json_object* obj;

	ems_assert(str != NULL);
	if (!str)
		return NULL;

	tok = json_tokener_new();
	if (!tok)
		return NULL;

	obj = json_tokener_parse_ex(tok, str, lstr);

	if(tok->err != json_tokener_success) {
		ems_l_warn("<< \033[01;32m JSON ERROR: (%d)%s:%s \033[00m>>>>",
			lstr, str, json_tokener_error_desc(tok->err));

		if (obj != NULL)
			json_object_put(obj);
		obj = NULL;
	}

	json_tokener_free(tok);
	return obj;
}

json_object *ems_json_tokener_parse(ems_cchar *str)
{
	return ems_json_tokener_parse_ex(str, -1);
}

ems_ctrl *ems_ctrl_new()
{
	ems_ctrl *ctrl = NULL;

	ctrl = (ems_ctrl *)ems_malloc(sizeof(ems_ctrl));
	if (ctrl) {
		ctrl->sess = NULL;
		ctrl->st   = st_stopped;
	}

	return ctrl;
}

ems_void ems_ctrl_destroy(ems_ctrl *ctrl)
{
	ems_assert(ctrl != NULL);
	if (ctrl) {
		ems_assert(ctrl->sess == NULL);
		ems_assert(ctrl->st == st_stopped);

		ems_free(ctrl);
	}
}


ems_capture *ems_capture_new()
{
	ems_capture *cap = NULL;

	cap = (ems_capture *)ems_malloc(sizeof(ems_capture));
	if (cap) {
		ems_queue_init(&cap->list_capture_inf);
		ems_queue_init(&cap->list_filters);
		cap->st = st_stopped;

		cap->mem_total = 0;
		cap->mem_left = 0;

		cap->_core = NULL;
		cap->max_capture_number = 0;
		cap->n_capture   = 0;
		cap->type        = 0;

		str_init(&cap->mac);
	}

	return cap;
}

ems_void ems_capture_destroy(ems_capture *cap)
{
	ems_assert(cap != NULL);

	if (cap) {
		ems_assert(ems_queue_empty(&cap->list_capture_inf));
		ems_assert(ems_queue_empty(&cap->list_filters));
		ems_assert(cap->st == st_stopped);

		str_uninit(&cap->mac);
		ems_free(cap);
	}
}


ems_output *ems_output_new()
{
	ems_output *output = NULL;

	output = (ems_output *)ems_malloc(sizeof(ems_output));
	if (output) {
		ems_queue_init(&output->list_files);

		ems_timeout_init(&output->to);
		output->pid = -1;
		output->st  = st_stopped;
		output->curfl = NULL;

	}

	return output;
}

ems_void ems_output_destroy(ems_output *output)
{
	ems_assert(output != NULL);

	if (output) {
		ems_assert(ems_queue_empty(&output->list_files));
		ems_assert(output->pid <= 0);
		ems_assert(output->st == st_stopped);
		ems_assert(output->curfl == NULL);

		ems_free(output);
	}
}

ems_int ems_core_init(ems_core *core)
{
	ems_assert(core);

	memset(core, 0, sizeof(ems_core));

	ems_buffer_init(&core->buf, EMS_BUFFER_8k);
	ems_event_init(&core->evt, EVT_DRIVER_EPOLL);
	core->flg = 0;

	core->ctrl    = ems_ctrl_new();
	core->capture = ems_capture_new();
	core->output  = ems_output_new();

	core->ctrl->_core    = core;
	core->capture->_core = core;
	core->output->_core  = core;

	ems_assert(core->ctrl && core->capture && core->output);
	if (!(core->ctrl && core->capture && core->output))
		return EMS_ERR;

	return EMS_OK;
}

ems_int ems_core_uninit(ems_core *core)
{
	ems_event_done(&core->evt);
	ems_buffer_uninit(&core->buf);

	if (core->ctrl)
		ems_ctrl_destroy(core->ctrl);

	if (core->capture)
		ems_capture_destroy(core->capture);

	if (core->output)
		ems_output_destroy(core->output);

	return EMS_OK;
}

ems_int core_pack_req(ems_session *sess, ems_uint tag)
{
	ems_cchar *ctx = NULL;
	ems_int   len, rtn;

	json_object *root = (json_object *)sess_request(sess);

	ctx = NULL;
	len = 0;
	if (root) {
		ctx = json_object_to_json_string(root);
		len = ems_strlen(ctx);
	}

	ems_l_trace("\033[01;33m <req tag: 0x%x, len: %d, ctx: %s> \033[00m",
			tag , len, ctx?ctx:"null");

	rtn = ems_pack_msg(tag, ctx, len, &sess->buf);

	if (root) {
		json_object_put(root);
		sess_request_set(sess, NULL);
	}

	return rtn;
}

ems_int core_pack_rsp(ems_session *sess, ems_uint tag, ems_int st)
{
	ems_cchar *ctx = NULL;
	ems_int   len, rtn;

	json_object *root = (json_object *)sess_response(sess);

	ctx = NULL;
	len = 0;

	if (!root)
		root = json_object_new_object();

	if (root) {
		json_object_object_add(root, "status", json_object_new_int(st));
		ctx = json_object_to_json_string(root);
		len = ems_strlen(ctx);
	}

	ems_l_trace("\033[01;34m<rsp tag: 0x%x, ctx(0x%x): %s> \033[00m",
		tag, len, (len < 1024)?(ctx?ctx:"no ctx"):"*CTX TOO LONG*");

	rtn = ems_pack_msg(tag, ctx, len, &sess->buf);

	if (root) {
		json_object_put(root);
		sess_response_set(sess, NULL);
	}

	return rtn;
}

ems_cchar *ems_strcat(ems_cchar *s1, ems_cchar *s2)
{
	static ems_char buf[512] = {0};

	snprintf(buf, 512, "%s%s 2&>/dev/null", s1, s2);
	return buf;
}

ems_int core_getargs(ems_core *core, ems_int argc, ems_char **argv)
{
	ems_int c;

	while ((c = getopt(argc, argv, "v")) != -1) {

		switch(c) {
		case 'v':
			ems_logger_set_level(logger(), EMS_LOG_TRACE);
			break;

		default:
			ems_l_error("usage: %s -v \n", argv[0]);
			return EMS_ERR;
		}
	}

	return 0;
}

ems_int ems_core_main(ems_core *core, ems_int argc, ems_char **argv)
{
	srandom(time(NULL));

	signal(SIGINT,  ems_sighandler);
	signal(SIGABRT, ems_sighandler);
	signal(SIGKILL, ems_sighandler);
	signal(SIGSTOP, ems_sighandler);
	signal(SIGTERM, ems_sighandler);

	do {
		if (core_getargs(core, argc, argv) != EMS_OK)
			break;

		if (ctrl_change_status(core->ctrl, st_init) != EMS_OK)
			break;

		if (capture_change_status(core->capture, st_init) != EMS_OK)
			break;

		if (output_change_status(core->output, st_init) != EMS_OK)
			break;

		if (!setjmp(jmpaddr)) {
			ems_event_run(&core->evt, NULL);
		} else
			ems_l_trace("aaa got signal, do exit");
	} while (0);

	ctrl_change_status(core->ctrl, st_stopped);
	capture_change_status(core->capture, st_stopped);
	output_change_status(core->output, st_stopped);

	return EMS_OK;
}

ems_cchar *ems_status_str(ems_status st)
{
	switch(st) {
	case st_init:
		return "START/INIT";

	case st_stopped:
		return "STOPPED";

	case st_normal:
		return "NORMAL/RUNNING/CAPTURING/PROCESS";

	case st_reg:
		return "REGISTER/UPLOADING/NOTIFY";

	case st_err:
		return "ERROR";

	case st_paused:
		return "PAUSED";

	case st_continue:
		return "CONTINUE";

	default:
		break;
	}

	return "UNKNOWN";
}

json_object *core_build_wtp_event(
		ems_int    type, 
		ems_int    ctype,
		ems_int    wlanid,
		ems_int    radioid,
		ems_cchar *inf,
		ems_int    errcode,
		ems_cchar *errmsg)
{
	json_object *jobj;

	ems_assert(inf != NULL);

	if (!inf)
		return NULL;

	jobj = json_object_new_object();
	if (!jobj)
		return NULL;

	json_object_object_add(jobj, "type",      json_object_new_int(type));
	json_object_object_add(jobj, "ctype",     json_object_new_int(ctype));
	json_object_object_add(jobj, "wlanid",    json_object_new_int(wlanid));
	json_object_object_add(jobj, "radioid",   json_object_new_int(radioid));
	json_object_object_add(jobj, "interface", json_object_new_string(inf));
	json_object_object_add(jobj, "errcode",   json_object_new_int(errcode));

	if (errmsg) {
		json_object_object_add(jobj, "errmsg", 
				json_object_new_string(errmsg));
	}
	return jobj;
}

json_object *core_build_apcfg_event(
		ems_int    evt,
		ems_cchar *arg,
		ems_int    errcode,
		ems_cchar *errmsg)
{
	json_object *jobj;

	jobj = json_object_new_object();
	if (!jobj)
		return NULL;

	json_object_object_add(jobj, "event", json_object_new_int(evt));
	if (arg)
		json_object_object_add(jobj,"arg", json_object_new_string(arg));

	json_object_object_add(jobj, "errcode", json_object_new_int(errcode));

	if (errmsg) {
		json_object_object_add(jobj, "errmsg", 
				json_object_new_string(errmsg));
	}
	return jobj;
}
