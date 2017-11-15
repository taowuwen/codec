
#include "ems_core.h"
#include <uci.h>
#include "ems_split.h"
#include "ems_uci.h"
#include "ems_client.h"


static ems_int ems_uci_load_section(struct uci_section *s, json_object *ary)
{
	json_object *obj;
	struct uci_element *e;
	struct uci_option  *o;

	obj = json_object_new_object();
	if (!obj)
		return EMS_ERR;

	json_object_object_add(obj, UCI_SECTION_NAME,  json_object_new_string(s->e.name));
	json_object_object_add(obj, UCI_SECTION_TYPE,  json_object_new_string(s->type));

	uci_foreach_element(&s->options, e) {
		o = uci_to_option(e);

		if (o->type != UCI_TYPE_STRING) 
			continue;

		json_object_object_add(obj, o->e.name, json_object_new_string(o->v.string)); 
	}

	json_object_array_add(ary, obj);

	return EMS_OK;
}

static ems_int 
uci_load_cfg(struct uci_context *ctx, ems_cchar *cfg, json_object *ary)
{
	struct uci_package *pkg;
	struct uci_element *e;

	ems_assert(ctx && cfg && ary);
	if (!(ctx && cfg && ary))
		return EMS_ERR;

	if (uci_load(ctx, cfg, &pkg)) {
		return EMS_ERR;
	}

	uci_foreach_element(&pkg->sections, e) {
		ems_uci_load_section(uci_to_section(e), ary);
	}

	uci_unload(ctx, pkg);

	return EMS_OK;
}

ems_int ems_uci_load_cfg(ems_cchar *cfg, ems_cchar *search, json_object *ary)
{
	ems_int      rtn;
	struct uci_context *ctx;

	ctx = uci_alloc_context();
	if (!ctx) 
		return EMS_ERR;

	if (search)
		uci_add_delta_path(ctx, search);

	rtn = uci_load_cfg(ctx, cfg, ary);

	uci_free_context(ctx);

	return rtn;
}


static ems_int 
ems_uci_set_option(struct uci_context *ctx, ems_char *tuple)
{
	struct uci_ptr ptr;


	if (uci_lookup_ptr(ctx, &ptr, tuple, true) != UCI_OK) {
		return EMS_ERR;
	}

	uci_set(ctx, &ptr);

	return EMS_OK;
}

static ems_int 
ems_uci_append_section(json_object *jsection, 
		       struct uci_context *ctx, 
		       struct uci_package *pkg,
		       ems_cchar *ty)
{
	struct uci_section *s = NULL;

	ems_assert(jsection && ctx && pkg && ty);

	uci_add_section(ctx, pkg, ty, &s);

	if (!s)
		return EMS_ERR;

	json_object_object_add(jsection, UCI_SECTION_NAME,  json_object_new_string(s->e.name));

	return EMS_OK;
}

static ems_int
ems_uci_update_section_option(struct uci_context *ctx,
			      struct uci_package *pkg,
			      json_object *jsection)
{
	ems_buffer  *buf = core_buffer();
	ems_char    *p;
	ems_cchar   *v;
	ems_int      l;
	ems_str sname;
	ems_str pname;

	str_init(&sname);
	str_init(&pname);

	ems_json_get_string_def(jsection,  UCI_SECTION_NAME, &sname, NULL);
	str_set(&pname, pkg->e.name);

	p = buf_wr(buf);
	l = buf_left(buf);

	ems_assert(str_len(&sname) > 0 && str_len(&pname) > 0);
	{
		json_object_object_foreach(jsection, key, val) {
			if (!json_object_is_type(val, json_type_string))
				continue;

			v = json_object_get_string(val);

			if (!v) continue;
			if (!strcmp(UCI_SECTION_NAME, key)) continue;
			if (!strcmp(UCI_SECTION_TYPE, key)) continue;

			snprintf(p, l, "%s.%s.%s=%s", 
					str_text(&pname), str_text(&sname), key,  v);

			if (ems_uci_set_option(ctx, p) != EMS_OK) {
				break;
			}
		}
	}

	str_uninit(&sname);
	str_uninit(&pname);

	return EMS_OK;
}

