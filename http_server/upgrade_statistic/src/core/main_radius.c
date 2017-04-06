
#include "ems_core.h"
#include "ems_cmd.h"

ems_int main_radius(ems_int cmd, ems_int argc, ems_char **argv)
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
			ems_json_reset_key(obj, req, "auto");

			if (!json_object_get_int(json_object_object_get(req, "auto"))) 
			{
				obj = json_object_object_get(req, "addr");
				if (!obj) break;

				obj = json_object_object_get(req, "secret");
				if (!obj) break;

				ems_json_reset_key(obj, req, "authport");
				ems_json_reset_key(obj, req, "acctport");
				ems_json_reset_key(obj, req, "rp_period");
				ems_json_reset_key(obj, req, "retry_times");
				ems_json_reset_key(obj, req, "retry_timeout");
			}
		}

		rtn = exec_cmd(cmd, req);
	} while (0);

	json_object_put(req);

	ems_l_trace("main_radius: %d, rtn= %d\n", cmd, rtn);
	return rtn;
}
