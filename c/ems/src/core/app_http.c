
#include "ems_core.h"
#include "ems_client.h"
#include "app.h"
#include "app_nic.h"
#include "ems_fw.h"
#include "ems_dns.h"
#include "ems_http.h"
#include "ems_split.h"

#define EMS_HTTP_SEP	":"


typedef struct _ems_http_mgmt_s ems_http_mgmt;

struct _ems_http_mgmt_s {
	ems_queue    list;
	json_object *rule;
	ems_nic_wireless *ssid;
};

/*
   {
   	port: ["80", "8080"],

	cna: ["aaa":80, "bbb:80"],
	nas: "saaaa":80:8080
	url: ["encrypt": 8080]
   }
 */

static json_object *ems_http_default_rules()
{
	json_object *root = NULL, *ary;

	root = json_object_new_object();

	ary = json_object_new_array();

	json_object_array_add(ary, json_object_new_string(ems_itoa(80)));

	json_object_object_add(root, "port", ary);
	json_object_object_add(root, "nas", json_object_new_string("115.28.38.73"));

	return root;
}

static ems_http *ems_http_new()
{
	ems_http *http = NULL;

	http = (ems_http *)ems_malloc(sizeof(ems_http));
	if (http) {
		memset(http, 0, sizeof(ems_http));

		ems_queue_init(&http->entry);
		http->flg  = 0;
		http->sess = NULL;
		ems_queue_init(&http->cmd);
		ems_queue_init(&http->url);
		ems_queue_init(&http->grey);
		ems_queue_init(&http->cna);
		ems_queue_init(&http->cna_list);
		http->src_port = http->dst_port = 0;
		str_init(&http->nas);
		http->st   = st_stopped;

		http->hdr = http_header_new();
		if (http->hdr)
			http_header_init(http->hdr);
	}

	return http;
}

static ems_void ems_http_destroy(ems_http *http)
{
	ems_assert(http != NULL);

	if (!http)
		return;

	ems_assert(http->sess == NULL);
	ems_assert(http->st == st_stopped);
	ems_assert(ems_queue_empty(&http->cmd));
	ems_assert(ems_queue_empty(&http->cna_list));

	ems_queue_clear(&http->url, ems_url_whitelist, entry, ems_url_whitelist_destroy);
	ems_queue_clear(&http->grey, ems_url_greylist, entry, ems_url_greylist_destroy);
	ems_queue_clear(&http->cna, ems_cna_rule, entry, ems_cna_rule_destroy); 
	str_uninit(&http->nas);

	if (http->hdr) {
		http_header_destroy(http->hdr);
		http->hdr = NULL;
	}

	ems_free(http);
}

static ems_int http_init(app_module *mod)
{
	ems_http_mgmt *mgmt;

	mgmt = (ems_http_mgmt *)ems_malloc(sizeof(ems_http_mgmt));

	if (!mgmt) 
		return EMS_ERR;

	ems_queue_init(&mgmt->list);
	mgmt->rule = NULL;
	mgmt->ssid = ems_module_attached(ems_nic_wireless, mod);

	mod->ctx = (ems_void *)mgmt;

	return EMS_OK;
}

static ems_int http_uninit(app_module *mod)
{
	ems_http_mgmt *mgmt = (ems_http_mgmt *)mod->ctx;


	ems_assert(mgmt && ems_queue_empty(&mgmt->list));
	ems_assert(mgmt->rule == NULL);

	if (!mgmt->rule) {
		json_object_put(mgmt->rule);
		mgmt->rule = NULL;
	}

	ems_free(mgmt);

	mod->ctx = NULL;

	return EMS_OK;
}

static ems_int ems_http_is_own(ems_queue *list, ems_int src_port)
{
	ems_split *sp = NULL;

	if (ems_split_len(list) == 1)
		return EMS_YES;

	sp = ems_split_find(list, ems_itoa(src_port));

	if (sp)
		return EMS_YES;

	return EMS_NO;
}

static ems_int ems_http_get_nas(ems_http *http, json_object *root)
{
	ems_str nas;
	ems_queue list;

	str_init(&nas);

	if (root && json_object_is_type(root, json_type_string)) {
		ems_queue_init(&list);
		str_set(&nas, json_object_get_string(root));
		ems_string_split(&list, str_text(&nas), EMS_HTTP_SEP);

		str_set(&nas, ems_split_get_str(&list, 0));

		if (str_len(&nas) > 0 && ems_http_is_own(&list, http->src_port)) {
			ems_l_trace("[http]nas address: %s", str_text(&nas));
			str_cpy(&http->nas, &nas);
			ems_flag_set(http->flg, EMS_FLG_HTTP_ENABLE_NASGETINFO);
		}

		ems_split_clear(&list);
	}

	str_uninit(&nas);

	return EMS_OK;
}

