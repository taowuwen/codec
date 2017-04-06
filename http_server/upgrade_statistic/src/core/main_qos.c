
#include "ems_core.h"
#include "ems_cmd.h"

ems_int main_qos(ems_int cmd, ems_int argc, ems_char **argv)
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
			ems_json_reset_key(obj, req, "enable");
		}

		rtn = exec_cmd(cmd, req);
	} while (0);

	json_object_put(req);

	ems_l_trace("main_qos: %d, rtn= %d\n", cmd, rtn);
	return rtn;
}
