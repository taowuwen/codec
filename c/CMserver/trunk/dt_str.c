
#include "dt.h"

dt_int str_set(dt_str *str, dt_cchar *text)
{
	if (str) {
		if (str->data)
			dt_free(str->data);

		if (text) {
			str->len  = strlen(text);
			str->data = (dt_uchar*)strdup(text);
		} else {
			str->len  = 0;
			str->data = NULL;
		}
	} 

	return OK;
}

dt_void str_clear(dt_str *str)
{
	str_set(str, NULL);
}

