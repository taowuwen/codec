
#include "ems.h"
#include "json.h"
#include "ems_main.h"
#include "ems_conf.h"


typedef struct _ems_kv_s      cfg_kv;

struct _ems_kv_s {
	ems_str   key;
	ems_str   val;
	ems_queue entry;
};

cfg_kv *cfg_kv_new()
{
	cfg_kv *kv = NULL;

	kv = (cfg_kv *)ems_malloc(sizeof(cfg_kv));
	if (kv) {
		str_init(&kv->key);
		str_init(&kv->val);
		ems_queue_init(&kv->entry);
	}

	return kv;
}

ems_void cfg_kv_destroy(cfg_kv *kv)
{
	if (kv) {
		str_clear(&kv->key);
		str_clear(&kv->val);
		ems_free(kv);
	}
}

static cfg_kv *cfg_find(ems_cfg *cfg, ems_cchar *key)
{
	ems_queue *p;
	cfg_kv    *kv;

	ems_assert(cfg && key);

	ems_queue_foreach(&cfg->kv_entry, p) {
		kv = ems_container_of(p, cfg_kv, entry);

		if (!strcmp(str_text(&kv->key), key))
			return kv;
	}

	return NULL;
}


ems_int  cfg_init(ems_cfg *cfg, ems_cchar *fl)
{
	ems_assert(cfg && fl);

	memset(cfg, 0, sizeof(ems_cfg));

	str_set(&cfg->fl, fl);
	ems_queue_init(&cfg->kv_entry);

	return EMS_OK;
}

ems_void cfg_uninit(ems_cfg *cfg)
{
	ems_assert(cfg);

	str_clear(&cfg->fl);
	ems_queue_clear(&cfg->kv_entry, cfg_kv, entry, cfg_kv_destroy);
}

ems_int cfg_parse_line(ems_cfg *cfg, ems_char *buf)
{
	ems_char *k, *v;

	k = buf;
	v = strchr(k, '=');
	if (!v)
		return EMS_ERR;

	*v++ = '\0';
	ems_trim(k);
	ems_trim(v);

	if (strlen(k) <= 0 || strlen(v) <= 0) 
		return EMS_ERR;

	return cfg_set(cfg, k, v);
}

ems_int  cfg_read(ems_cfg *cfg)
{
	FILE *fp = NULL;
	ems_char buf[1024] = {0};

	ems_assert(cfg && str_len(&cfg->fl) > 0);
	
	fp = fopen(str_text(&cfg->fl), "r");

	if (!fp) {
		ems_l_warn("open cfg file : %s err: %s", str_text(&cfg->fl), ems_lasterrmsg());
		return EMS_ERR;
	}

	while (NULL != fgets(buf, sizeof(buf) - 1, fp)) {
		ems_trim(buf);
		if (strlen(buf) > 0) {
			cfg_parse_line(cfg, buf);
		}
	}

	fclose(fp);

	return EMS_OK;
}

ems_int cfg_write(ems_cfg *cfg)
{
	FILE      *fp = NULL;
	ems_queue *p;
	cfg_kv    *kv;
	ems_char buf[1024] = {0};

	ems_assert(cfg && str_len(&cfg->fl) > 0);
	
	fp = fopen(str_text(&cfg->fl), "w");

	if (!fp) {
		ems_l_warn("open cfg file  %s err: %s", str_text(&cfg->fl), ems_lasterrmsg());
		return EMS_ERR;
	}

	ems_queue_foreach(&cfg->kv_entry, p) {
		kv = ems_container_of(p, cfg_kv, entry);
		ems_assert(str_len(&kv->key) > 0);
		ems_assert(str_len(&kv->val) > 0);

		if (str_len(&kv->key) <= 0 || str_len(&kv->val) <= 0) 
			continue;

		snprintf(buf, sizeof(buf), "%s=%s\n", str_text(&kv->key), str_text(&kv->val));
		fwrite(buf, strlen(buf), 1, fp);
	}

	fclose(fp);

	return EMS_OK;
}

