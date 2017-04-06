
#include "ems_core.h"


#define cap_event_set(cap, flg, cb) do { \
	cap->evt.fd = cap->pcap.fd; \
	ems_event_add(eventer(), &cap->evt, flg | EMS_EVT_EDGE_TRIGGER, cb); \
} while (0)

#define cap_event_cancel(cap) ems_event_del(eventer(), &cap->evt)

#define ONE_MIN	60000 /* 1min = 60 * 1000 */

#define cap_timeout_set(cap, cb) do { 		\
	ems_int msecs = ONE_MIN;		\
	if (cap->limit_timeuse < msecs)		\
		msecs = cap->limit_timeuse;	\
	cap->limit_timeuse -= msecs;		\
	ems_timeout_set(timeouter(), &cap->to, msecs, cb, EMS_TIMEOUT_SORT); \
} while (0)

#define cap_timeout_cancel(cap) ems_timeout_cancel(&cap->to)


/*             +--------------+
               |              |
               V              |
   start --> capturing ---->paused
     |	        |	      |
     |	        |	      |
     V	        V	      | 
   stopped<---uploading <-----+
 */

ems_channel *ems_channel_new()
{
	ems_channel *ch = NULL;

	ch = (ems_channel *)ems_malloc(sizeof(ems_channel));
	if (ch) {
		ems_queue_init(&ch->entry);
		ch->channel = 0;
	}
	
	return ch;
}

ems_void ems_channel_destroy(ems_channel *ch)
{
	ems_assert(ch != NULL);
	if (ch) {
		ems_free(ch);
	}
}


static ems_sniffer_file *ems_sniffer_file_new()
{
	ems_sniffer_file *fl = NULL;

	fl = (ems_sniffer_file *)ems_malloc(sizeof(ems_sniffer_file));
	if (fl) {
		str_init(&fl->name);
		str_init(&fl->path);

		fl->filesize = 0;
		fl->flg      = 0;

		ems_queue_init(&fl->output_entry);
		ems_queue_init(&fl->entry);
		
		fl->inf = NULL;
	}

	return fl;
}

static ems_void ems_sniffer_file_destroy(ems_sniffer_file *fl)
{
	ems_assert(fl != NULL);

	if (fl) {
		str_uninit(&fl->name);
		str_uninit(&fl->path);
		ems_free(fl);
	}
}

static ems_int cap_file_make(ems_do_capture *cap)
{
	struct tm *tm = &cap->tm_start;
	ems_char fname[512];

	snprintf(fname, sizeof(fname), 
				/* 201683194432*/
			"%s_%04d%02d%02d%02d%02d%02d_%s_%d.pcap", 
			str_text(&cap->capture->mac), 
			tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
			tm->tm_hour, tm->tm_min, tm->tm_sec,
			str_text(&cap->inf), ++cap->file.id);

	str_set(&cap->file.name, fname);

	return EMS_OK;
}

static ems_int cap_file_prepare(ems_do_capture *cap)
{
	ems_char  fl[512];

	ems_assert(cap->file.fp == NULL);

	if (cap->file.fp != NULL)
		return EMS_OK;

	cap_file_make(cap);

	snprintf(fl, sizeof(fl), "%s/%s", 
		str_text(&cap->file.rootpath), str_text(&cap->file.name));

	ems_l_trace("pcap_dump_open file: %s", fl);

	cap->file.fp = pcap_dump_open(cap->pcap.pfd, fl);

	if (!cap->file.fp) {
		snprintf(cap->errmsg, sizeof(cap->errmsg), 
			"pcap_dump_open error: %s", pcap_geterr(cap->pcap.pfd));
		cap->lasterr = ERR_EVT_PCAP_ERROR;
		return EMS_ERR;
	}

	return EMS_OK;
}

/*
   close current pcap file 
   send to output module
 */
