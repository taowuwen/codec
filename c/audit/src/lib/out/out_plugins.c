
#include "audit.h"
#include "ems_core.h"
#include "ems_client.h"
#include "out.h"


extern out_plugin out_16wifi;
extern out_plugin out_file;

static out_plugin *gout_plug[] = {
	&out_file,
	&out_16wifi,
	NULL
};

static out_plugin *out_plug_dup(out_plugin *plg)
{
	out_plugin *p = (out_plugin *)ems_malloc(sizeof(out_plugin));

	if (p) {
		memset(p, 0, sizeof(out_plugin));

		p->id      = plg->id;
		p->mount   = plg->mount;
		str_set(&p->desc, str_text(&plg->desc));
		p->ctx     = NULL;
		p->init    = plg->init;
		p->process = plg->process;
		p->uninit  = plg->uninit;

		ems_queue_init(&p->entry_post);
		ems_queue_init(&p->entry);
	}

	return p;
}

static ems_void out_plug_destroy(out_plugin *plg)
{
	ems_assert(plg != NULL);
	if (plg) {
		str_uninit(&plg->desc);
		ems_free(plg);
	}
}


ems_int out_load_plugins(ems_queue *list, ems_int mount)
{
	out_plugin *plg;
	ems_int     i;

	/* maybe load from files...*/

	for (i = 0; gout_plug[i]; i++) {
		plg = gout_plug[i];

		if (plg->mount == mount) {
			plg = out_plug_dup(gout_plug[i]);
			if (plg) {
				ems_queue_insert_tail(list, &plg->entry);
				if (plg->init) plg->init(plg);
			}
		}
	}

	return EMS_OK;
}

ems_int out_unload_plugins(ems_queue *list)
{
	ems_queue  *p;
	out_plugin *plg;

	while (!ems_queue_empty(list)) {
		p = ems_queue_head(list);

		plg = ems_container_of(p, out_plugin, entry);
		ems_queue_remove(p);

		if (plg->uninit) plg->uninit(plg);
		out_plug_destroy(plg);
		plg = NULL;
	}

	return EMS_OK;
}


ems_int out_plug_broadcast(ems_queue *list, ems_uint evt, ems_void *arg)
{
	ems_queue  *q = NULL;
	out_plugin *plg;
	ems_int     rtn;

	ems_queue_foreach(list, q) {
		plg = ems_container_of(q, out_plugin, entry);
		rtn = out_plug_sendmsg(plg, evt, arg);
		if (rtn != EMS_OK)
			return rtn;
	}

	return EMS_OK;
}


ems_int out_plug_sendmsg(out_plugin *plg, ems_uint evt, ems_void *arg)
{
	ems_assert(plg && plg->process);

	ems_l_trace("out plug send msg to %s evt: %d", str_text(&plg->desc), evt);
	if (plg && plg->process) {
		return plg->process(plg, evt, arg);
	}

	return EMS_ERR;
}

static out_plugin *out_find_plugin(ems_queue *list, ems_int id)
{
	ems_queue  *q = NULL;
	out_plugin *plg = NULL;

	ems_assert(list != NULL);

	if (!list)
		return NULL;

	ems_queue_foreach(list, q) {
		plg = ems_container_of(q, out_plugin, entry);
		if (plg->id == id)
			break;

		if (!ems_queue_empty(&plg->entry_post)) {
			plg = out_find_plugin(&plg->entry_post, id);
			if (!plg) 
				break;
		}
	}

	return plg;
}

ems_int out_sendmsg(ems_int s, ems_int d, ems_uint evt, ems_void *arg)
{
	ems_l_trace(" out, %d --> %d send msg: 0x%x(%d)", s, d, evt, evt);

	return out_plug_sendmsg(out_find_plugin(out_filters(), d), evt, arg);
}