ems_int cfg_flush(ems_cfg *cfg)
{
	ems_assert(cfg && "never show up this line");
	ems_queue_clear(&cfg->kv_entry, cfg_kv, entry, cfg_kv_destroy);
	return cfg_read(cfg);
}

ems_int cfg_set(ems_cfg *cfg, ems_cchar *k, ems_cchar *v)
{
	cfg_kv *kv;

	kv = cfg_find(cfg, k);
	if (!kv) {
		if (!(v && strlen(v) > 0))
			return EMS_OK;

		kv = cfg_kv_new();
		if (!kv)
			return MSG_ST_MEM_ERR;

		str_set(&kv->key, k);
		str_set(&kv->val, v);
		ems_queue_insert_tail(&cfg->kv_entry, &kv->entry);
	}
	else {
		if (!(v && strlen(v) > 0)) {
			ems_queue_remove(&kv->entry);
			cfg_kv_destroy(kv);
		} else
			str_set(&kv->val, v);
	}

	return EMS_OK;
}

ems_cchar *cfg_get(ems_cfg *cfg, ems_cchar *key)
{
	cfg_kv *kv;

	kv = cfg_find(cfg, key);
	if (kv) 
		return str_text(&kv->val);

	return NULL;
}

ems_int cfg_select(ems_cfg *cfg, ems_cchar *like, ems_void *arg, cfg_search_cb cb)
{
	ems_int    rtn, len, n;
	ems_queue *p, *q;
	cfg_kv    *kv;

	if (!cb)
		return EMS_ERR;

	len = strlen(like);

	ems_queue_foreach_safe(&cfg->kv_entry, p, q) {
		kv = ems_container_of(p, cfg_kv, entry);

		ems_assert(str_len(&kv->key) > 0);
		ems_assert(str_len(&kv->val) > 0);

		n = (len <= str_len(&kv->key))?len: str_len(&kv->key);

		if (!strncmp(str_text(&kv->key), like, n)) {
			rtn = cb(arg, str_text(&kv->key), str_text(&kv->val));
			if (rtn != EMS_OK)
				return rtn;
		}
	}

	return EMS_OK;
}

static ems_int cfg_del_json(ems_cfg *cfg, ems_cchar *like)
{
	ems_int    len, n;
	ems_queue *p, *q;
	cfg_kv    *kv;

	len = strlen(like);

	ems_queue_foreach_safe(&cfg->kv_entry, p, q) {
		kv = ems_container_of(p, cfg_kv, entry);

		ems_assert(str_len(&kv->key) > 0);
		ems_assert(str_len(&kv->val) > 0);

		n = (len <= str_len(&kv->key))?len: str_len(&kv->key);

		if (!strncmp(str_text(&kv->key), like, n)) {
			ems_queue_remove(&kv->entry);
			cfg_kv_destroy(kv);
		}
	}

	return EMS_OK;
}

ems_int cfg_set_json(ems_cfg *cfg, ems_cchar *key, json_object *rules)
{
	ems_int i;
	json_object *obj;
	ems_char     buf[128] = {0};

	cfg_del_json(cfg, key);

	if (!rules)
		return EMS_OK;

	ems_assert(rules && json_object_is_type(rules, json_type_array));

	for (i = 0; i < json_object_array_length(rules); i++) {
		obj = json_object_array_get_idx(rules, i);

		if (obj && json_object_is_type(obj, json_type_string)) {
			snprintf(buf, 128, "%s.%d", key, i);
			cfg_set(cfg, buf, json_object_get_string(obj));
		}
	}

	return EMS_OK;
}


static ems_int cfg_get_json_cb(ems_void *arg, ems_cchar *key, ems_cchar *val)
{
	json_object *ary = (json_object *)arg;

	ems_assert(key && val);

	if (key && val)
		json_object_array_add(ary, json_object_new_string(val));

	return EMS_OK;
}

json_object *cfg_get_json(ems_cfg *cfg, ems_cchar *key)
{
	json_object *ary = NULL;

	ary = json_object_new_array();

	cfg_select(cfg, key, (ems_void *)ary, cfg_get_json_cb);

	return ary;
}