static ems_int cap_file_finished(ems_do_capture *cap)
{
	ems_sniffer_file *sfl = NULL;

	if (!cap->file.fp) {
		/* just did not recvied any data */
		cap->lasterr = EMS_OK;
		return EMS_ERR;
	}

	/* update local limit settings */
	if (cap->limit_pkg > 0)
		cap->limit_pkg -= cap->file.pkgs;

	if (cap->limit_filesize > 0)
		cap->limit_filesize -= cap->file.size;

	pcap_dump_close(cap->file.fp);
	cap->file.fp = NULL;

	sfl = ems_sniffer_file_new();
	ems_assert(sfl != NULL);
	if (!sfl) {
		/* delete current file , memory not enough. */
		cap->lasterr = ERR_EVT_MEMORY_ERROR;
		snprintf(cap->errmsg, sizeof(cap->errmsg),
				"memory error: %s", ems_lasterrmsg());
		return EMS_ERR;
	}

	str_cpy(&sfl->name, &cap->file.name);
	str_cpy(&sfl->path, &cap->file.rootpath);

	sfl->filesize = cap->file.size;
	sfl->inf = cap;

	ems_queue_insert_tail(&cap->file.list, &sfl->entry);

	ems_l_trace("output do upload file: %s/%s, limit(pkg: %d, size: %d)",
			str_text(&sfl->path), str_text(&sfl->name), 
			cap->limit_pkg, cap->limit_filesize);

	cap->file.pkgs = 0;
	cap->file.size = 0;

	ems_assert(cap->capture->_core->output != NULL);

	return output_upload_file(cap->capture->_core->output, sfl);
}

ems_void
cap_packages(u_char *args, const struct pcap_pkthdr *header, const u_char *packet)
{
	off_t fsize, len;
	ems_do_capture *cap = (ems_do_capture *)args;

	if (ems_flag_like(cap->flg, FLG_UPLOADING)) 
		return;

	if (cap->file.fp == NULL) {
		if (cap_file_prepare(cap) != EMS_OK)
			goto end_capture;
	}

	pcap_dump((u_char *)cap->file.fp, header, packet);
	pcap_dump_flush(cap->file.fp);

	fsize = pcap_dump_ftell(cap->file.fp);

	len = fsize - cap->file.size; /* get last time write length */
	if (len <= 0) {
		cap->lasterr = ERR_EVT_PCAP_ERROR;
		snprintf(cap->errmsg, sizeof(cap->errmsg),
			"pcap_write error, %s", ems_lasterrmsg());
		goto end_capture;
	}

	/* check file size */
	cap->file.size = fsize;
	if ((cap->limit_filesize > 0) && 
			(cap->limit_filesize <= cap->file.size)) {
		ems_l_trace("[%s]filesize limit reached", str_text(&cap->inf));
		cap->lasterr = 0;
		goto end_capture;
	}

	/* check pkgs */
	cap->file.pkgs++;
	if (cap->limit_pkg > 0) {
		if (cap->limit_pkg <= cap->file.pkgs) {
			ems_l_trace("[%s]package number limit reached",
					str_text(&cap->inf));
			cap->lasterr = 0;
			goto end_capture;
		}
	}

	if (cap->max_filesize <= cap->file.size) {
		ems_l_trace("[%s] max_filesize(%d) limit reached",
				str_text(&cap->inf), cap->max_filesize);
		if (cap_file_finished(cap) != EMS_OK)
			goto end_capture;
	}

	/* check ap memory left space */
	cap->capture->mem_left -= len;
	if (cap->capture->mem_left <= 0) {
		ems_flag_set(cap->flg, FLG_PAUSE);
	}

	ems_l_trace("[%s] -> pkgs: %10lld, filesize: %12lld, recved: %4lld, "
		    "mem_left: %12lld, max_filesize: %12lld",
		    str_text(&cap->inf),
		    cap->file.pkgs, cap->file.size, len,
		    cap->capture->mem_left, cap->max_filesize);

	return;
end_capture:
	ems_l_trace("[%s] -> pkgs: %10lld, filesize: %12lld, recved: %4lld, "
		    "mem_left: %12lld, max_filesize: %12lld",
		    str_text(&cap->inf),
		    cap->file.pkgs, cap->file.size, len,
		    cap->capture->mem_left, cap->max_filesize);
	ems_flag_set(cap->flg, FLG_UPLOADING);
	/* do not change status here */
	//cap_change_status(cap, st_uploading);
}

