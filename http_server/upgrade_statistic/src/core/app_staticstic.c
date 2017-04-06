
#include "ems_core.h"
#include "ems_client.h"
#include "app.h"
#include "ems_statistic.h"
#include "ems_cmd.h"

static ems_void stat_timeout_cb(ems_timeout *timeout)
{
	ems_queue  *p;
	st_upgrade *upg = ems_container_of(timeout, st_upgrade, to);
	st_device  *dev = upg->dev;

	ems_assert(upg && dev && dev->st);

	ems_l_trace("sn:(%s, %s, %s)@%s timeout", 
			str_text(&dev->sn),
			str_text(&upg->ty),
			str_text(&upg->ver),
			str_text(&upg->status),
			str_text(&upg->ip));

	ems_flag_unset(upg->flg, EMS_FLG_ONLINE);
	ems_assert(dev->st != NULL);
	ems_assert(ems_flag_like(dev->flg, EMS_FLG_ONLINE));

	/*
	   set device status maybe, not now
	 */
	ems_queue_foreach(&dev->entry_upg, p) {
		upg = ems_container_of(p, st_upgrade, entry);

		if (ems_flag_like(upg->flg, EMS_FLG_ONLINE))
			return;
	}

	ems_flag_unset(dev->flg, EMS_FLG_ONLINE);
	ems_queue_remove(&dev->entry);
	ems_queue_insert_head(&dev->st->offline, &dev->entry);
	
	return;
}

static ems_int stat_timeout_set(st_upgrade *upg, ems_int msecs)
{
	return ems_timeout_set(timeouter(), &upg->to,  msecs, stat_timeout_cb, EMS_TIMEOUT_SORT);
}

static st_upgrade *stat_upgrade_new()
{
	st_upgrade *upg = NULL;

	upg = (st_upgrade *)ems_malloc(sizeof(st_upgrade));

	if (upg) {
		memset(upg, 0, sizeof(st_upgrade));

		str_init(&upg->ip);
		str_init(&upg->ty);
		str_init(&upg->ver);
		str_init(&upg->status);

		gettimeofday(&upg->create, NULL);
		memcpy(&upg->access, &upg->create, sizeof(upg->access));

		ems_timeout_init(&upg->to);

		upg->flg = 0;
		upg->dev = NULL;

		ems_queue_init(&upg->entry);
	}

	return upg;
}


static st_device *stat_device_new()
{
	st_device *dev = NULL;

	dev = (st_device *)ems_malloc(sizeof(st_device));
	if (dev) {
		memset(dev, 0, sizeof(st_device));

		str_init(&dev->sn);
		dev->flg = 0;
		dev->st  = NULL;

		ems_queue_init(&dev->entry_upg);
		ems_queue_init(&dev->entry);
	}

	return dev;
}


static ems_void st_upgrade_destroy(st_upgrade *upg)
{
	ems_assert(upg);

	if (upg) {
		str_uninit(&upg->ip);
		str_uninit(&upg->ty);
		str_uninit(&upg->ver);
		str_uninit(&upg->status);

		ems_timeout_cancel(&upg->to);
		upg->dev = NULL;

		ems_free(upg);
	}
}

static ems_void st_device_destroy(st_device *dev)
{
	ems_assert(dev);
	if (dev) {

		ems_queue_clear(&dev->entry_upg, st_upgrade, entry, st_upgrade_destroy);

		str_uninit(&dev->sn);
		dev->st  = NULL;
		dev->flg = 0;
		ems_free(dev);
	}
}

static ems_int stat_init(app_module *mod)
{
	st_statistic *st = NULL;

	st = (st_statistic *)ems_malloc(sizeof(st_statistic));
	if (!st)
		return EMS_ERR;

	ems_queue_init(&st->online);
	ems_queue_init(&st->offline);

	mod->ctx = (ems_void *)st;

	return EMS_OK;
}

