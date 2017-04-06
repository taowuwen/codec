
#ifndef EMS_APP_HEADER_ALL
#define EMS_APP_HEADER_ALL



typedef struct _app_module_s    app_module;
typedef struct _ems_app_s	ems_app;


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
ems_app  *ems_app_find(ems_cchar *key);

ems_int  ems_app_modules_init();
ems_int  ems_app_modules_uninit();
ems_int  ems_app_modules_run(ems_int run);
ems_int  ems_applist_process(json_object *root);
ems_int  ems_applist_process_evt(ems_uint evt, json_object *root);

ems_int ems_app_attach(ems_app *app);
ems_int ems_app_detach(ems_app *app);
ems_int ems_app_process(ems_uint s, ems_uint d, ems_uint evt, json_object *root);

ems_cchar *ems_app_radius_username(ems_cchar *ip);
ems_int    ems_app_radius_user_number();
json_object *ems_app_radius_userlist();

ems_int ems_app_run(ems_uint id);


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
#define EMS_APP_PORTAL_ADDRESS		0x000c

#define EMS_APP_SERVER_BWLIST		0x000d

#define EMS_APP_FW_ADDRESS_ADD		0x000e
#define EMS_APP_FW_ADDRESS_DEL		0x000f
#define EMS_APP_FW_RADIUS_DEVICE_FREE	0x0010
#define EMS_APP_CHECK_SUBDOMAIN		0x0011
#define EMS_APP_CHECK_PARAM_APPLE_COM   0x0012

#define EMS_APP_START	EMS_APP_RADIUS_START
#define EMS_APP_STOP	EMS_APP_RADIUS_STOP

#define EMS_APP_DNS_INTERCEPT_START	0x0013
#define EMS_APP_DNS_INTERCEPT_STOP	0x0014
#define EMS_APP_EMS_STATUS		0x0015
#define EMS_APP_FW_CLEAR		0x0016


#endif
