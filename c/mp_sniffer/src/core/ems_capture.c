
#include "ems_core.h"
/*


   start----> paused ----> running ---->stopped
               ^              |
	       |              |
	       +--------------+
 */


static struct _cap_memory_map {
	off_t high;
	off_t low;
	off_t rsrv;
	off_t max; /* max sigle file size*/
} _gcapmem[] = {
	{131072, 102400, 20480, 10240}, /* 128M, rsrv 20M,  */
	{262144, 245760, 20480, 10240}, /* 256M, rsrv 20M */
	{32768,  28672,  2048, 1024}   /* 32M,  rsrv 2M  */
};

/*
	system total memory: 
	    >>> free | awk  '/\<Mem\>/{print $2}'
	memory free: 
	    >>> free | awk '/\<cache\>/{print $3}'

	for 128M: reserved 15M ~ 20M
	for 256M: reserved 20M ~ 25M 
	for 32M:  reserved 2M 
 */
static ems_int capture_memory_info(ems_capture *cap)
{
	ems_int i, sz;
	struct _cap_memory_map  *map;
	off_t total;

#ifndef GENERIC_LINUX
	total = ems_atoll(ems_popen_get("free | awk '/\\<Mem\\>/{print $2}'"));
#else
	total = ems_atoll(ems_popen_get(
			"free | awk '/\\<Mem\\>:/{print $2*1024}'"));
#endif
	
	sz = sizeof(_gcapmem) / sizeof(struct _cap_memory_map);

	cap->mem_rsrv = 2048; /* 2M*/
	cap->mem_maxsingle = 2048;

	for (i = 0; i < sz; i++) {
		map = &_gcapmem[i];

		if (map->high >= total && map->low <= total)  {
			cap->mem_maxsingle = map->max;
			cap->mem_rsrv      = map->rsrv;
			break;
		}
	}

#ifndef GENERIC_LINUX
	cap->mem_total = 
		ems_atoll(ems_popen_get(
			"free | awk '/\\<buffers\\>.*:/{print $4}'"));
#else
	cap->mem_total = 
		ems_atoll(ems_popen_get(
			"free | awk '/\\<Mem\\>:/{print ($4+$6)*1024}'"));
#endif
	cap->mem_total -= cap->mem_rsrv;

	ems_l_trace("total: %lldk, mem_total: %lldk, mem_rsrv: %lldk, "
			"max single file size: %lldk", 
		total, cap->mem_total, cap->mem_rsrv, cap->mem_maxsingle);

	/* update total into bytes */
	cap->mem_total *= 1024;
	cap->mem_maxsingle *= 1024;

	return EMS_OK;
}

static ems_int capture_status_into_start(ems_capture *capture)
{
	capture_memory_info(capture);
	capture->max_capture_number = 3;
	capture->n_capture   = 0;

#ifdef BOARD_AR9344 
	str_set(&capture->mac, ems_popen_get(
		"ip -o link show dev br1 |"
		" awk '{mac=$(NF-2); gsub(\":\", \"\", mac); print mac}'"));
#else
	/* GENERIC_LINUX */
	str_set(&capture->mac,
		ems_popen_get(
		"ip -o link show dev %s |"
		" awk '{mac=$(NF-2); gsub(\":\", \"\", mac); print mac}'",

			/* get default route  interface */
			ems_popen_get(
			"ip route | awk '/default/{for (i =0; i < NF; i++)"
			"{if($i == \"dev\") {print $(i+1); exit}}}'"
			)
		)
	);
#endif

	/*
	   do not call capture_change_status(capture, st_paused),
	   for that we need not send apcfg module evt "all stopped"
	 */
	capture->st = st_paused;
	return EMS_OK;
}

/*
   reset total memory could be used
   send apcfg event: all the capturing interface are stopped
 */