static ems_int 
ems_uci_update_section(json_object *jsection, struct uci_context *ctx, struct uci_package *pkg)
{
	ems_str sname;
	ems_str stype;

	str_init(&sname);
	str_init(&stype);

	do {
		ems_json_get_string_def(jsection,  UCI_SECTION_NAME, &sname, NULL);
		ems_json_get_string_def(jsection,  UCI_SECTION_TYPE, &stype, NULL);

		if (str_len(&stype) <= 0) { break; }
		if (str_len(&sname) <= 0) {
			ems_uci_append_section(jsection, ctx, pkg, str_text(&stype));

			ems_json_get_string_def(jsection,  UCI_SECTION_NAME, &sname, NULL);
			if (str_len(&sname) <= 0) {
				break;
			}
		}

		ems_uci_update_section_option(ctx, pkg, jsection);
	} while (0);

	str_uninit(&sname);
	str_uninit(&stype);

	return EMS_OK;
}

static ems_int 
ems_uci_update_cfg(struct uci_context *ctx, ems_cchar *cfg, json_object *ary)
{
	json_object *obj;
	ems_int      i;
	struct uci_package *pkg;

	ems_assert(ctx && cfg && ary);
	if (!(ctx && cfg && ary))
		return EMS_ERR;

	if (uci_load(ctx, cfg, &pkg)) {
		return EMS_ERR;
	}

	for (i = 0; i < json_object_array_length(ary); i++) {

		obj = json_object_array_get_idx(ary, i);

		if (obj && json_object_is_type(obj, json_type_object)) {
			if (ems_uci_update_section(obj, ctx, pkg) != EMS_OK) {
				break;
			}
		}
	}

	uci_save(ctx, pkg);
	uci_commit(ctx, &pkg, true);
	uci_unload(ctx, pkg);

	return EMS_OK;
}


ems_int ems_uci_write_cfg(ems_cchar *cfg, ems_cchar *search, json_object *ary)
{
	ems_int      rtn;
	struct uci_context *ctx;

	ctx = uci_alloc_context();
	if (!ctx) 
		return EMS_ERR;

	if (search)
		uci_add_delta_path(ctx, search);

	rtn = ems_uci_update_cfg(ctx, cfg, ary);

	uci_free_context(ctx);

	return rtn;
}


ems_int ems_uci_foreach(json_object *ary, cb_ucisection cb, ems_void *arg)
{
	ems_int      ret;
	json_object *obj;
	ems_int      i;

	if (!(ary && cb))
		return EMS_ERR;

	for (i = 0; i < json_object_array_length(ary); i++) {

		obj = json_object_array_get_idx(ary, i);

		ret = cb(arg, obj);
		if (ret != EMS_OK)
			break;
	}

	return ret;
}

ems_int ems_uci_remove_section(ems_cchar *cfg, ems_cchar *section)
{
	struct uci_context *ctx;
	struct uci_ptr      ptr;
	ems_char cmd[512];
	ems_int  ret;

	ret = EMS_ERR;
	ctx = uci_alloc_context();
	if (!ctx) 
		return EMS_ERR;

	snprintf(cmd, 512, "%s.%s", cfg, section);

	if (uci_lookup_ptr(ctx, &ptr, cmd, true) != UCI_OK) {
		goto err_out;
	}

	uci_delete(ctx, &ptr);
	uci_save(ctx,    ptr.p);
	uci_commit(ctx, &ptr.p, true);

	ret = EMS_OK;

err_out:
	uci_free_context(ctx);

	return ret;
}

json_object *ems_uci_find_ssid(json_object *ary, ems_int iface_id)
{
	json_object *jobj;
	ems_int      i;
	ems_str      id;

	ems_assert(ary && iface_id > 0);

	str_init(&id);
	for (i = 0; i < json_object_array_length(ary); i++) {

		jobj = json_object_array_get_idx(ary, i);

		ems_assert(jobj != NULL);

		ems_json_get_string_def(jobj, "ems_iface_id", &id, NULL);

		if (str_len(&id) > 0 && (ems_atoi(str_text(&id)) == iface_id)) {
			str_uninit(&id);
			return jobj;
		}
	}

	str_uninit(&id);

	return NULL;
}
