
#ifndef EMS_APP_HEADER_ALL
#define EMS_APP_HEADER_ALL

typedef struct _app_module_s     app_module;
typedef struct _ems_app_s	 ems_app;

struct _ems_app_s
{
	ems_uint     flg;
	ems_uint     ty;
	ems_str      nick;
	ems_str      rsrv;
	json_object *obj;

	ems_queue   entry;
};


typedef ems_int (*func_send_msg)(ems_uint s, ems_uint d, ems_uint evt, json_object *root);


struct _app_module_s {
	ems_uint  ty;
	ems_str   desc;
	ems_void *ctx;
	ems_void *rsrv;
	ems_uint  flg;

	ems_int (*init)(app_module *mod);
	ems_int (*uninit)(app_module *mod);

	ems_int (*process)(app_module *mod, ems_uint s_mod, ems_uint d_mod, ems_uint evt, json_object *root);
	ems_int (*run)(app_module *mod, ems_int run);
	ems_int (*process_rule)(app_module *mod, json_object *rule);
	ems_int (*version_match)(app_module *mod);
	ems_int (*install)(app_module *mod);
};


ems_app  *ems_app_new();
ems_void  ems_app_destroy(ems_app *app);

ems_int ems_app_attach(ems_app *app);


#define EMS_APP_CMD_RADIUS_AUTH		0x0001
#define EMS_APP_CMD_RADIUS_AUTH_RSP	0x8001
#define EMS_APP_CMD_RADIUS_LOGOUT	0x0002
#define EMS_APP_CMD_PORTAL_LOGOUT	0x0003

#define EMS_APP_EVT_FW_RELOAD		0x0004
#define EMS_APP_RULES_UPDATE		0x0005 /* rule's by hand */
#define EMS_APP_SERVER_RULES_UPDATE	0x0006 /* rules from server*/
#define EMS_APP_EVT_STATUS		0x0007


/*
   for downlink check only
 */
#define EMS_EVT_DOWNLINK_IN		0x0008
#define EMS_EVT_DOWNLINK_NUM		0x0009

#define EMS_APP_RADIUS_STOP		0x000a
#define EMS_APP_RADIUS_START		0x000b

#define EMS_APP_SERVER_BWLIST		0x000d

#define EMS_APP_FW_ADDRESS_ADD		0x000e
#define EMS_APP_FW_ADDRESS_DEL		0x000f
#define EMS_APP_FW_RADIUS_DEVICE_FREE	0x0010
#define EMS_APP_CHECK_SUBDOMAIN		0x0011
#define EMS_APP_CHECK_URL_WHITELIST     0x0012

#define EMS_APP_START	EMS_APP_RADIUS_START
#define EMS_APP_STOP	EMS_APP_RADIUS_STOP

#define EMS_APP_DNS_INTERCEPT_START	0x0013
#define EMS_APP_DNS_INTERCEPT_STOP	0x0014
#define EMS_APP_EMS_STATUS		0x0015
#define EMS_APP_FW_CLEAR		0x0016
#define EMS_APP_FW_SET_NAS_FREE		0x0017
#define EMS_APP_FW_SET_HTTP_FREE	0x0018
#define EMS_APP_FW_SET_GREY_FREE	0x0019

#define EMS_APP_EVT_DETAIL		0x001a
#define EMS_APP_PROCESS_MSGQUEUE	0x001b

#define EMS_APP_EVT_USERINFO		0x001c
#define EMS_APP_EVT_BSSIDINFO		0x001d
#define EMS_APP_EVT_USERNUMBER		0x001e
#define EMS_APP_EVT_USERLIST		0x001f
#define EMS_APP_EVT_USERNAME		0x0020
#define EMS_APP_EVT_WIRED_RELOAD	0x0021
#define EMS_APP_EVT_NETWORK_UP		0x0022
#define EMS_APP_EVT_NETWORK_DOWN	0x0023

app_module *ems_app_modules_new();
ems_void ems_app_module_destroy(app_module *mod);

app_module **ems_app_module_array();
ems_void  ems_app_module_array_destroy(app_module **mod);

ems_int _ems_app_module_load(app_module **mod_ary, ems_app_type ty, ems_void *ctx);
#define ems_app_module_load(ary, ty, ctx) _ems_app_module_load(ary, ty, (ems_void *)ctx)
ems_int ems_app_modules_run(app_module **mod_ary, ems_int run);

ems_int _ems_send_message(ems_queue *q, ems_uint s, ems_uint d, ems_uint evt, json_object *root);
#define ems_send_message(s, d, e, j)	_ems_send_message(&emscorer()->msg_entry, s, d, e, j)

ems_int _ems_app_process(app_module **mod, ems_uint s, ems_uint d, ems_uint evt, json_object *root);
#define ems_app_process(s, d, e, j)	_ems_app_process(core_moduler(), s, d, e, j)

ems_cchar   *ems_app_radius_username(ems_cchar *ip);
json_object *ems_app_radius_userlist();

ems_int     _ems_app_run(app_module **, ems_uint id);
#define ems_app_run(modid)	_ems_app_run(core_moduler(), modid)

#define ems_module_attached(ty,  mod)	((ty *)(mod->rsrv))

#endif
