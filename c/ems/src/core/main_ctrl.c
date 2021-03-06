#include "ems_core.h"
#include "ems_cmd.h"


ems_int main_ctrl(ems_int cmd, ems_int argc, ems_char **argv)
{
	ems_int      rtn;
	json_object *req, *obj;

	req = json_object_new_object();

	cmd_parse_cmd(argc, argv, req);

	do {
		rtn = MSG_ST_REQUEST_ERR;

		obj = json_object_object_get(req, "method");
		if (!obj) break;

		if (!strcasecmp(json_object_get_string(obj), "set")) 
		{
			obj = json_object_object_get(req, "auto");
			if (!obj) break;
			json_object_object_add(req, "auto", 
					json_object_new_int(ems_atoi(json_object_get_string(obj))));

			obj = json_object_object_get(req, "run");
			if (!obj) break;

			json_object_object_add(req, "run", 
					json_object_new_int(ems_atoi(json_object_get_string(obj))));

		}

		rtn = exec_cmd(cmd, req);
	} while (0);

	json_object_put(req);

	ems_l_trace("main_c: %d, rtn= %d\n", cmd, rtn);
	return rtn;
}