static ems_int capture_status_into_paused(ems_capture *capture)
{
	ems_assert(capture->_core->ctrl != NULL);
	ems_assert(ems_queue_empty(&capture->list_capture_inf));

	if (capture->type == CAPTURE_TYPE_RADIO) {
		if (ems_flag_like(capture->_core->flg, FLG_MONITOR_MODE)) {
#ifdef BOARD_AR9344 
			ems_systemcmd("/usr/sbin/sniffer_setapmonitor stop");
#else
			ems_systemcmd("sniffer_setapmonitor stop"); 
#endif

			ctrl_send_event_to_apcfg(capture->_core->ctrl, 
				core_build_apcfg_event(APCFG_MSG_ENV_RESTORE, 
					NULL, 0, NULL
				)
			);
			ems_flag_unset(capture->_core->flg,  FLG_MONITOR_MODE);
		}
	}

	ems_l_trace("[capture] send apcfg all stopped evt goes from here");
	ctrl_send_event_to_apcfg(capture->_core->ctrl, 
			core_build_apcfg_event(APCFG_MSG_STOP, NULL, 0, NULL));

	return EMS_OK;
}

/*
   1. get currently total memory could be used
   2. prepare env
       mkdir -p /tmp/sniffer
       update env
   3. alloc memory for capture node and start it
 */
static ems_int capture_status_into_running(ems_capture *capture)
{
	/* memory */
	capture->mem_left = capture->mem_total;

	ems_l_warn("mem->total: %lld, mem_left: %lld", 
			capture->mem_total, capture->mem_left);

	if (capture->mem_left <= 0) {
		capture_change_status(capture, st_paused);
		return ERR_EVT_NO_SPACE_LEFT;
	}

	if (capture->type == CAPTURE_TYPE_RADIO) {
#ifdef BOARD_AR9344 
		ems_systemcmd("/usr/sbin/sniffer_setapmonitor start"); 
#else
		ems_systemcmd("sniffer_setapmonitor start"); 
#endif
		ems_flag_set(capture->_core->flg,  FLG_MONITOR_MODE);
	}

	rm_rf(SNIFFER_WORKING_DIR);
	mkdir_p(SNIFFER_WORKING_DIR);

	ctrl_send_event_to_apcfg(capture->_core->ctrl, 
		core_build_apcfg_event(APCFG_MSG_START, NULL, 0, NULL));

	return EMS_OK;
}

/*
   stop all the captures 
   change all the captures into stopped status
 */
static ems_int capture_status_into_stop(ems_capture *capture)
{
	ems_queue      *p;
	ems_do_capture *cap;

	while(!ems_queue_empty(&capture->list_capture_inf)) {
		p = ems_queue_head(&capture->list_capture_inf);
		cap = ems_container_of(p, ems_do_capture, entry);

		ems_l_trace("[capture] stopping: %s", str_text(&cap->inf));
		cap_change_status(cap, st_stopped);
	}

	if (capture->type == CAPTURE_TYPE_RADIO) {
		if (ems_flag_like(capture->_core->flg, FLG_MONITOR_MODE)) {
#ifdef BOARD_AR9344 
			ems_systemcmd("/usr/sbin/sniffer_setapmonitor stop");
#else
			ems_systemcmd("sniffer_setapmonitor stop"); 
#endif
			/*
			   send evt restore env 
			 */
			ems_flag_unset(capture->_core->flg,  FLG_MONITOR_MODE);
		}
	}

	rm_rf(SNIFFER_WORKING_DIR);

	return EMS_OK;
}


ems_int capture_change_status(ems_capture *capture, ems_status st)
{
	ems_l_trace("[capture] change status from %s into %s", 
			ems_status_str(capture->st), ems_status_str(st));

	if (capture->st == st)
		return EMS_OK;

	capture->st = st;

	switch(st) {
	case st_start:
		return capture_status_into_start(capture);

	case st_paused:
		return capture_status_into_paused(capture);

	case st_running:
		return capture_status_into_running(capture);

	case st_stopped:
		return capture_status_into_stop(capture);

	default:
		break;
	}

	return EMS_ERR;
}


