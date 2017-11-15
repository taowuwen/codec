
#include "ems_core.h"
#include "ems_client.h"
#include "app.h"
#include "app_nic.h"
#include "ems_fw.h"
#include "ems_dns.h"
#include "ems_http.h"
#include "ems_split.h"


struct _http_header_s {
	ems_str  _ary[HTTP_MAX];
};


http_header *http_header_new()
{
	return (http_header *)ems_malloc(sizeof(http_header));
}

ems_void http_header_destroy(http_header *hdr)
{
	if (hdr) {
		http_header_uninit(hdr);
		ems_free(hdr);
	}
}

ems_void http_header_init(http_header *hdr)
{
	http_request_key ty;

	ems_assert(hdr != NULL);
	
	for (ty = HTTP_MIN; ty < HTTP_MAX; ty++) {
		str_init(&hdr->_ary[ty]);
	}
}

ems_void http_header_uninit(http_header *hdr)
{
	http_request_key ty;
	
	ems_assert(hdr != NULL);
	for (ty = HTTP_MIN; ty < HTTP_MAX; ty++) {
		str_uninit(&hdr->_ary[ty]);
	}
}

ems_cchar *http_header_get(http_header *hdr, http_request_key ty)
{
	ems_assert(ty >= HTTP_MIN && ty < HTTP_MAX);

	if (ty >= HTTP_MIN && ty < HTTP_MAX) 
		return str_text(&hdr->_ary[ty]);

	return NULL;
}

ems_cchar *http_header_set(http_header *hdr, http_request_key ty, ems_cchar *str)
{
	if (ty >= HTTP_MIN && ty < HTTP_MAX) {
		str_set(&hdr->_ary[ty], str);
		return str_text(&hdr->_ary[ty]);
	}

	return NULL;
}

typedef struct {
	http_request_key key;
	ems_cchar       *val;
} http_request_key_map; 


static http_request_key_map _gkeymap[] = {
	{HTTP_Host,      "Host"},
	{HTTP_UserAgent, "User-Agent"},
	{HTTP_MAX,        NULL}
};

static http_request_key http_request_key_get(ems_cchar *val)
{
	http_request_key_map *map = NULL;

	ems_assert(val != NULL);

	if (ems_strlen(val) <= 0)
		return HTTP_MAX;

	for (map = &_gkeymap[0]; map->val != NULL; map++) {
		if (!strcmp(val, map->val))
			return map->key;
	}

	return HTTP_MAX;
}

static ems_cchar *http_request_val_get(http_request_key key)
{
	http_request_key_map *map = NULL;

	ems_assert(key >= HTTP_MIN && key < HTTP_MAX);

	for (map = &_gkeymap[0]; map->val != NULL; map++) {
		if (map->key == key)
			return map->val;
	}

	return "->";
}

static ems_int http_parse_line(http_header *hdr, ems_char *line)
{
	ems_queue list;

	ems_queue_init(&list);

	if (!http_header_get(hdr, HTTP_Method)) {
		ems_string_split(&list, line, " ");

		if (ems_split_len(&list) == 3) {
			http_header_set(hdr, HTTP_Method,  ems_split_get_str(&list, 0));
			http_header_set(hdr, HTTP_Param,   ems_split_get_str(&list, 1));
			http_header_set(hdr, HTTP_Version, ems_split_get_str(&list, 2));
		}
	} else {
		ems_string_split(&list, line, ": ");

		if (ems_split_len(&list) == 2) {
			http_header_set(hdr, 
					http_request_key_get(ems_split_get_str(&list, 0)),
					ems_split_get_str(&list, 1));
		}
	}

	ems_split_clear(&list);

	return EMS_OK;
}

static ems_void http_header_print(http_header *hdr)
{
	http_request_key key;

	for (key = HTTP_MIN; key < HTTP_MAX; key++) {
		ems_l_trace("[http] (%02d) %s: %s", 
			key, http_request_val_get(key), http_header_get(hdr, key));
	}
}

ems_int http_header_parse(http_header *hdr, ems_cchar *reqctx)
{
	ems_cchar *p, *end;
	ems_char  *line;
	ems_int    len;
	ems_buffer *buf = core_buffer();

	ems_assert(hdr && reqctx);
	ems_assert(http_msg_is_web(reqctx));
	ems_assert(http_header_full(reqctx));

	http_header_clear(hdr);

	end = strstr(reqctx, HTTP_NEWLINE HTTP_NEWLINE);
	ems_assert(end != NULL);

	line = buf_head(buf);

	while (reqctx < end) {

		p = strstr(reqctx, HTTP_NEWLINE);
		ems_assert(p != NULL);

		len = abs(p - reqctx);

		if (buf_left(buf) > len) {
			ems_buffer_write(buf, reqctx, len);

			*(line + len) = '\0';
			http_parse_line(hdr, line);

			ems_buffer_clear(buf);
		}

		reqctx = p + strlen(HTTP_NEWLINE);
	}

	http_header_print(hdr);

	return EMS_OK;
}

ems_int http_header_full(ems_cchar *reqctx)
{
	if (strstr(reqctx, HTTP_NEWLINE HTTP_NEWLINE))
		return EMS_YES;

	return EMS_NO;
}

ems_int http_msg_is_web(ems_cchar *msg)
{
	if (!strncmp(msg, HTTP_GET,  3))
		return EMS_YES;

	if (!strncmp(msg, HTTP_POST, 4))
		return EMS_YES;

	return EMS_NO;
}