static ems_int stat_uninit(app_module *mod)
{
	st_statistic *st = mod->ctx;

	if (st) {
		ems_assert(ems_queue_empty(&st->online));
		ems_assert(ems_queue_empty(&st->offline));

		ems_queue_clear(&st->online,  st_device, entry, st_device_destroy);
		ems_queue_clear(&st->offline, st_device, entry, st_device_destroy);

		ems_free(st);
	}
	mod->ctx = NULL;

	return EMS_OK;
}

static ems_int stat_ems_run(app_module *mod, ems_int run)
{
	st_statistic *st = (st_statistic *)mod->ctx;

	if (!run) {
		ems_queue_clear(&st->online,  st_device, entry, st_device_destroy);
		ems_queue_clear(&st->offline, st_device, entry, st_device_destroy);
	}

	return EMS_OK;
}

static ems_int stat_run(app_module *mod, ems_int run)
{
	if (run) {
		if (ems_flag_like(mod->flg, FLG_RUN))
			return EMS_OK;

		ems_l_trace("statistic running here");
		ems_flag_set(mod->flg, FLG_RUN);

		stat_ems_run(mod, EMS_YES);
	} else {
		if (ems_flag_unlike(mod->flg, FLG_RUN))
			return EMS_OK;

		ems_l_trace("statistic stopped");
		ems_flag_unset(mod->flg, FLG_RUN);

		stat_ems_run(mod, EMS_NO);
	}

	return EMS_OK;
}

/*

   	{"sn": sn, "flag": flg, "upgs": 
		[
			{ "ip": ip, "ty": rom type, "ver": "rom version", "status":"status",
			  "create": create time, "access": "access", "flag": flg },
			  ...
		] 
	},

*/

static ems_int stat_dump_device_info(ems_queue *list, json_object *ary)
{
	ems_queue   *p    = NULL;
	st_upgrade  *upg  = NULL;
	json_object *jupg = NULL;

	ems_queue_foreach(list, p) {

		upg = ems_container_of(p, st_upgrade, entry);
		ems_assert(upg != NULL);

		jupg = json_object_new_object();

		if (!jupg)
			continue;

		json_object_object_add(jupg, "ip",     json_object_new_string(str_text(&upg->ip)));
		json_object_object_add(jupg, "ty",     json_object_new_string(str_text(&upg->ty)));
		json_object_object_add(jupg, "ver",    json_object_new_string(str_text(&upg->ver)));
		json_object_object_add(jupg, "status", json_object_new_string(str_text(&upg->status)));
		json_object_object_add(jupg, "create", json_object_new_int64(upg->create.tv_sec));
		json_object_object_add(jupg, "access", json_object_new_int64(upg->access.tv_sec));
		json_object_object_add(jupg, "flag",   json_object_new_int(upg->flg));

		json_object_array_add(ary, jupg);
	}

	return EMS_OK;
}

static ems_int stat_dump_info(ems_queue *list, json_object *ary)
{
	ems_queue   *p    = NULL;
	st_device   *dev  = NULL;
	json_object *jdev = NULL, *jary = NULL;

	ems_queue_foreach(list, p) {

		dev = ems_container_of(p, st_device, entry);
		ems_assert(dev != NULL);

		jdev = json_object_new_object();
		if (!jdev)
			continue;

		json_object_object_add(jdev, "sn",   json_object_new_string(str_text(&dev->sn)));
		json_object_object_add(jdev, "flag", json_object_new_int(dev->flg));

		jary = json_object_new_array();
		if (!jary) {
			json_object_put(jdev);
			jdev = NULL;
			continue;
		}

		stat_dump_device_info(&dev->entry_upg, jary);

		json_object_object_add(jdev, "upgs", jary);

		json_object_array_add(ary, jdev);
	}

	return EMS_OK;
}

static ems_int stat_statistic(st_statistic *st, json_object *root)
{
	json_object *ary = NULL;

	ary = json_object_new_array();

	stat_dump_info(&st->online, ary);
	stat_dump_info(&st->offline, ary);

	json_object_object_add(root, "device", ary);

	return EMS_OK;
}