static ems_void cap_evt_cb(ems_event_fd *event, ems_int flg)
{
	ems_do_capture *cap = ems_container_of(event, ems_do_capture, evt);

	ems_assert(cap != NULL);

	if (event->error) {
		cap->lasterr = ERR_EVT_PCAP_ERROR;
		snprintf(cap->errmsg, sizeof(cap->errmsg),
				"capture event error: %s", ems_lasterrmsg());
		cap_change_status(cap, st_uploading);
		return;
	}

	if (cap->st != st_capturing)
		return;

	if (ems_flag_like(flg, EMS_EVT_READ)) {
		cap_event_cancel(cap);

		if (pcap_dispatch(cap->pcap.pfd, 
				-1, cap_packages, (ems_uchar *)cap) < 0) {
			cap->lasterr = ERR_EVT_PCAP_ERROR;
			snprintf(cap->errmsg, sizeof(cap->errmsg),
				"pcap_dispatch error: %s", 
				pcap_geterr(cap->pcap.pfd));
			ems_flag_set(cap->flg, FLG_UPLOADING);
		}

		cap_event_set(cap, EMS_EVT_READ, cap_evt_cb);

		if (ems_flag_like(cap->flg, FLG_UPLOADING)) {
			cap_change_status(cap, st_uploading);
		} else if (ems_flag_like(cap->flg, FLG_PAUSE)) {
			cap_change_status(cap, st_paused);
		} else {
			/* do nothing */
		}
	}
}

static ems_void cap_timeout_cb(ems_timeout *timeout)
{
	ems_do_capture *cap = ems_container_of(timeout, ems_do_capture, to);

	if (cap->limit_timeuse <= 0) {
		ems_l_trace("[%s] timeout reached, stop capture", 
				str_text(&cap->inf));

		ems_assert(cap->st == st_capturing || cap->st == st_paused);

		cap->lasterr = 0;
		cap_change_status(cap, st_uploading);
		return;
	}

	ems_l_trace("[%s] capture time left: %lld msecs", 
			str_text(&cap->inf), cap->limit_timeuse);
	cap_timeout_set(cap, cap_timeout_cb);
}

static ems_int cap_open_pcap(ems_do_capture *cap)
{
	ems_int    dlt;
	ems_char   errbuf[PCAP_ERRBUF_SIZE];

	ems_l_trace("cap open dev : %s", str_text(&cap->inf));

	cap->pcap.pfd = pcap_create(str_text(&cap->inf), errbuf);
	if (cap->pcap.pfd == NULL) {
		snprintf(cap->errmsg, sizeof(cap->errmsg),
				"pcap_create failed, %s", errbuf);
		goto err_out;
	}

	if (pcap_set_snaplen(cap->pcap.pfd, 65535) != 0) {
		snprintf(cap->errmsg, sizeof(cap->errmsg),
				"pcap_set_snaplen failed: %s", pcap_geterr(cap->pcap.pfd));
		goto err_out;
	}

	if (pcap_set_promisc(cap->pcap.pfd, 1)) {
		snprintf(cap->errmsg, sizeof(cap->errmsg),
			"pcap_set_promisc failed: %s", pcap_geterr(cap->pcap.pfd));
		goto err_out;
	}

#if 0
	if (pcap_set_immediate_mode(cap->pcap.pfd, 1)) {
		snprintf(cap->errmsg, sizeof(cap->errmsg),
			"pcap_set_immediate_mode failed: %s", pcap_geterr(cap->pcap.pfd));
		goto err_out;
	}
#endif

	if (pcap_activate(cap->pcap.pfd)) {
		snprintf(cap->errmsg, sizeof(cap->errmsg),
			"pcap_activate failed: %s", pcap_geterr(cap->pcap.pfd));
		goto err_out;
	}

	dlt = pcap_datalink(cap->pcap.pfd);

	ems_l_trace("[%s]pcap datalink: %s, %s",
			str_text(&cap->inf),
			pcap_datalink_val_to_name(dlt),
			pcap_datalink_val_to_description(dlt));

	return EMS_OK;

err_out:
	cap->lasterr = ERR_EVT_LOAD_PCAP_FILED;
	ems_l_warn("[cap] failed: %s", cap->errmsg);

	if (cap->pcap.pfd != NULL) {
		pcap_close(cap->pcap.pfd);
		cap->pcap.pfd = NULL;
	}

	return ERR_EVT_LOAD_PCAP_FILED;
}

/*
   check local SERVER ROUTING filter

excepted_filter: 

invalid: 
	not ( (host a.b.c.d or host c.d.e.f or...) and tcp port FTPPORT)

	for that ftp-data port is not "FTPPORT", it is the another way

valid:
	not ((host a.b.c.d or host c.d.e.f or... ) and tcp)
	all tcp datas to that server are gonna be gone

 */
