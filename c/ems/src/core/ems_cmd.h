
#ifndef EMS_COMMAND_HEADER___
#define EMS_COMMAND_HEADER___


#define CMD_EMS			0
#define CMD_EMS_C		0x0001
#define CMD_EMS_CTRL		0x0002
#define CMD_EMS_STATUS		0x0003
#define CMD_EMS_QOS		0x0004
#define CMD_EMS_PORTAL		0x0005
#define CMD_EMS_RADIUS		0x0006
#define CMD_EMS_BWLIST		0x0007
#define CMD_EMS_FW		0x0008
#define CMD_EMS_APP		0x0009
#define CMD_EMS_USER		0x000a
#define CMD_EMS_WIRELESS	0x000b
#define CMD_EMS_NETWORK		0x000c
#define CMD_EMS_CONFIG		0x000d
#define CMD_EMS_LAN		0x000e

#define CMD_BRIDGE_BASE		0x1000
#define CMD_BRIDGE_REQ		0x1001
#define CMD_BRIDGE_HB		0x1002
#define CMD_EMS_TEST_RADIUS	0x2000
#define CMD_EMS_LOG		0x2001
#define CMD_EMS_DETAIL		0x2002




#define MSG_ST_REQUEST_ERR		199
#define MSG_ST_CONNECT_EMS_FAILED	200
#define MSG_ST_RESULT_ERROR		201

ems_int exec_cmd(ems_int cmd, json_object *root);
ems_int cmd_parse_cmd(ems_int argc, ems_char **argv, json_object *req);

#define ems_json_reset_key(obj, req, key) \
	obj = json_object_object_get(req, key); \
	if (!obj) break; \
	json_object_object_add(req, key, \
		json_object_new_int(ems_atoi(json_object_get_string(obj))))

#define ems_json_key_atoi(obj, req, key) \
	obj = json_object_object_get(req, key); \
	if (obj) \
		json_object_object_add(req, key, \
		json_object_new_int(ems_atoi(json_object_get_string(obj)))); \
	else \
		ems_l_trace("notice, cfg \"%s\" did not support: (%s:%d) ", key, __FILE__, __LINE__)

#endif
