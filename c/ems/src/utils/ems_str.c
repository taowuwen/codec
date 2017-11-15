
#include "ems.h"

ems_int str_set(ems_str *str, ems_cchar *text)
{
	if (str) {
		if (str->data)
			ems_free(str->data);

		if (text) {
			str->len  = strlen(text);
			str->data = (ems_uchar*)ems_strdup(text);
		} else {
			str->len  = 0;
			str->data = NULL;
		}
	} 

	return OK;
}

ems_void str_clear(ems_str *str)
{
	str_set(str, NULL);
}

