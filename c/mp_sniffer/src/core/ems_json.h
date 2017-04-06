#ifndef EMS_JSON_HEADER___V1
#define EMS_JSON_HEADER___V1

#include "json.h"

struct json_object *ems_json_tokener_parse(ems_cchar *str);
struct json_object *ems_json_tokener_parse_ex(ems_cchar *str, ems_int lstr);


#define ems_json_get_int64_def(root, key, val, def) do {		\
	struct json_object *obj;					\
	obj = json_object_object_get(root, key);			\
	if (obj && json_object_is_type(obj, json_type_int))		\
		(val) = json_object_get_int64(obj);			\
	else 								\
		(val) = (def);						\
} while (0)

#define  ems_json_get_int64(root, key, val) do {  			\
	struct json_object *obj;					\
	obj = json_object_object_get(root, key);			\
	if (obj && json_object_is_type(obj, json_type_int))		\
		(val) = json_object_get_int64(obj);			\
	else								\
		return MSG_ST_INVALID_ARG;				\
} while (0)

#define ems_json_get_int_def(root, key, val, def) do {			\
	struct json_object *obj;					\
	obj = json_object_object_get(root, key);			\
	if (obj && json_object_is_type(obj, json_type_int))		\
		(val) = json_object_get_int(obj);			\
	else 								\
		(val) = (def);						\
} while (0)

#define  ems_json_get_int(root, key, val) do {  			\
	struct json_object *obj;					\
	obj = json_object_object_get(root, key);			\
	if (obj && json_object_is_type(obj, json_type_int))		\
		(val) = json_object_get_int(obj);			\
	else								\
		return MSG_ST_INVALID_ARG;				\
} while (0)

#define  ems_json_get_string_def(root, key, str, def) do {		\
	struct json_object *obj;					\
	obj = json_object_object_get(root, key);			\
	if (obj && json_object_is_type(obj, json_type_string))		\
		str_set((str), json_object_get_string(obj));		\
	else								\
		str_set((str), (def));					\
} while (0)

#define  ems_json_get_string(root, key, str) do {  			\
	struct json_object *obj;					\
	obj = json_object_object_get(root, key);			\
	if (obj && json_object_is_type(obj, json_type_string))		\
		str_set((str), json_object_get_string(obj));		\
	else								\
		return MSG_ST_INVALID_ARG;				\
} while (0)

#define ems_json_object_to_string_cmp(j1, j2)	\
	strcmp(json_object_to_json_string(j1), json_object_to_json_string(j2))


#endif
