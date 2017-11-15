
#ifndef EMS_CLIENT_CORE_HEADER_____
#define EMS_CLIENT_CORE_HEADER_____

#include "ems_main.h"
#include "json.h"

struct json_object *ems_json_tokener_parse(ems_cchar *str);


#define ems_json_get_int64_def(root, key, val, def) do {		\
	struct json_object *_obj;					\
	_obj = json_object_object_get(root, key);			\
	if (_obj && json_object_is_type(_obj, json_type_int))		\
		(val) = json_object_get_int64(_obj);			\
	else 								\
		(val) = (def);						\
} while (0)

#define  ems_json_get_int64(root, key, val) do {  			\
	struct json_object *_obj;					\
	_obj = json_object_object_get(root, key);			\
	if (_obj && json_object_is_type(_obj, json_type_int))		\
		(val) = json_object_get_int64(_obj);			\
	else								\
		return MSG_ST_INVALID_ARG;				\
} while (0)

#define ems_json_get_int_def(root, key, val, def) do {			\
	struct json_object *_obj;					\
	_obj = json_object_object_get(root, key);			\
	if (_obj && json_object_is_type(_obj, json_type_int))		\
		(val) = json_object_get_int(_obj);			\
	else 								\
		(val) = (def);						\
} while (0)

#define  ems_json_get_int(root, key, val) do {  			\
	struct json_object *_obj;					\
	_obj = json_object_object_get(root, key);			\
	if (_obj && json_object_is_type(_obj, json_type_int))		\
		(val) = json_object_get_int(_obj);			\
	else								\
		return MSG_ST_INVALID_ARG;				\
} while (0)

#define  ems_json_get_string_def(root, key, str, def) do {		\
	struct json_object *_obj;					\
	_obj = json_object_object_get(root, key);			\
	if (_obj && json_object_is_type(_obj, json_type_string))	\
		str_set((str), json_object_get_string(_obj));		\
	else								\
		str_set((str), (def));					\
} while (0)

#define  ems_json_get_string(root, key, str) do {  			\
	struct json_object *_obj;					\
	_obj = json_object_object_get(root, key);			\
	if (_obj && json_object_is_type(_obj, json_type_string))	\
		str_set((str), json_object_get_string(_obj));		\
	else								\
		return MSG_ST_INVALID_ARG;				\
} while (0)


#endif
