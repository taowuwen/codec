
#include "audit.h"
#include "ems_core.h"
#include "ems_client.h"
#include "net_user.h"
#include "net.h"

static ems_queue *_gnet_filter;
static net_core  *_gnet_core;

ems_queue *net_filters()
{
	return _gnet_filter;
}

net_core  *netcorer()
{
	return _gnet_core;
}

static ems_int net_core_init(net_core *net)
{
	ems_cchar *cmd_ip = NULL, *cmd_mac = NULL;
	net_flow  *pkg = &net->pkgs;

#ifdef BOARD_MT7620N
	cmd_ip  = "ip addr show dev $(uci get -P /tmp/state network.wan.ifname) | awk '/inet/{gsub(\"/.*$\",\"\", $2); print $2}'";
	cmd_mac = "ip link show dev $(uci get -P /tmp/state network.wan.ifname) | sed -n '/link\\/ether/{s|.*link/ether ||g;s| brd.*$||g; p}'";
#elif BOARD_AR9344
	cmd_ip  = "ip addr show dev br1 | awk '/inet/{gsub(\"/.*$\",\"\", $2); print $2}'";
	cmd_mac = "ip link show dev br1 | sed -n '/link\\/ether/{s|.*link/ether ||g;s| brd.*$||g; p}'";
#else
	cmd_mac = "ip link | sed -n '/link\\/ether/{s|.*link/ether ||g;s| brd.*$||g; p}' | head -1";
	cmd_ip  = "ip addr show dev $(ip route | grep -v lo | grep scope | awk '{print $3}' | head -1) | sed -n '/inet\\ / {s|.*inet ||g; s|/.*$||g; p }'";
#endif

	memset(net, 0, sizeof(net_core));
	ems_queue_init(&net->pcap);
	ems_queue_init(&net->flt);
	_gnet_filter = &net->flt;

	net->h_port = ems_hash_create(128);
	if (!net->h_port)
		return EMS_ERR;

	net->h_user = ems_hash_create(128);
	if (!net->h_user)
		return EMS_ERR;

	net->st   = st_stop;

	pkg->l2 = pkg->l3 = pkg->l4 = pkg->l5 = NULL;
	str_init(&pkg->gwip);
	str_init(&pkg->apmac);
	pkg->hdr    = NULL;
	pkg->packet = NULL;
	pkg->user   = NULL;

	str_set(&pkg->gwip,  ems_popen_get(cmd_ip));
	str_set(&pkg->apmac, ems_popen_get(cmd_mac));

	ems_timeout_init(&net->to);
	net->to_period = 60; /* one second */

	return EMS_OK;
}

static ems_void net_core_uninit(net_core *net)
{
	ems_assert(net && net->st == st_stop);
	ems_assert(ems_queue_empty(&net->flt));
	ems_assert(ems_queue_empty(&net->pcap));
	_gnet_filter = NULL;

	if (net->h_port) {
		ems_hash_destroy(net->h_port);
		net->h_port = NULL;
	}

	if (net->h_user) {
		net_user_remove_all(net->h_user);
		ems_hash_destroy(net->h_user);
		net->h_user = NULL;
	}

	{
		net_flow  *pkg = &net->pkgs;
		str_uninit(&pkg->gwip);
		str_uninit(&pkg->apmac);
		pkg->l2 = pkg->l3 = pkg->l4 = pkg->l5 = NULL;
		pkg->hdr    = NULL;
		pkg->packet = NULL;
		pkg->user   = NULL;
	}
}

static ems_uint net_id() 
{
	return mod_net;
}

static ems_uint net_type() 
{
	return MODULE_TYPE_IN;
}

static ems_cchar *net_nick() 
{
	return "net";
}

static ems_int net_init(audit_class *cls) 
{
	net_core  *net = NULL;
	ems_l_trace("net do init..., and all the sub module's init");

	net = (net_core *)ems_malloc(sizeof(net_core));

	if (!net)
		return EMS_ERR;

	if (net_core_init(net) != EMS_OK) {
		net_core_uninit(net);
		ems_free(net);
		return EMS_ERR;
	}

	_gnet_core = net;
	cls->ctx = (ems_void *)net;

	return EMS_OK;
}

