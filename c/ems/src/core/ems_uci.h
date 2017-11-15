
#ifndef EMS_UCI_SECTION__HEADER___
#define EMS_UCI_SECTION__HEADER___

ems_int ems_uci_load_cfg(ems_cchar *cfg, ems_cchar *search, json_object *ary);
ems_int ems_uci_write_cfg(ems_cchar *cfg, ems_cchar *search, json_object *ary);

#define UCI_CFG_WIRELESS	"wireless"
#define UCI_SEARCH_PATH		"/tmp/state"
#define UCI_SECTION_NAME	"section_name"
#define UCI_SECTION_TYPE	"section_type"

#define UCI_DEFAULT_ID		1

typedef ems_int (*cb_ucisection)(ems_void *arg, json_object *obj);
ems_int ems_uci_foreach(json_object *ary, cb_ucisection, ems_void *arg);

ems_int ems_uci_remove_section(ems_cchar *cfg, ems_cchar *section);
json_object *ems_uci_find_ssid(json_object *ary, ems_int id);

#endif