static ems_int cap_update_filters(ems_do_capture *cap)
{
	struct addrinfo *answer, hint, *curr;
	ems_char *pbuf;
	ems_int   ret, left;
	ems_buffer buf;

	bzero(&hint, sizeof(hint));
	hint.ai_family   = AF_INET;
	hint.ai_socktype = SOCK_DGRAM | SOCK_STREAM;
	hint.ai_flags    = AI_PASSIVE;

	ret = getaddrinfo(str_text(&cap->server.host), NULL, &hint, &answer);
	if (ret != 0) {
		ems_l_warn("getaddrinfo(%s) failed: %s",
				str_text(&cap->server.host), gai_strerror(ret));
		return ERR_EVT_SERVER_FTP_HOST;
	}

	ems_assert(answer != NULL);
	if (!answer) /* there's no filter here */
		return ERR_EVT_SERVER_FTP_HOST;

	ems_buffer_init(&buf, EMS_BUFFER_2K);

	pbuf = (ems_char *)buf_wr(&buf);
	left = buf_left(&buf);

	ret = snprintf(pbuf, left, "not ((host %s", 
		inet_ntoa(((struct sockaddr_in *)answer->ai_addr)->sin_addr));

	pbuf += ret;
	left -= ret;

	for (curr = answer->ai_next; curr != NULL; curr = curr->ai_next) {
		ret = snprintf(pbuf, left, "or host %s", 
			inet_ntoa(((struct sockaddr_in *)curr->ai_addr)->sin_addr));

		pbuf += ret;
		left -= ret;
	}

	freeaddrinfo(answer);

	ret = snprintf(pbuf, left, ") and tcp)");
	pbuf += ret;
	left -= ret;

	str_set(&cap->pcap.excepted_filter, buf_rd(&buf));

	ems_buffer_uninit(&buf);

	ems_l_trace("excepted filter : %s", str_text(&cap->pcap.excepted_filter));

	return EMS_OK;
}

ems_cchar *cap_build_filters(ems_do_capture *cap)
{
	ems_buffer *buf = core_buffer();

	if (str_len(&cap->pcap.excepted_filter) > 0 && str_len(&cap->pcap.filter) > 0) {
		snprintf(buf_wr(buf), buf_left(buf), "(%s) and (%s)", 
				str_text(&cap->pcap.excepted_filter),
				str_text(&cap->pcap.filter));

		return (ems_cchar *)buf_wr(buf);
	} else {
		if (str_len(&cap->pcap.excepted_filter) > 0) {
			snprintf(buf_wr(buf), buf_left(buf), "%s", 
					str_text(&cap->pcap.excepted_filter));
			return (ems_cchar *)buf_wr(buf);
		}

		if (str_len(&cap->pcap.filter) > 0) {
			snprintf(buf_wr(buf), buf_left(buf), "%s", 
					str_text(&cap->pcap.excepted_filter));
			return (ems_cchar *)buf_wr(buf);
		}
	}

	return NULL;
}

static ems_int cap_prepare_pcap(ems_do_capture *cap)
{
	ems_int     ret;
	ems_cchar  *filter = NULL;
	ems_char    errbuf[PCAP_ERRBUF_SIZE];

	filter = cap_build_filters(cap);

	if (cap_open_pcap(cap) != EMS_OK) {
		ret = cap->lasterr;
		cap_change_status(cap, st_stopped);
		return ret;
	}

	if (pcap_lookupnet(str_text(&cap->inf),
			&cap->pcap.network, 
			&cap->pcap.netmask, errbuf) < 0)
	{
		cap->pcap.network = cap->pcap.netmask = 0;
		ems_l_trace("pcap_lookupnet failed on %s, %s", 
				str_text(&cap->inf), errbuf);
	}

	ems_l_trace("[cap %s] filter: %s", str_text(&cap->inf), filter);

	if (pcap_compile(cap->pcap.pfd, &cap->pcap.fp, filter, 0, 
				cap->pcap.network) == -1) {
		cap->lasterr = ERR_EVT_PCAP_FILTER;
		snprintf(cap->errmsg, sizeof(cap->errmsg),
			"pcap_comiple failed. %s", pcap_geterr(cap->pcap.pfd));
		ret = cap->lasterr;
		cap_change_status(cap, st_stopped);
		return ret;
	}

	if (pcap_setfilter(cap->pcap.pfd, &cap->pcap.fp) == -1) {
		cap->lasterr = ERR_EVT_PCAP_FILTER;
		snprintf(cap->errmsg, sizeof(cap->errmsg),
			"pcap_setfilter failed. %s", pcap_geterr(cap->pcap.pfd));
		ret = cap->lasterr;
		cap_change_status(cap, st_stopped);
		return ret;
	}

	cap->pcap.fd  = pcap_get_selectable_fd(cap->pcap.pfd);

	pcap_setnonblock(cap->pcap.pfd, 1, errbuf);

	return EMS_OK;
}