static ems_int 
capture_get_channel_lists(ems_do_capture *cap, json_object *req)
{
	ems_int        i;
	ems_channel   *ch;
	json_object   *jchannel, *jobj;

	jchannel = json_object_object_get(req, "channel");
	if (!jchannel) /* did not configure channel list, use default */
		return EMS_OK;

	ems_assert(json_object_is_type(jchannel, json_type_array));
	if (!json_object_is_type(jchannel, json_type_array))
		return EMS_ERR;

	for (i = 0; i < json_object_array_length(jchannel); i++) {

		jobj =  json_object_array_get_idx(jchannel, i);
		
		if (!jobj)
			continue;

		ch = ems_channel_new();
		if (!ch)
			continue;

		ch->channel = json_object_get_int(jobj);
		ems_queue_insert_tail(&cap->channel_lists, &ch->entry);
	}

	
	return EMS_OK;
}

/*
 * ftp://[user[:password]@]host:port/path
 * */
static ems_int capture_parse_server_host_ftp(ems_do_capture *cap)
{
	ems_char *p, *q;
	ems_buffer *buf = core_buffer();

	if (str_len(&cap->server.ftp.url) <= 0)
		return EMS_ERR;

	p = buf_wr(buf);

	snprintf(p, buf_left(buf), "%s", str_text(&cap->server.ftp.url));

#define SLASH	"/"
#define FTP_HEADER	"ftp:" SLASH SLASH
	if (strncmp(p, FTP_HEADER, strlen(FTP_HEADER)))
		return EMS_ERR;

	p += strlen(FTP_HEADER);

	q = strchr(p, '@');
	if (q != NULL)
		p = q + 1;

	/*now, p point to host */
	q = strchr(p, '/');
	if (!q)
		q = p + strlen(p);

	*q = '\0';

	ems_l_trace("host and port: %s", p);

	q = strchr(p, ':');
	if (q)
		*q = '\0';

	ems_l_trace("host: %s", p);

	str_set(&cap->server.host, p);

	return EMS_OK;
}

/*
   find this capture already started or not
 */
ems_int capture_start_capturing(ems_do_capture *cap, json_object *req)
{
	json_object   *jserver;

	ems_assert(cap && req);

	ems_json_get_int64_def(req,  "limit_filesize", cap->limit_filesize, 0);
	ems_json_get_int64_def(req,  "limit_time",     cap->limit_timeuse, 0);
	ems_json_get_int64_def(req,  "limit_package",  cap->limit_pkg, 0);
	ems_json_get_string_def(req, "filter",        &cap->pcap.filter, NULL);

	jserver = json_object_object_get(req, "server");
	if (!(jserver && json_object_is_type(jserver, json_type_object))) {
		ems_l_warn("server missing for: %s", str_text(&cap->inf));
		return ERR_EVT_SERVER_MISSING;
	}

	ems_json_get_int_def(jserver, "type", cap->server.type, -1);
	ems_json_get_int_def(jserver, "data_compress", cap->server.compress, 0);

	switch(cap->server.type) {

	case SERVER_TYPE_FTP:

		ems_json_get_string_def(jserver, "ftp_server", 
				&cap->server.ftp.url, NULL);

		if (capture_parse_server_host_ftp(cap) != EMS_OK) {
			ems_l_warn("invalid ftp url: %s", 
					str_text(&cap->server.ftp.url));
			return ERR_EVT_SERVER_FTP_HOST;
		}

		ems_l_trace("server is ftp(%s): %s",
				str_text(&cap->server.host),
				str_text(&cap->server.ftp.url));
		break;

	default:
		ems_l_trace("unknown server type, not support: %d", cap->type);
		break;
	}

	ems_l_trace("limit( size: %lld, time: %lld, pkg: %lld, filter: %s",
		cap->limit_filesize, 
		cap->limit_timeuse, 
		cap->limit_pkg, 
		str_text(&cap->pcap.filter));


	switch(cap->type) {
	case CAPTURE_TYPE_RADIO:
		capture_get_channel_lists(cap, req);
		break;

	default:
		break;
	}

	(ems_void)cap_change_status(cap, st_start);

	return EMS_OK;
}

