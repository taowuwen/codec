
#ifndef DT_NET_STRING_CONTROL_HEADER___
#define DT_NET_STRING_CONTROL_HEADER___

#include "dt_types.h"

typedef struct {
	dt_uint		 len;
	dt_uchar	*data;
} dt_str;


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


dt_int  str_set(dt_str *str, dt_cchar *text);
dt_void str_clear(dt_str *str);

#define str_uninit		str_clear
#define str_init(str)		(str)->len = 0; (str)->data = NULL
#define str_text(str)		(dt_cchar *)((str)->data)
#define str_len(str)		(str)->len

#define str_cpy(dst, src) 	str_set(dst, str_text(src))

/*
 * for read only strings.
 * */
#define dt_string(str)	{ sizeof(str) - 1, (dt_uchar *)str }
#define dt_null_string	{0, NULL}
#define dt_str_set(str, text)	\
		(str)->len = strlen(text); (str)->data =(dt_uchar *)text



#endif