static ems_int cap_prepare_output(ems_do_capture *cap)
{
	ems_char buf[512];

	snprintf(buf, sizeof(buf), "%s/%s", 
			SNIFFER_WORKING_DIR, str_text(&cap->inf));

	str_set(&cap->file.rootpath, buf);
	rm_rf(str_text(&cap->file.rootpath));
	mkdir_p(str_text(&cap->file.rootpath));

	return EMS_OK;
}

static ems_int cap_status_into_start(ems_do_capture *cap)
{
	time_t      tm_t;
	ems_int     ret;

	ret = cap_prepare_output(cap);
	if (ret != EMS_OK) {
		cap->lasterr = ret;
		snprintf(cap->errmsg, sizeof(cap->errmsg), 
			"cap_prepare_output failed: %s", ems_lasterrmsg());
		cap_change_status(cap, st_stopped);
		return ret;
	}

	if (str_len(&cap->pcap.excepted_filter) <= 0) {
		ret = cap_update_filters(cap);
		if (ret != EMS_OK) {
			/* send wtp event "err" */
			cap->lasterr = ret;
			cap_change_status(cap, st_stopped);
			return ret;
		}
	}

	tm_t = time(NULL);
	gmtime_r(&tm_t, &cap->tm_start);

	ret = cap_prepare_pcap(cap);
	if (ret != EMS_OK)
		return ret;

	if (cap->limit_timeuse > 0)
		cap_timeout_set(cap, cap_timeout_cb);

	return cap_change_status(cap, st_capturing);
}

static ems_int cap_channel_update_set_next(ems_do_capture *cap);

static ems_void cap_channel_update_timeout_cb(ems_timeout *timeout)
{
	ems_do_capture *cap = 
		ems_container_of(timeout, ems_do_capture, channel_timeout);

	ems_assert(cap->cur_channel != NULL);
	cap_channel_update_set_next(cap);
}

#ifdef SNIFFER_USE_LOCAL_CHANNEL_UPDATE
static ems_int cap_channel_find_and_set(ems_do_capture *cap)
{
	ems_int      ret;
	ems_channel *ch;
	ems_queue   *p, *q;

	ems_assert(cap->cur_channel != NULL);

retry_again:
	for (p = cap->cur_channel, q = ems_queue_next(p);
	     p != ems_queue_sentinel(&cap->channel_lists);
	     p = q, q = ems_queue_next(p)) {

		ch  = ems_container_of(p, ems_channel, entry);
		ret = ems_systemcmd("iwconfig %s channel %d",
				str_text(&cap->inf), ch->channel);

		if (ret != EMS_OK) {
			ems_l_warn("[%s], channel %d set failed, remove it", 
					str_text(&cap->inf), ch->channel);
			ems_queue_remove(&ch->entry);
			ems_channel_destroy(ch);
		} else {
			if (q == ems_queue_sentinel(&cap->channel_lists)) {
				q = ems_queue_head(&cap->channel_lists);
			}

			cap->cur_channel = q;
			return EMS_OK;
		}
	}
	/* no channal valid, reset channel index*/
	if (ems_queue_empty(&cap->channel_lists))
		return EMS_ERR;

	cap->cur_channel = ems_queue_head(&cap->channel_lists);

	goto retry_again;

	return EMS_ERR;
}

static void capture_enable_disable_event(ems_capture *capture, ems_int enable)
{
	ems_queue      *p;
	ems_do_capture *cap = NULL;
	ems_assert(capture != NULL);

	ems_queue_foreach(&capture->list_capture_inf, p) {
		cap = ems_container_of(p, ems_do_capture, entry);

		if (enable) {
			cap_event_set(cap, EMS_EVT_READ, cap_evt_cb);
		} else {
			cap_event_cancel(cap);
		}
	}
}