ems_do_capture *
capture_get_cap(ems_capture *capture, ems_cchar *inf)
{
	ems_queue      *p = NULL;
	ems_do_capture *cap = NULL;

	ems_queue_foreach(&capture->list_capture_inf, p) {

		cap = ems_container_of(p, ems_do_capture, entry);

		ems_assert(str_text(&cap->inf) != NULL);
		if (!strcmp(str_text(&cap->inf), inf))
			return cap;
	}

	return NULL;
}

ems_int capture_handle_msg_start(ems_capture *capture, json_object *req)
{
	ems_int errcode;
	ems_int type, wlanid, radioid;
	ems_str inf;
	ems_do_capture  *cap = NULL;

	str_init(&inf);

	ems_json_get_int_def(req, "type",   type, 0);
	ems_json_get_int_def(req, "wlanid",  wlanid, 0);
	ems_json_get_int_def(req, "radioid", radioid, 0);
	ems_json_get_string_def(req, "interface", &inf, NULL);

	ems_assert(type >= 1 && type <= 3);
	ems_assert(str_len(&inf) > 0);

	if (type <= 0 || type > 3) {
		errcode = ERR_EVT_TYPE_INVALID;
		goto err_out;
	}

	if (str_len(&inf) <= 0) {
		errcode = ERR_EVT_INTERFACE_UNKNOWN;
		goto err_out;
	}

	if (capture->st == st_paused) {
		ems_assert(capture->n_capture == 0);

		capture->type = type;

		errcode = capture_change_status(capture, st_running);
		if (errcode != EMS_OK)
			goto err_out;
	} else {
		if (capture->n_capture >= capture->max_capture_number) {
			errcode = ERR_EVT_OVER_MAX_CAPTURE;
			goto err_out;
		}

	/*
	accepted rule: 
		while radio capturing, we deny other type of capture
		while others, we deny radio capture
	 */
		if ( (capture->type == CAPTURE_TYPE_RADIO &&
			       type != CAPTURE_TYPE_RADIO) 
		     ||
			(capture->type != CAPTURE_TYPE_RADIO && 
				  type == CAPTURE_TYPE_RADIO))
		{
			errcode = ERR_EVT_TYPE_CONFILICT;
			goto err_out;
		}


		if (capture_get_cap(capture, str_text(&inf)) != NULL) {
			errcode = ERR_EVT_CAPTURE_BUSY;
			goto err_out;
		}
	}

	{
		cap = ems_do_capture_new();

		if (!cap) {
			errcode = ERR_EVT_MEMORY_ERROR;
			goto err_out;
		}

		ems_queue_insert_tail(&capture->list_capture_inf, &cap->entry);

		cap->wlanid  = wlanid;
		cap->radioid = radioid;
		cap->type    = type;
		str_cpy(&cap->inf, &inf);
		cap->max_filesize = capture->mem_maxsingle;

		cap->capture = capture;
		errcode = capture_start_capturing(cap, req);

		if (errcode != EMS_OK) {
			ems_queue_remove(&cap->entry);
			ems_do_capture_destroy(cap);
			cap = NULL;
		}
	}


	if (errcode == EMS_OK) {
		str_uninit(&inf);
		return EMS_OK;
	}

	/* send capturing start in "doing capture module " not here */
err_out:
	ctrl_send_event_to_wtp(capture->_core->ctrl, 
			core_build_wtp_event(WTP_EVT_STOP,
				type, wlanid, radioid, str_text(&inf), 
				errcode, NULL 
			)
		);
	str_uninit(&inf);

	return EMS_OK;
}

