
#include "audit.h"
#include "ems_core.h"
#include "ems_client.h"
#include "net.h"


extern net_plugin net_http;

static net_plugin  *gnet_plug[] = {
	&net_http,
//	net_http_url,
	NULL
};

static net_plugin *net_plug_dup(net_plugin *plg)
{
	net_plugin *p = (net_plugin *)ems_malloc(sizeof(net_plugin));

	if (p) {
		memset(p, 0, sizeof(net_plugin));

		p->id    = plg->id;
		p->mount = plg->mount;
		str_set(&p->desc, str_text(&plg->desc));
		p->ctx   = NULL;
		p->init  = plg->init;
		p->process = plg->process;
		p->uninit  = plg->uninit;

		ems_queue_init(&p->entry_port);
		ems_queue_init(&p->entry_post);
		ems_queue_init(&p->entry);
	}

	return p;
}

static ems_void net_plug_destroy(net_plugin *plg)
{
	ems_assert(plg != NULL);
	if (plg) {
		str_uninit(&plg->desc);
		ems_free(plg);
	}
}

ems_int net_load_plugins(ems_queue *list, ems_int mount)
{
	net_plugin *plg;
	ems_int     i;

	/* maybe load from files...*/

	for (i = 0; gnet_plug[i]; i++) {
		plg = gnet_plug[i];

		if (plg->mount == mount) {
			plg = net_plug_dup(gnet_plug[i]);
			if (plg) {
				ems_queue_insert_tail(list, &plg->entry);
				if (plg->init) plg->init(plg);
			}
		}
	}

	return EMS_OK;
}


ems_int net_unload_plugins(ems_queue *list)
{
	ems_queue  *p;
	net_plugin *plg;

	while (!ems_queue_empty(list)) {
		p = ems_queue_head(list);

		plg = ems_container_of(p, net_plugin, entry);
		ems_queue_remove(p);

		if (plg->uninit) plg->uninit(plg);
		net_plug_destroy(plg);
		plg = NULL;
	}

	return EMS_OK;
}

ems_int net_plug_broadcast(ems_queue *list, ems_uint evt, ems_void *arg)
{
	ems_queue  *q = NULL;
	net_plugin *plg;
	ems_int     rtn;

	ems_queue_foreach(list, q) {
		plg = ems_container_of(q, net_plugin, entry);
		rtn = net_plug_sendmsg(plg, evt, arg);
		if (rtn != EMS_OK)
			return rtn;
	}

	return EMS_OK;
}


ems_int net_plug_sendmsg(net_plugin *plg, ems_uint evt, ems_void *arg)
{
	ems_assert(plg && plg->process);

	ems_l_trace("net plug send msg to %s evt: %d", str_text(&plg->desc), evt);
	if (plg && plg->process) {
		return plg->process(plg, evt, arg);
	}

	return EMS_ERR;
}

static net_plugin *net_find_plugin(ems_queue *list, ems_int id)
{
	ems_queue  *q = NULL;
	net_plugin *plg = NULL;

	ems_assert(list != NULL);

	if (!list)
		return NULL;

	ems_queue_foreach(list, q) {
		plg = ems_container_of(q, net_plugin, entry);
		if (plg->id == id)
			break;

		if (!ems_queue_empty(&plg->entry_post)) {
			plg = net_find_plugin(&plg->entry_post, id);
			if (!plg) 
				break;
		}
	}

	return plg;
}

ems_int net_sendmsg(ems_int s, ems_int d, ems_uint evt, ems_void *arg)
{
	ems_l_trace("net, %d --> %d send msg: 0x%x(%d)", s, d, evt, evt);

	return net_plug_sendmsg(net_find_plugin(net_filters(), d), evt, arg);
}