static ems_int cap_channel_update_set_next(ems_do_capture *cap)
{
	ems_int      len = 0;

	if (cap->type != CAPTURE_TYPE_RADIO)
		return EMS_OK;

	if (ems_queue_empty(&cap->channel_lists))
		goto err_out;
#if 0
	cap_event_cancel(cap);
#else
	capture_enable_disable_event(cap->capture, 0);
#endif

	if (!cap->cur_channel)
		cap->cur_channel = ems_queue_head(&cap->channel_lists);

	if (EMS_OK != cap_channel_find_and_set(cap)) {
		ems_assert(ems_queue_empty(&cap->channel_lists));
		goto err_out;
	}

	ems_assert(!ems_queue_empty(&cap->channel_lists));

	ems_queue_len(&cap->channel_lists, len);

	/* if we have only one channel could be use, we 
	 * need not update the channel
	 * */
	if (len <= 1)
		goto err_out;

	ems_timeout_set(timeouter(), &cap->channel_timeout, 
		cap->channel_internal, cap_channel_update_timeout_cb,
		EMS_TIMEOUT_SORT);

err_out:
#if 0
	cap_event_set(cap, EMS_EVT_READ, cap_evt_cb);
#else
	capture_enable_disable_event(cap->capture, 1);
#endif

	return EMS_OK;
}
#else
/* channel updated by apcfg, just send channel to apcfg */
static ems_int cap_channel_update_set_next(ems_do_capture *cap)
{
	json_object  *jobj = NULL;
	ems_channel  *ch;
	ems_int       len = 0;

	if (cap->type != CAPTURE_TYPE_RADIO)
		return EMS_OK;

	if (ems_queue_empty(&cap->channel_lists))
		return EMS_OK;

	if (   !cap->cur_channel || 
		cap->cur_channel == ems_queue_sentinel(&cap->channel_lists)) {
		cap->cur_channel = ems_queue_head(&cap->channel_lists);
	}

	jobj = core_build_apcfg_event(APCFG_MSG_UPDATE_CHANNEL, NULL, 0, NULL);
	if (jobj) {
		ch = ems_container_of(cap->cur_channel, ems_channel, entry);

		json_object_object_add(jobj, "interface", 
				json_object_new_string(str_text(&cap->inf)));
		json_object_object_add(jobj, "channel", 
				json_object_new_int(ch->channel));

		ctrl_send_event_to_apcfg(cap->capture->_core->ctrl, jobj);
	}

	cap->cur_channel = ems_queue_next(cap->cur_channel);

	ems_assert(!ems_queue_empty(&cap->channel_lists));
	ems_queue_len(&cap->channel_lists, len);

	/* if we have only one channel could be use, we 
	 * need not update the channel
	 * */
	if (len <= 1)
		return EMS_OK;

	ems_timeout_set(timeouter(), &cap->channel_timeout, 
		cap->channel_internal, cap_channel_update_timeout_cb,
		EMS_TIMEOUT_SORT);

	return EMS_OK;
}

#endif


/*
   if we are in radio module, we should set timeout for channal 
   update timeout, just finished it later
 */
static ems_int cap_status_into_capturing(ems_do_capture *cap)
{
	if (ems_flag_unlike(cap->flg, FLG_PAUSE)) {
		ctrl_send_event_to_wtp(cap->capture->_core->ctrl, 
			core_build_wtp_event(WTP_EVT_START,
				cap->type, cap->wlanid, cap->radioid,
				str_text(&cap->inf),
				0, NULL 
			)
		);
	} else {
		ems_flag_unset(cap->flg, FLG_PAUSE);
		ctrl_send_event_to_wtp(cap->capture->_core->ctrl, 
			core_build_wtp_event(WTP_EVT_CONTINUE,
				cap->type, cap->wlanid, cap->radioid,
				str_text(&cap->inf),
				0, NULL 
			)
		);
	}

	cap_event_set(cap, EMS_EVT_READ, cap_evt_cb);
	cap_channel_update_set_next(cap);

	return EMS_OK;
}

static ems_int cap_status_into_paused(ems_do_capture *cap)
{
	ems_assert(ems_flag_like(cap->flg, FLG_PAUSE));

	ctrl_send_event_to_wtp(cap->capture->_core->ctrl, 
		core_build_wtp_event(WTP_EVT_PAUSED,
			cap->type, cap->wlanid, cap->radioid, 
			str_text(&cap->inf),
			0, NULL 
		)
	);

	cap_event_cancel(cap);
	ems_timeout_cancel(&cap->channel_timeout);
	/*
	   we should set cap finished, 
	   do not cancel timeout here this time
	 */
	return EMS_OK;
}

