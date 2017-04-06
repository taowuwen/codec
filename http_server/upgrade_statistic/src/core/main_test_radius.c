
#include "ems_core.h"
#include "ems_cmd.h"

ems_int main_test_radius(ems_int cmd, ems_int argc, ems_char **argv)
{
	ems_int      rtn;
	json_object *req, *obj;

	req = json_object_new_object();

	cmd_parse_cmd(argc, argv, req);

	do {
		rtn = MSG_ST_REQUEST_ERR;

		obj = json_object_object_get(req, "method");
		if (!obj) break;

		obj = json_object_object_get(req, "username");
		if (!obj) break;

		obj = json_object_object_get(req, "userpass");
		if (!obj) break;

		obj = json_object_object_get(req, "userip");
		if (!obj) break;

		obj = json_object_object_get(req, "usermac");
		if (!obj) break;

		rtn = exec_cmd(cmd, req);
	} while (0);

	json_object_put(req);

	ems_l_trace("main_test_radius: %d, rtn= %d\n", cmd, rtn);
	return rtn;
}
