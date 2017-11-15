
#ifndef EMS_CONFIG___HEADER___
#define EMS_CONFIG___HEADER___

typedef struct _ems_config_s  ems_cfg;

struct _ems_config_s {
	ems_str    fl;
	ems_queue  kv_entry;
};

ems_int  cfg_init(ems_cfg *cfg, ems_cchar *fl);
ems_void cfg_uninit(ems_cfg *cfg);

ems_int    cfg_read(ems_cfg *cfg);
ems_int    cfg_write(ems_cfg *cfg);
ems_int    cfg_flush(ems_cfg *cfg);
ems_int    cfg_set(ems_cfg *cfg, ems_cchar *key, ems_cchar *val);
ems_cchar *cfg_get(ems_cfg *cfg, ems_cchar *key);


/*
   for find cfg's like some key

   e.g.

   key
   key.0
   key.1
   key.2
   key.3


   cfg_select(cfg, "key", arg, cb);

   which gonna invoke cb for 5 times, 
   key, key.0, key.1, key.2 key.3, key.4
 */
typedef ems_int (*cfg_search_cb)(ems_void *arg, ems_cchar *key, ems_cchar *val);
ems_int cfg_select(ems_cfg *cfg, ems_cchar *like, ems_void *arg, cfg_search_cb cb);

json_object *cfg_get_json(ems_cfg *cfg, ems_cchar *key);
ems_int cfg_set_json(ems_cfg *cfg, ems_cchar *key, json_object *rules);


/*
   for iNM
 */
#define CFG_ems_s_addr		"ems_s.addr"
#define CFG_ems_s_port		"ems_s.port"

#define CFG_radius_dict		"ems.radius.dictionay"

/*
   for user defined
 */

#define CFG_user_mac_white	"user.mac_white"
#define CFG_user_mac_black	"user.mac_black"


/*
   cfg for client
 */
#define CFG_client_retry_period		"client.retry_period"
#define CFG_client_getconf_period	"client.getconf_period"
#define CFG_client_upt_period		"client.upt_period"
#define CFG_client_subdomain_enable	"client.enable_subdomain"
#define CFG_client_autoupdate		"client.autoupdate"

/*
   cfg for log
 */
#define CFG_log_level			"ctrl.log.level"
#define CFG_network_detect_escape	"network.escape"

/*
   cfg for online management
 */

#define CFG_tunnel_enable		"tunnel.enable"
#define CFG_tunnel_address		"tunnel.addr"
#define CFG_tunnel_port			"tunnel.port"
#define CFG_tunnel_heartbeat		"tunnel.heartbeat"


#endif
