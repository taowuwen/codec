
#include "ems_core.h"
#include "ems_cmd.h"

/*

   { 
   "device": [
   	{"sn": sn, "flag": flg, "upgs": 
		[
			{ "ip": ip, "ty": rom type, "ver": "rom version", "status":"status",
			  "create": create time, "access": "access", "flag": flg },
			  ...
		] 
	},

	...
   ]
   }
 */




/*
output:


sn,flg(should trans), (ip, rom, ver, status, createtime, accesstime, flg),...

 */

#if 0
static ems_int stat_print_one(json_object *device)
{
	printf("not finished yet");

	return EMS_OK;
}

static ems_int print_stat_info()
{
	ems_int i;
	struct json_object *rsp = NULL, *ary = NULL, *obj;

	rsp = json_parse_from_file(JSON_RESULT);

	if (!rsp) {
		fprintf(stderr, "json_parse_from_file : %s failed", JSON_RESULT);
		return EMS_ERR;
	}

	ary = json_object_object_get(rsp, "device");

	if (!(ary && json_object_is_type(ary, json_type_array)))
		return EMS_ERR;

	for (i = 0; i < json_object_array_length(ary); i++) {
		obj = json_object_array_get_idx(ary, i);

		if (obj && json_object_is_type(obj, json_type_object)) 
		{
			stat_print_one(obj);
		}
	}

	json_object_put(rsp);

	return EMS_OK;
}
#endif

ems_int main_statistic(ems_int cmd, ems_int argc, ems_char **argv)
{
	ems_int      rtn;
	json_object *req, *obj;

	req = json_object_new_object();

	cmd_parse_cmd(argc, argv, req);

	do {
		rtn = MSG_ST_REQUEST_ERR;

		obj = json_object_object_get(req, "method");
		if (!obj) break;

		rtn = exec_cmd(cmd, req);
	} while (0);

	json_object_put(req);

	ems_l_trace("main_statistic: %d, rtn= %d\n", cmd, rtn);
	return rtn;
}