static ems_int cap_status_into_uploading(ems_do_capture *cap)
{
	ems_l_trace("[%s] capture stopped and uploading", str_text(&cap->inf));

	cap_timeout_cancel(cap);
	cap_event_cancel(cap);
	ems_timeout_cancel(&cap->channel_timeout);

	if (cap_file_finished(cap) != EMS_OK)
		return cap_change_status(cap, st_stopped);

	cap->lasterr = EMS_OK;
	return EMS_OK;
}

static ems_int cap_status_into_stop(ems_do_capture *cap)
{
	ems_capture *capture;
	ems_queue   *p;
	ems_sniffer_file *sfl;

	cap_timeout_cancel(cap);
	cap_event_cancel(cap);
	ems_timeout_cancel(&cap->channel_timeout);

	if (cap->file.fp != NULL) {
		pcap_dump_close(cap->file.fp);
		cap->file.fp = NULL;
	}

	if (cap->pcap.pfd != NULL) {
		pcap_close(cap->pcap.pfd);
		cap->pcap.pfd = NULL;
		cap->pcap.fd  = 0;
	}

	ems_l_trace("[%s] capture stopped(%d: %s)", 
			str_text(&cap->inf), cap->lasterr, cap->errmsg);

	while (!ems_queue_empty(&cap->file.list)) {
		p = ems_queue_head(&cap->file.list);
		sfl = ems_container_of(p, ems_sniffer_file, entry);

		/* check whether that we uploading current file, setted by
		 * output module  */
		if (ems_flag_like(sfl->flg, FLG_UPLOADING)){
			ems_output  *out;
			out = cap->capture->_core->output;
			if (out && (out->st != st_stopped))
				output_change_status(out, st_stopped);
		} else {
			ems_queue_remove(&sfl->output_entry);
		}

		ems_queue_remove(p);

		cap->capture->mem_left += sfl->filesize;

		ems_sniffer_file_destroy(sfl);
	}

	rm_rf(str_text(&cap->file.rootpath));

	ctrl_send_event_to_wtp(cap->capture->_core->ctrl, 
		core_build_wtp_event(WTP_EVT_STOP,
			cap->type, cap->wlanid, cap->radioid, 
			str_text(&cap->inf),
			cap->lasterr, cap->errmsg
		)
	);

	/* destroy channel lists */
	ems_queue_clear(&cap->channel_lists, 
			ems_channel, entry, ems_channel_destroy);

	cap->cur_channel = NULL;
	ems_timeout_cancel(&cap->channel_timeout);

	/* notify capture module "CAPTURING MODULE" exit */
	capture = cap->capture;
	capture_cap_exit(capture, cap);
	capture_continue_capturing(capture);

	return EMS_OK;
}

ems_int cap_change_status(ems_do_capture *cap, ems_status st)
{
	ems_l_trace("[cap %s]status change[%s -> %s]", 
		str_text(&cap->inf),
		ems_status_str(cap->st), ems_status_str(st));

	if (cap->st == st)
		return EMS_OK;

	cap->st = st;

	switch(st) {
	case st_start:
		return cap_status_into_start(cap);

	case st_paused:
		return cap_status_into_paused(cap);

	case st_capturing:
		return cap_status_into_capturing(cap);

	case st_uploading:
		return cap_status_into_uploading(cap);

	case st_stopped:
		return cap_status_into_stop(cap);

	default:
		break;
	}

	return EMS_OK;
}

static ems_int cap_server_init(ems_server *srv)
{
	ems_assert(srv != NULL);

	memset(srv, 0, sizeof(ems_server));
	str_init(&srv->host);
	str_init(&srv->ftp.url);

	return EMS_OK;
}

static ems_void cap_server_uninit(ems_server *srv)
{
	ems_assert(srv != NULL);

	switch(srv->type) {
	case SERVER_TYPE_FTP:
		str_uninit(&srv->ftp.url);
		break;

	default:
		break;
	}

	str_uninit(&srv->host);
	srv->type = 0;
}