static ems_int ems_http_get_url_whitelist(ems_http *http, json_object *ary)
{
	json_object *obj;
	ems_int   i;
	ems_str   str;
	ems_queue list;
	ems_url_whitelist *white;

	str_init(&str);
	ems_queue_init(&list);

	if (ary && json_object_is_type(ary, json_type_array)) {

		for (i = 0; i < json_object_array_length(ary); i++) {
			obj = json_object_array_get_idx(ary, i);

			if (!(obj && json_object_is_type(obj, json_type_string)))
				continue;

			ems_string_split(&list, json_object_get_string(obj), EMS_HTTP_SEP);
			str_set(&str, ems_split_get_str(&list, 0));

			if (str_len(&str) > 0  && ems_http_is_own(&list, http->src_port)) {
				ems_l_trace("[http]url whitelist address: %s", str_text(&str));
				white = ems_url_whitelist_new();
				if (white) {
					str_cpy(&white->key, &str);
					ems_queue_insert_tail(&http->url, &white->entry);
					ems_flag_set(http->flg, EMS_FLG_HTTP_ENABLE_URL);
				}
			}
			ems_split_clear(&list);
		}
	}

	str_uninit(&str);

	return EMS_OK;
}

static ems_int ems_http_get_cna_list(ems_http *http, json_object *ary)
{
	json_object *obj;
	ems_int   i;
	ems_str   host, param;
	ems_queue list;
	ems_cna_rule  *cna;

	str_init(&host);
	str_init(&param);
	ems_queue_init(&list);

	if (ary && json_object_is_type(ary, json_type_array)) {

		for (i = 0; i < json_object_array_length(ary); i++) {
			obj = json_object_array_get_idx(ary, i);

			if (!(obj && json_object_is_type(obj, json_type_string)))
				continue;

			ems_string_split(&list, json_object_get_string(obj), EMS_HTTP_SEP);
			str_set(&host,  ems_split_get_str(&list, 0));
			str_set(&param, ems_split_get_str(&list, 1));

			if (str_len(&host) > 0  && ems_http_is_own(&list, http->src_port)) {
				ems_l_trace("[http]cna address: %s: %s", str_text(&host), str_text(&param));
				cna = ems_cna_rule_new();
				if (cna) {
					str_cpy(&cna->host,  &host);
					str_cpy(&cna->param, &param);
					ems_queue_insert_tail(&http->cna, &cna->entry);
					ems_flag_set(http->flg, EMS_FLG_HTTP_ENABLE_CNA);
				}
			}
			ems_split_clear(&list);
		}
	}

	str_uninit(&host);
	str_uninit(&param);

	return EMS_OK;
}

static ems_int ems_http_get_url_greylist(ems_http *http, json_object *ary)
{
	json_object *obj;
	ems_int   i;
	ems_str   str;
	ems_queue list;
	ems_url_greylist *grey;

	str_init(&str);
	ems_queue_init(&list);

	if (ary && json_object_is_type(ary, json_type_array)) {

		for (i = 0; i < json_object_array_length(ary); i++) {
			obj = json_object_array_get_idx(ary, i);

			if (!(obj && json_object_is_type(obj, json_type_string)))
				continue;

			ems_string_split(&list, json_object_get_string(obj), EMS_HTTP_SEP);
			str_set(&str, ems_split_get_str(&list, 0));

			if (str_len(&str) > 0  && ems_http_is_own(&list, http->src_port)) {
				ems_l_trace("[http]url greylist address: %s", str_text(&str));
				grey = ems_url_greylist_new();
				if (grey) {
					str_cpy(&grey->key, &str);
					ems_queue_insert_tail(&http->grey, &grey->entry);
					ems_flag_set(http->flg, EMS_FLG_HTTP_ENABLE_GREYLIST);
				}
			}
			ems_split_clear(&list);
		}
	}

	str_uninit(&str);

	return EMS_OK;
}


static ems_int ems_http_create_new(ems_http_mgmt *mgmt, ems_int src_port)
{
	ems_http *http = NULL;

	http = ems_http_new();

	http->src_port = src_port;
	http->ssid = mgmt->ssid;

	ems_l_trace("[http] redirect port: %d", http->src_port);
	ems_http_get_nas(http, json_object_object_get(mgmt->rule, "nas"));
	ems_http_get_url_whitelist(http, json_object_object_get(mgmt->rule, "url"));
	ems_http_get_cna_list(http, json_object_object_get(mgmt->rule, "cna"));
	ems_http_get_url_greylist(http, json_object_object_get(mgmt->rule, "grey"));

	if (ems_http_change_status(http, st_start) != OK) {
		ems_http_change_status(http, st_stopped);
		ems_http_destroy(http);
		return EMS_ERR;
	}

	ems_queue_insert_tail(&mgmt->list, &http->entry);
	return EMS_OK;
}


