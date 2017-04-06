
#include "ems_common.h"
#include "ems_split.h"

ems_split *ems_split_new()
{
	ems_split *sp = NULL;


	sp = (ems_split *)ems_malloc(sizeof(ems_split));
	if (sp) {
		memset(sp, 0, sizeof(ems_split));
		ems_queue_init(&sp->entry);
		str_init(&sp->str);
	}

	return sp;
}

ems_void ems_split_destroy(ems_split *sp)
{
	if (sp) {
		str_uninit(&sp->str);
		ems_free(sp);
	}
}


ems_int ems_string_split(ems_queue *list, ems_cchar *src, ems_cchar *sep)
{
	ems_cchar *p;
	ems_char  *q;
	ems_split *sp = NULL;
	ems_char   buf[512];
	ems_int    len = 0;

	if (!(list && src && sep))
		return EMS_ERR;

	p = src;
	q = NULL;

	while(NULL != (q = strstr(p, sep))) {
		len = abs(p - q);
		if (len >= 512)
			len = 511;

		memcpy(buf, p, len);
		buf[len] = '\0';

		sp = ems_split_new();
		if (sp) {
			str_set(&sp->str, buf);
			ems_queue_insert_tail(list, &sp->entry);
		}

		p = q + strlen(sep);
	}

	if (*p) {
		sp = ems_split_new();
		if (sp) {
			str_set(&sp->str, p);
			ems_queue_insert_tail(list, &sp->entry);
		}
	}

	return EMS_OK;
}


ems_void ems_split_clear(ems_queue *list)
{
	ems_queue_clear(list, ems_split, entry, ems_split_destroy);
}

ems_split *ems_split_find(ems_queue *list, ems_cchar *key)
{
	ems_split *sp;
	ems_queue *p;

	ems_queue_foreach(list, p) {
		sp = ems_container_of(p, ems_split, entry);

		if (str_len(&sp->str) > 0 && !strcmp(key, str_text(&sp->str))) 
			return sp;
	}

	return NULL;
}

ems_cchar *ems_split_get_str(ems_queue *list, ems_int ind)
{
	ems_int    i = 0;
	ems_queue *p;
	ems_split *sp;

	ems_queue_foreach(list, p) {

		sp = ems_container_of(p, ems_split, entry);

		if (i == ind)
			return str_text(&sp->str);
		i++;
	}

	return NULL;
}

ems_int ems_split_len(ems_queue *list)
{
	ems_int len;
	ems_queue_len(list, len);
	return len;
}

ems_int ems_split_foreach(ems_queue *list,  psplit_cb cb, ems_void *arg)
{
	ems_int    ret;
	ems_queue *p;
	ems_split *sp;

	if (!(list && cb))
		return EMS_ERR;

	ret = EMS_OK;
	ems_queue_foreach(list, p) {

		sp = ems_container_of(p, ems_split, entry);

		ret = cb(arg, str_text(&sp->str));
		if (ret != EMS_OK)
			break;
	}

	return ret;
}