static ems_int stat_total_info(st_statistic *st, json_object *root)
{
	ems_int len_offline, len_online, total;

	ems_queue_len(&st->offline, len_offline);
	ems_queue_len(&st->online,  len_online);

	total = len_offline + len_online;

	json_object_object_add(root, "total",   json_object_new_int(total));
	json_object_object_add(root, "online",  json_object_new_int(len_online));
	json_object_object_add(root, "offline", json_object_new_int(len_offline));

	ems_l_trace("total devices: %d", len_offline + len_online);

	return EMS_OK;
}

static st_upgrade *stat_device_new_upgrade(st_device *dev)
{
	st_upgrade *upg = NULL;

	ems_assert(dev);

	upg = stat_upgrade_new();
	if (upg) {
		upg->dev = dev;
		ems_queue_insert_head(&dev->entry_upg, &upg->entry);
	}

	return upg;
}

static ems_int stat_update_status(st_upgrade *upg, ems_uint evt)
{
	st_device    *dev;
	st_statistic *st;

	ems_assert(upg->dev && upg->dev->st);

	dev = upg->dev;
	st  = dev->st;

	switch(evt) {
	case CMD_GET_DC:
		str_set(&upg->status, "getdc");
		stat_timeout_set(upg, 5000);
		gettimeofday(&upg->access, NULL);
		break;

	case CMD_GET_CONF:
		str_set(&upg->status, "getconf");
		stat_timeout_set(upg, 5000);
		gettimeofday(&upg->access, NULL);
		break;

	case CMD_GET_UPDATEFILE:
		str_set(&upg->status, "get_updatefile");
		stat_timeout_set(upg, 65000); /*65s*/
		gettimeofday(&upg->access, NULL);
		break;

	case CMD_UPDATESTATUS:
		str_set(&upg->status, "updatestatus");
		stat_timeout_set(upg, 120000); /* 2mins */
		gettimeofday(&upg->access, NULL);
		break;

	case CMD_DOWNLOAD:
		str_set(&upg->status, "download/updating");
		stat_timeout_set(upg, 120000); /* 2mins */
		gettimeofday(&upg->access, NULL);
		ems_flag_set(upg->flg, FLG_STAT_DOWNLOAD);
		break;

	default:
		ems_assert(0 && "never be here");
		str_set(&upg->status, "unkwon");
		break;
	}

	if (ems_flag_unlike(dev->flg,  EMS_FLG_ONLINE)) {
		ems_queue_remove(&dev->entry);
		ems_queue_insert_head(&st->online, &dev->entry);
		ems_flag_set(dev->flg, EMS_FLG_ONLINE);
	}

	ems_flag_set(upg->flg, EMS_FLG_ONLINE);

	return EMS_OK;
}

static st_device *stat_find_dev_on(ems_queue *list, ems_cchar *sn)
{
	ems_queue *p;
	st_device *dev;

	ems_queue_foreach(list, p) {
		dev = ems_container_of(p, st_device, entry);

		ems_assert(str_text(&dev->sn));

		if (!strcmp(str_text(&dev->sn), sn))
			return dev;
	}

	return NULL;
}

static st_device *stat_find_device(st_statistic *st, ems_cchar *sn)
{
	st_device *dev = NULL;

	dev = stat_find_dev_on(&st->online, sn);

	if (!dev)
		dev = stat_find_dev_on(&st->offline, sn);

	return dev;
}


static st_upgrade *stat_find_upg(st_device *dev, ems_cchar *ip)
{
	ems_queue  *p;
	st_upgrade *upg = NULL;


	ems_queue_foreach(&dev->entry_upg, p) {
		upg = ems_container_of(p, st_upgrade, entry);

		ems_assert(str_text(&upg->ip));

		if (!strcmp(str_text(&upg->ip), ip))
			return upg;
	}

	return NULL;
}

/*
devicesn:      ssssss
devicetype:    type
deviceversion: version
deviceipaddr:  ipaddr
 */