ems_int capture_handle_msg_stop(ems_capture *capture, json_object *req)
{
	ems_int errcode;
	ems_int type, wlanid, radioid;
	ems_str inf;
	ems_do_capture  *cap = NULL;

	str_init(&inf);

	ems_json_get_int_def(req, "type",   type, 0);
	ems_json_get_int_def(req, "wlanid",  wlanid, 0);
	ems_json_get_int_def(req, "radioid", radioid, 0);
	ems_json_get_string_def(req, "interface", &inf, NULL);

	ems_assert(type >= 1 && type <= 3);
	ems_assert(str_len(&inf) > 0);

	if (type <= 0 || type > 3) {
		errcode = ERR_EVT_TYPE_INVALID;
		goto err_out;
	}

	if (str_len(&inf) <= 0) {
		errcode = ERR_EVT_INTERFACE_UNKNOWN;
		goto err_out;
	}

	if (capture->st == st_paused) {
		ems_assert(capture->n_capture == 0);
		errcode = 0;
		goto err_out;
	} else {
		/* find out that capturing interface */
		cap = capture_get_cap(capture, str_text(&inf));
		if (!cap) {
			errcode = 0;
			goto err_out;
		}

		ems_assert(cap->st != st_stopped && cap->st != st_start);
		cap_change_status(cap, st_uploading);
	}

	str_uninit(&inf);
	return EMS_OK;

err_out:
	ctrl_send_event_to_wtp(capture->_core->ctrl, 
			core_build_wtp_event(WTP_EVT_STOP,
				type, wlanid, radioid, str_text(&inf),
				errcode, NULL 
			)
		);
	str_uninit(&inf);

	return EMS_OK;
}

ems_int capture_handle_msg_stopall(ems_capture *capture, json_object *req)
{
	ems_int type, wlanid, radioid;
	ems_str inf;
	ems_do_capture  *cap = NULL;

	str_init(&inf);

	ems_json_get_int_def(req, "type",   type, 0);
	ems_json_get_int_def(req, "wlanid",  wlanid, 0);
	ems_json_get_int_def(req, "radioid", radioid, 0);
	ems_json_get_string_def(req, "interface", &inf, NULL);

	if ( (capture->st == st_paused) || 
		ems_queue_empty(&capture->list_capture_inf)) 
	{
		ems_assert(capture->n_capture == 0);
		ctrl_send_event_to_wtp(capture->_core->ctrl, 
			core_build_wtp_event(WTP_EVT_STOP,
				type, wlanid, radioid, str_text(&inf),
				0, NULL 
			)
		);
	} else {
		/* find out that capturing interface */
		ems_queue *p, *q;

		ems_queue_foreach_safe(&capture->list_capture_inf, p, q) {
			cap = ems_container_of(p, ems_do_capture, entry);

			ems_assert( (cap->st != st_stopped) && 
					(cap->st != st_start));
			cap_change_status(cap, st_uploading);
		} 
	}

	str_uninit(&inf);
	return EMS_OK;
}

ems_int capture_cap_exit(ems_capture *capture, ems_do_capture *cap)
{
	ems_assert(capture && cap);

	ems_l_trace("destroy capture: %s", str_text(&cap->inf));

	ems_queue_remove(&cap->entry);
	ems_do_capture_destroy(cap);

	if (ems_queue_empty(&capture->list_capture_inf)) {
		if (capture->st != st_stopped)
			return capture_change_status(capture, st_paused);
	}

	return EMS_OK;
}

ems_int capture_continue_capturing(ems_capture *capture)
{
	ems_queue      *p;
	ems_do_capture *cap;

	if (capture->st != st_capturing)
		return EMS_OK;

	ems_queue_foreach(&capture->list_capture_inf, p) {

		cap = ems_container_of(p, ems_do_capture, entry);

		if (cap->st == st_paused) {
			ems_l_trace("continue capture: %s", str_text(&cap->inf));
			cap_change_status(cap, st_capturing);
		}
	}

	return EMS_OK;
}
