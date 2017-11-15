
#ifndef EMS_NET_STRING_CONTROL_HEADER___
#define EMS_NET_STRING_CONTROL_HEADER___

#include "ems_types.h"

typedef struct {
	ems_uint	 len;
	ems_uchar	*data;
} ems_str;


#ifdef WIN32

#if defined (_MSC_VER) && (_MSC_VER >= 1500)
	#define snprintf(A, B, C, ...)	\
			_snprintf_s(A, B, _TRUNCATE, C, ##__VA_ARGS__)
	#define vsnprintf(A,B,C,...) \
				_vsnprintf_s(A, B, _TRUNCATE, C, ##__VA_ARGS__)
#else
	#define snprintf	_snprintf
	#define vsnprintf	_vsnprintf
#endif

	#define strncasecmp 	_strnicmp
	#define strcasecmp	_stricmp
#endif


ems_int  str_set(ems_str *str, ems_cchar *text);
ems_void str_clear(ems_str *str);

#define str_uninit		str_clear
#define str_init(str)		(str)->len = 0; (str)->data = NULL
#define str_text(str)		(ems_cchar *)((str)->data)
#define str_len(str)		(str)->len

#define str_cpy(dst, src) 	str_set(dst, str_text(src))

/*
 * for read only strings.
 * */
#define ems_string(str)	{ sizeof(str) - 1, (ems_uchar *)str }
#define ems_null_string	{0, NULL}
#define ems_str_set(str, text)	\
		(str)->len = strlen(text); (str)->data =(ems_uchar *)text



#endif