static ems_int stat_updatestatus(st_statistic *st, json_object *root, ems_uint evt)
{
	st_device  *dev = NULL;
	st_upgrade *upg = NULL;
	ems_int ret;
	ems_str sn, ty, ver, ip;

	str_init(&sn); str_init(&ty); str_init(&ver); str_init(&ip);
	do {
		ret = EMS_ERR;
		ems_json_get_string_def(root, "devicesn",      &sn, NULL); 
		if (str_len(&sn) <= 0) break;

		ems_json_get_string_def(root, "devicetype",    &ty, NULL);
		if (str_len(&ty) <= 0) break;

		ems_json_get_string_def(root, "deviceversion", &ver,NULL);
		if (str_len(&ver) <= 0) break;

		ems_json_get_string_def(root, "deviceipaddr",  &ip, NULL);
		if (str_len(&ip) <= 0) break;

		dev = stat_find_device(st, str_text(&sn));

		if (!dev) {
			dev = stat_device_new();
			if (!dev) break;

			str_cpy(&dev->sn, &sn);
			dev->st = st;

			ems_l_trace("new device in: %s", str_text(&dev->sn));
			ems_queue_insert_head(&st->offline, &dev->entry);
		}

		upg = stat_find_upg(dev, str_text(&ip));
		if (!upg) {
			upg = stat_device_new_upgrade(dev);
			if (!upg) break;
		}

		str_cpy(&upg->ip, &ip);
		str_cpy(&upg->ty, &ty);
		str_cpy(&upg->ver, &ver);

		ret = stat_update_status(upg, evt);
	} while (0);
	str_uninit(&sn); str_uninit(&ty); str_uninit(&ver); str_uninit(&ip);

	return ret;
}

/*
deviceipaddr: ipaddr
 */
static ems_int stat_download(st_statistic *st, json_object *root)
{
	ems_queue  *p;
	st_device  *dev;
	st_upgrade *upg;

	ems_str ip;

	str_init(&ip);
	do {
		ems_json_get_string_def(root, "deviceipaddr",  &ip, NULL);
		if (str_len(&ip) <= 0) break;

		ems_queue_foreach(&st->online, p) {
			dev = ems_container_of(p, st_device, entry);

			upg = stat_find_upg(dev, str_text(&ip));

			if (upg) {
				stat_update_status(upg, CMD_DOWNLOAD);
				break;
			}
		}

	} while (0);
	str_uninit(&ip);
	return EMS_OK;
}

static ems_int
stat_process(app_module *mod, ems_uint s, ems_uint d, ems_uint evt, json_object *root)
{
	st_statistic *st = (st_statistic *)mod->ctx;
	ems_assert(st != NULL);

	ems_l_trace("statistic evt: 0x%x, from: 0x%x, args: %s", 
			evt, s, root?json_object_to_json_string(root):"");

	switch(evt) {
	case EMS_APP_START:
		return stat_run(mod, EMS_YES);

	case EMS_APP_STOP:
		return stat_run(mod, EMS_NO);

	default:
		break;
	}

	if (ems_flag_unlike(mod->flg, FLG_RUN)) {
		ems_l_trace("statistic link not running");
		return EMS_OK;
	}

	switch(evt) {

	case CMD_STATICSTIC:
		return stat_statistic(st, root);

	case CMD_GET_DC:
	case CMD_GET_CONF:
	case CMD_GET_UPDATEFILE:
	case CMD_UPDATESTATUS:
		return stat_updatestatus(st, root, evt);

	case CMD_DOWNLOAD:
		return stat_download(st, root);

	case CMD_TOTAL_INFO:
		return stat_total_info(st, root);

	default:
		break;
	}

	return EMS_OK;
}


app_module app_statistic = 
{
	.ty      = ty_statistic,
	.desc    = ems_string("statistic"),
	.ctx     = NULL,
	.flg     = 0,

	.init    = stat_init,
	.uninit  = stat_uninit,
	.run     = stat_run,
	.process = stat_process,
	.process_rule  = NULL,
	.version_match = NULL,
	.install       = NULL
};
