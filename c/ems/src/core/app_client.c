
#include "ems_core.h"
#include "ems_client.h"
#include "app.h"
#include "ems_fw.h"
#include "ems_dns.h"

ems_int clnt_init(app_module *mod)
{
	ems_client *cl = NULL;

	cl = (ems_client *)ems_malloc(sizeof(ems_client));
	if (cl) {
		memset(cl, 0, sizeof(ems_client));

		cl->st   = st_stopped;
		cl->next = st_stopped;
		cl->sess = NULL;

		cl->upt   = -1;
		cl->retry = 0;
		cl->flg   = 0;
		cl->use_ssl = EMS_NO;
		str_init(&cl->nm_addr);
		cl->nm_port = 0;
		cl->lasterr = 0;
		cl->pid     = 0;

		cl->n_conf  = -1;
		cl->n_upt   = -1;
		cl->getconf = 2;

		cl->upt_period = 0;
		cl->getconf_period = 0;
		cl->retry_period = 0;

		mod->ctx = (ems_void *)cl;
	}

	return EMS_OK;
}

static ems_int clnt_uninit(app_module *mod)
{
	ems_client *cl = (ems_client *)mod->ctx;

	if (!cl)
		return EMS_OK;

	mod->ctx = NULL;

	ems_assert(cl->st == st_stopped);
	ems_assert(cl->sess == NULL);
	ems_assert(cl->pid == 0);

	if (cl->sess) {
		ems_session_shutdown_and_destroy(cl->sess);
		cl->sess = NULL;
	}

	cl->st   = st_stopped;
	cl->next = st_stopped;
	cl->getconf = 0;
	str_uninit(&cl->nm_addr);

	if (cl->pid) {
		waitpid(cl->pid, NULL, WNOHANG);
		cl->pid = 0;
	}

	ems_free(cl);

	return EMS_OK;
}

static ems_int clnt_run(app_module *mod, ems_int run)
{
	ems_client *cl = (ems_client *)mod->ctx;

	if (run) {
		if (ems_flag_like(mod->flg, FLG_RUN))
			return EMS_OK;

		ems_flag_set(mod->flg, FLG_RUN);
		cl->flg = 0;
		return cl_change_status(cl, st_init);
	} else {

		if (ems_flag_unlike(mod->flg, FLG_RUN))
			return EMS_OK;

		ems_flag_unset(mod->flg, FLG_RUN);
		return cl_change_status(cl, st_stopped);
	}

	return EMS_OK;
}

static ems_int clnt_ems_status(app_module *mod, json_object *root)
{
	ems_int      st, err=0;
	json_object *rsp = NULL;
	ems_client  *cl = (ems_client *)mod->ctx;

	ems_assert(cl != NULL);
	if (!cl)
		return EMS_ERR;

	rsp = json_object_new_object();

	if (err == 0) {
		switch(cl->st) {
			case st_stopped:
				st = 0; /* stopped */
				break;

			case st_start:
			case st_getdc:
				st = 1; /* connect to server */
				break;

			case st_getconfig:
			case st_getupdatefile:
				st = 3; /* download config files*/
				break;

			case st_normal:
			case st_updatestatus:
			case st_download:
			case st_apply:
				st = 2; /* normal  */
				break;

			case st_connect:
				{
					switch(cl->next) {
					case st_getdc:
						st = 1; /* connect to server */
						break;

					case st_getconfig:
					case st_getupdatefile:
						st = 3; /* download config files*/
						break;

					case st_updatestatus:
					case st_download:
						st = 2; /* normal  */
						break;

					default:
						ems_assert(0 && "should never be here");
						st = 4; /* errors */
						break;
					}
				}
				break;

			default:
				st = 4; /* errors */
				break;
		}
	}

	do {
		if (st == 4) break;

		err = ems_app_process(0, ty_portal, EMS_APP_EVT_STATUS, NULL);
		if (err != 0) {
			st = 4;
			break;
		}

		err = ems_app_process(0, ty_radius, EMS_APP_EVT_STATUS, NULL);
		if (err != 0) {
			st = 4;
			break;
		}
	} while (0);

	json_object_object_add(rsp,  "status", json_object_new_int(st));

	if (st == 4) 
	{
		json_object *obj;
		obj = json_object_new_object();

		if (err == 0) err = cl->lasterr;

		json_object_object_add(obj, "code", json_object_new_int(err));
		json_object_object_add(rsp, "err",  obj);
	}

	json_object_object_add(root, "ems_c", rsp);

	return EMS_OK;
}

static ems_int
clnt_evt_trigger_restart(ems_client *cl, json_object *root)
{
	ems_assert(cl != NULL);

	if (!cl)
		return EMS_OK;

	ems_l_trace("[clnt (%d, %d, %d)]configNumber: %d, updateFileNumber: %d", 
			cl->last, cl->st, cl->next, cl->n_conf, cl->n_upt);

	if (  (cl->n_conf == -1)
	   && ((cl->st == st_err) /* waitting stat for getdc */
	   ||  (cl->st == st_normal) /* waitting stat for getconfig*/ ))
	{
		cl_change_status(cl, st_stopped);
		cl_change_status(cl, st_start);
	}

	return EMS_OK;
}

static ems_int
clnt_process(app_module *mod, ems_uint s, ems_uint d, ems_uint evt, json_object *root)
{
	switch(evt) {
	case EMS_APP_START:
		return clnt_run(mod, EMS_YES);

	case EMS_APP_STOP:
		return clnt_run(mod, EMS_NO);

	case EMS_APP_EMS_STATUS:
		return clnt_ems_status(mod, root);

	default:
		break;
	}

	ems_assert(ems_flag_like(mod->flg, FLG_RUN));

	if (!ems_flag_like(mod->flg, FLG_RUN))
		return EMS_ERR;

	switch(evt) {
	case EMS_APP_EVT_FW_RELOAD:
		return clnt_evt_trigger_restart((ems_client *)mod->ctx, root);

	case EMS_APP_EVT_NETWORK_UP:
		return clnt_evt_trigger_restart((ems_client *)mod->ctx, root);

	default:
		break;
	}


	return EMS_OK;
}

app_module app_client = 
{
	.ty      = ty_client,
	.desc    = ems_string("clnt"),
	.ctx     = NULL,
	.flg     = 0,

	.init    = clnt_init,
	.uninit  = clnt_uninit,
	.run     = clnt_run,
	.process = clnt_process,
	.process_rule  = NULL,
	.version_match = NULL,
	.install       = NULL
};