ems_do_capture *ems_do_capture_new()
{
	ems_do_capture *cap = NULL;


	cap = (ems_do_capture *)ems_malloc(sizeof(ems_do_capture));
	if (cap) {
		memset(cap, 0, sizeof(ems_do_capture));

		ems_queue_init(&cap->entry);
		cap->st = st_stopped;
		ems_event_fd_init(&cap->evt);
		ems_timeout_init(&cap->to);
		cap->flg = 0;

		cap->pcap.pfd  = NULL;
		cap->pcap.fd   = 0;
		str_init(&cap->pcap.filter);
		str_init(&cap->pcap.excepted_filter);


		cap->file.id = 0;
		cap->file.fp = NULL;
		str_init(&cap->file.name);
		str_init(&cap->file.rootpath);
		ems_queue_init(&cap->file.list);
		cap->file.pkgs = 0;
		cap->file.timeuse = 0;
		cap->file.size = 0;


		cap->limit_pkg = 0;
		cap->limit_timeuse = 0;
		cap->limit_filesize = 0;

		cap->max_filesize = SNIFFER_MAX_FILESIZE;

		cap->radioid = 0;
		cap->wlanid  = 0;
		cap->type    = 0;
		str_init(&cap->inf);

		cap_server_init(&cap->server);
		cap->capture = NULL;

		ems_timeout_init(&cap->channel_timeout);
		cap->channel_internal = 3000; /* 3s */
		ems_queue_init(&cap->channel_lists);
		cap->cur_channel = NULL;
	}

	return cap;
}

ems_void ems_do_capture_destroy(ems_do_capture *cap)
{
	ems_assert(cap != NULL);

	if (cap) {
		ems_assert(cap->st == st_stopped);
		ems_assert(cap->pcap.pfd == NULL);
		ems_assert(cap->pcap.fd <= 0);
		ems_assert(cap->file.fp == NULL);
		ems_assert(ems_queue_empty(&cap->file.list));
		ems_assert(ems_queue_empty(&cap->channel_lists));
		ems_assert(cap->cur_channel == NULL);

		str_uninit(&cap->pcap.filter);
		str_uninit(&cap->pcap.excepted_filter);
		str_uninit(&cap->file.name);
		str_uninit(&cap->file.rootpath);
		str_uninit(&cap->inf);
		cap_server_uninit(&cap->server);

		ems_free(cap);
	}
}

/* send notify to wtp */
/* send notify to "DO CAPTURING" module */
ems_int 
cap_file_upload_finished(ems_do_capture *cap, ems_sniffer_file *fl, ems_int st)
{
	json_object *jobj;
	ems_char    *errmsg;
	ems_capture *capture;
	ems_assert(cap && fl);

	capture = cap->capture;

	ems_queue_remove(&fl->entry);
	ems_flag_unset(fl->flg, FLG_UPLOADING);

	/* increase capture->mem_left buffer */
	cap->capture->mem_left += fl->filesize;

	if (st != EMS_OK) {
		st = ERR_EVT_UPLOAD_FAILED;
		errmsg = cap->errmsg;
		snprintf(cap->errmsg, sizeof(cap->errmsg), 
			"upload file %s failed", str_text(&fl->name));
	} else {
		errmsg = NULL;
		cap->errmsg[0] = '\0';
	}

	jobj = core_build_wtp_event(WTP_EVT_UPLOADING,
			cap->type, cap->wlanid, cap->radioid, 
			str_text(&cap->inf), st, errmsg);
	if (jobj) {
		json_object_object_add(jobj, "filename",
				json_object_new_string(str_text(&fl->name)));
		ctrl_send_event_to_wtp(cap->capture->_core->ctrl,jobj);
	}

	ems_sniffer_file_destroy(fl);

	if (st != EMS_OK) {
		ems_l_warn("%s capture stopped, upload failed: %d", 
				str_text(&cap->inf), st);
		cap->lasterr = st;
		cap_change_status(cap, st_stopped);
		return EMS_OK;
	}

	if ((cap->st == st_uploading) && ems_queue_empty(&cap->file.list)) {
		cap_change_status(cap, st_stopped);
		return EMS_OK;
	}

	capture_continue_capturing(capture);

	return EMS_OK;
}

ems_int cap_omit_channel_update(ems_do_capture *cap, ems_int channel)
{
	ems_channel *ch = NULL;
	ems_queue *p, *q;

	if (!cap)
		return EMS_ERR;
	ems_l_trace("[%s] remove channel %d", str_text(&cap->inf), channel);

	ems_queue_foreach_safe(&cap->channel_lists, p, q) {
		ch = ems_container_of(p, ems_channel, entry);

		if (ch->channel == channel) {
			ems_queue_remove(p);
			ems_channel_destroy(ch);
			break;
		}
	}

	return EMS_OK;
}
