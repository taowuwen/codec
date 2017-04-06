
#include "ems_core.h"
#include "ems_cmd.h"


#if 0
static ems_int bwlist_update_field(ems_cchar *key, json_object *root)
{
	ems_buffer    buf;
	ems_cchar    *ctx;
	ems_int       len;
	json_object *obj, *ary;

	ary = json_object_new_array();

	obj = json_object_object_get(root, key);
	if (!(obj && json_object_is_type(obj, json_type_string))) {

		json_object_object_add(root, key, ary);
		return EMS_ERR;
	}

	ctx = json_object_get_string(obj);

	len = ems_strlen(ctx);
	if (len > 0) {
		ems_buffer_init(&buf, len + 2);

		snprintf(buf_wr(&buf), buf_left(&buf), "%s", ctx);
		{
			ems_char *p, *q;

			p = buf_rd(&buf);


			while (*p && (NULL != (q = strchr(p, ',')))) {
				*q++ = '\0';
				json_object_array_add(ary, json_object_new_string(p));
				p = q;
			}

			if (*p)
				json_object_array_add(ary, json_object_new_string(p));

		}

		ems_buffer_uninit(&buf);
	}

	json_object_object_add(root, key, ary);

	return EMS_OK;
}
#endif


static ems_int bwlist_parse_from_file(json_object *root, ems_cchar *fl)
{
	ems_buffer  buff;
	ems_char   *buf;
	ems_int     len, size;
	FILE *fp = NULL;
	struct json_tokener *tok;
	struct json_object  *jobj, *obj;
	enum   json_tokener_error jerr;

	fp = fopen(fl, "r");
	if (!fp)
		return MSG_ST_REQUEST_ERR;

	tok = json_tokener_new();
	if (!tok) {
		fclose(fp);
		return EMS_ERR;
	}

	ems_buffer_init(&buff, EMS_BUFFER_1K);

	buf  = buf_wr(&buff);
	size = buf_left(&buff);

	jobj = NULL;
	do {
		len = fread(buf, 1, size, fp);
		if (len <= 0)
			break;

		jobj = json_tokener_parse_ex(tok, buf, len);

	} while ((jerr = json_tokener_get_error(tok)) == json_tokener_continue);

	json_tokener_free(tok);
	ems_buffer_uninit(&buff);
	fclose(fp);

	if (!(jobj && (jerr == json_tokener_success))) {
		ems_l_trace("parse err: %s, file: %s", json_tokener_error_desc(jerr), fl);
		if (jobj)
			json_object_put(jobj);
		return MSG_ST_REQUEST_ERR;
	}

	obj = json_object_object_get(jobj, "url");
	if (obj) {
		json_object_object_add(root, "url", 
				ems_json_tokener_parse(json_object_to_json_string(obj)));
	}

	obj = json_object_object_get(jobj, "whitemac");
	if (obj) {
		json_object_object_add(root, "whitemac", 
				ems_json_tokener_parse(json_object_to_json_string(obj)));
	}

	obj = json_object_object_get(jobj, "blackmac");
	if (obj) {
		json_object_object_add(root, "blackmac", 
				ems_json_tokener_parse(json_object_to_json_string(obj)));
	}

	json_object_put(jobj);

	return EMS_OK;
}

ems_int main_bwlist(ems_int cmd, ems_int argc, ems_char **argv)
{
	ems_int      rtn;
	json_object *req, *obj;
	ems_uint     flg;

	req = json_object_new_object();

	cmd_parse_cmd(argc, argv, req);

	do {
		rtn = MSG_ST_REQUEST_ERR;

		obj = json_object_object_get(req, "flag");
		if (!obj) break;

		flg = ems_atoi(json_object_get_string(obj));
		json_object_object_add(req, "flag", json_object_new_int(flg));

		obj = json_object_object_get(req, "method");
		if (!obj) break;

		if (!strcasecmp(json_object_get_string(obj), "set")) 
		{
#if 0
			if (ems_flag_like(flg, 0x01 << 0)) {
				bwlist_update_field("url", req);
			}

			if (ems_flag_like(flg, 0x01 << 1)) {
				bwlist_update_field("whitemac", req);
			}

			if (ems_flag_like(flg, 0x01 << 2)) {
				bwlist_update_field("blackmac", req);
			}
#else

			obj = json_object_object_get(req, "path");
			if (!obj) break;

			rtn = bwlist_parse_from_file(req, json_object_get_string(obj));
			if (rtn != EMS_OK) break;

			json_object_object_del(req, "path");
#endif
		}

		rtn = exec_cmd(cmd, req);
	} while (0);

	json_object_put(req);

	ems_l_trace("main_c: %d, rtn= %d\n", cmd, rtn);
	return rtn;
}