static ems_int http_start_all(ems_http_mgmt *mgmt)
{
	json_object *ary, *obj;
	ems_int i;

	if (!mgmt->rule)
		mgmt->rule = ems_http_default_rules();

	if (!mgmt->rule)
		return EMS_ERR;

	ems_l_trace("[http] rules: %s", json_object_to_json_string(mgmt->rule));

	ems_assert(ems_queue_empty(&mgmt->list));

	ary = json_object_object_get(mgmt->rule, "port");

	ems_assert(ary != NULL);

	if (!ary) {
		ems_send_message(ty_http, ty_http, EMS_APP_STOP,  NULL);
		ems_send_message(ty_http, ty_http, EMS_APP_START, NULL);
		return EMS_ERR;
	}

	for (i = 0; i < json_object_array_length(ary); i++) {

		obj = json_object_array_get_idx(ary, i);

		if (obj && json_object_is_type(obj, json_type_string)) 
			ems_http_create_new(mgmt, ems_atoi(json_object_get_string(obj)));
	}

	return EMS_OK;
}

static ems_int http_stop_all(ems_http_mgmt *mgmt)
{
	ems_queue *p, *q;
	ems_http  *http = NULL;

	ems_queue_foreach_safe(&mgmt->list, p, q) {

		http = ems_container_of(p, ems_http, entry);
		ems_http_change_status(http, st_stopped);

		ems_queue_remove(&http->entry);
		ems_http_destroy(http);
	}

	if (mgmt->rule != NULL) {
		json_object_put(mgmt->rule);
		mgmt->rule = NULL;
	}

	return EMS_OK;
}

static ems_int http_run(app_module *mod, ems_int run)
{
	ems_http_mgmt *mgmt = (ems_http_mgmt *)mod->ctx;

	if (run) {
		if (ems_flag_like(mod->flg, FLG_RUN))
			return EMS_OK;

		if (http_start_all(mgmt) == EMS_OK)
			ems_flag_set(mod->flg, FLG_RUN);
	} else {
		if (ems_flag_unlike(mod->flg, FLG_RUN))
			return EMS_OK;

		http_stop_all(mgmt);
		ems_flag_unset(mod->flg, FLG_RUN);
	}

	return EMS_OK;
}

static ems_int 
http_server_rules_update(app_module *mod, json_object *root)
{
	ems_http_mgmt *mgmt = (ems_http_mgmt *)mod->ctx;

	if (!root)
		return EMS_OK;

	if (!ems_json_object_to_string_cmp(root, mgmt->rule))
		return EMS_OK;

	http_stop_all(mgmt);

	mgmt->rule = ems_json_tokener_parse(json_object_to_json_string(root));

	if (ems_flag_like(mod->flg, FLG_RUN))
		http_start_all(mgmt);

	return EMS_OK;
}

extern ems_int http_set_nas_free(ems_http *http, ems_int enable);
extern ems_int http_set_http_free(ems_http *http, ems_int enable);
extern ems_int http_set_grey_free(ems_http *http, ems_int enable);

static ems_int
http_fw_reload(app_module *mod, json_object *req)
{
	ems_http_mgmt *mgmt = (ems_http_mgmt *)mod->ctx;
	ems_queue *p;
	ems_http  *http;

	ems_queue_foreach(&mgmt->list, p){
		http = ems_container_of(p, ems_http, entry);

		if (ems_flag_like(http->flg, EMS_FLG_HTTP_ENABLE_NASGETINFO))
			http_set_nas_free(http, EMS_YES);

		if (ems_flag_like(http->flg, EMS_FLG_HTTP_ENABLE_GREYLIST))
			http_set_grey_free(http, EMS_YES);

		http_set_http_free(http, EMS_YES);
	}

	return EMS_OK;
}

static ems_int
http_process(app_module *mod, ems_uint s, ems_uint d, ems_uint evt, json_object *root)
{
	switch(evt) {
	case EMS_APP_SERVER_RULES_UPDATE:
		return http_server_rules_update(mod, root);

	case EMS_APP_EVT_FW_RELOAD:
		return http_fw_reload(mod, root);

	case EMS_APP_START:
		return http_run(mod, EMS_YES);

	case EMS_APP_STOP:
		return http_run(mod, EMS_NO);

	default:
		break;
	}

	return EMS_OK;
}


app_module app_http = 
{
	.ty      = ty_http,
	.desc    = ems_string("http"),
	.ctx     = NULL,
	.flg     = 0,

	.init    = http_init,
	.uninit  = http_uninit,
	.run     = http_run,
	.process = http_process,
	.process_rule  = NULL,
	.version_match = NULL,
	.install       = NULL
};