static ems_int net_core_run(audit_class *cls , ems_int run)
{
	net_core *net = (net_core *)cls->ctx;

	if (run) {
		if (ems_flag_like(cls->flg, FLG_RUN)) 
			return EMS_OK;

		if (net_change_status(net, st_start) != EMS_OK)
			return EMS_ERR;

		ems_flag_set(cls->flg, FLG_RUN);
	} else {
		if (ems_flag_unlike(cls->flg, FLG_RUN))
			return EMS_OK;

		net_change_status(net, st_stop);
		ems_flag_unset(cls->flg, FLG_RUN);
	}

	return EMS_OK;
}


static ems_int net_evt_userinfo(net_core *net, json_object *req)
{
	return net_user_flush_user(net->h_user, req);
}

static ems_int net_evt_setuser(net_core *net, json_object *req)
{
	return net_user_setuser(net->h_user, req);
}

static ems_int net_evt_deluser(net_core *net, json_object *req)
{
	return net_user_deluser(net->h_user, req);
}

static ems_int net_evt_module_ctrl(audit_class *cls, json_object *req)
{
	json_object *root, *obj;
	ems_int  enable;

	ems_assert(req && json_object_is_type(req, json_type_object));

	root = json_object_object_get(req, "net");
	do {
		obj = json_object_object_get(root, "enable");
		if (obj) {
			enable = json_object_get_int(obj);
#ifdef BOARD_AR9344
			/* force it do a restart */
			net_core_run(cls, EMS_NO);
#endif
			net_core_run(cls, enable);
		}

		/* send msg to rest of net filters, not done for now */
	} while (0);

	return EMS_OK;
}

static ems_int net_evt_module_info(audit_class *cls, json_object *req)
{
	json_object *root;

	root = json_object_new_object();
	if (!root)
		return EMS_ERR;
	{
		ems_int enable = ems_flag_like(cls->flg, FLG_RUN)?1:0 ;

		json_object_object_add(root, "enable", json_object_new_int(enable));
	}

	json_object_object_add(req, "net", root);

	/* send msg to rest of net filters, not done for now */

	return EMS_OK;
}

static ems_int net_evt_module_set(audit_class *cls, json_object *req)
{
	return EMS_OK;
}

static ems_int net_evt_module_get(audit_class *cls, json_object *req)
{
	return EMS_OK;
}

static ems_int net_evt_network_reload(net_core *net, json_object *req)
{
	net_load_all_netinf(net);
	return EMS_OK;
}

static ems_int net_process(audit_class *cls, ems_uint evt, ems_uchar *arg) 
{
	net_core *net = (net_core *)cls->ctx;

	ems_l_trace("net(%s), got msg 0x%x(%d)", 
			ems_flag_like(cls->flg, FLG_RUN)?"running":"stopped",
			evt, evt);

	switch(evt) {
	case A_AUDIT_START:
		return net_core_run(cls, EMS_YES);

	case A_AUDIT_STOP:
		return net_core_run(cls, EMS_NO);

	case MSG_MODULE_CTRL:
		return net_evt_module_ctrl(cls, (json_object *)arg);

	case MSG_MODULE_INFO:
		return net_evt_module_info(cls, (json_object *)arg);

	case MSG_MODULE_SET:
		return net_evt_module_set(cls, (json_object *)arg);

	case MSG_MODULE_GET:
		return net_evt_module_get(cls, (json_object *)arg);

	default:
		break;
	}

	if (ems_flag_unlike(cls->flg, FLG_RUN))
		return EMS_ERR;

	switch(evt) {
	case MSG_USERINFO:
		return net_evt_userinfo(net, (json_object *)arg);

	case MSG_SETUSER:
		return net_evt_setuser(net, (json_object *)arg);

	case MSG_DELUSER:
		return net_evt_deluser(net, (json_object *)arg);

	case MSG_NETWORK_RELOAD:
		return net_evt_network_reload(net, (json_object *)arg);

	default:
		break;
	}

	return EMS_OK;
}

static ems_int net_uninit(audit_class *cls) 
{
	net_core *net = (net_core *)cls->ctx;
	ems_l_trace("net do uninit.., and all the submodule's uninit");

	_gnet_core = NULL;
	if (net) {
		net_core_uninit(net);
		ems_free(net);
	}
	cls->ctx = NULL;

	return EMS_OK;
}

audit_class c_net={
	.id   = net_id,
	.type = net_type, 
	.nick = net_nick,
	.init = net_init,
	.process = net_process,
	.uninit  = net_uninit,
	.ctx  = NULL
};
