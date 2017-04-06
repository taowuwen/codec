
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
   for url download
 */

#define CFG_download_url_ios		"ems_down.ios.url"
#define CFG_download_url_android	"ems_down.android.url"
#define CFG_download_url_winphone	"ems_down.winphone.url"

/*
   for iNM
 */

#define CFG_ems_version		"ems.version"
#define CFG_ems_system_version	"ems.system.version"
#define CFG_ems_sn		"ems.sn"
#define CFG_ems_mac		"ems.mac"
#define CFG_ems_devicetype	"ems.devicetype"

#define CFG_ems_c_addr		"ems_c.addr"
#define CFG_ems_c_port		"ems_c.port"
#define CFG_ems_c_auto		"ems_c.auto"

#define CFG_ems_s_addr		"ems_s.addr"
#define CFG_ems_s_port		"ems_s.port"

#define CFG_ems_s_sn		CFG_ems_sn
#define CFG_ems_s_code		CFG_ems_sn
#define CFG_ems_s_acname	CFG_ems_sn
#define CFG_ems_s_mac		CFG_ems_mac

#define CFG_radius_dict		"ems.radius.dictionay"

/*
   for user defined
 */

#define CFG_user_url_white	"user.url_white"
#define CFG_user_mac_white	"user.mac_white"
#define CFG_user_mac_black	"user.mac_black"

/*
   cfg for app portal
 */

#define CFG_app_portal_addr	"app.portal.addr" // www.portal.com
#define CFG_app_portal_port	"app.portal.port" // 2000 
#define CFG_app_portal_redirect	"app.portal.redirect" // 80
#define CFG_app_portal_reg	"app.portal.reg"  // 600
#define CFG_app_portal_hb	"app.portal.hb"   // 30
#define CFG_app_portal_auto	"app.portal.auto" // EMS_YES

/*
   cfg for app radius
 */
#define CFG_app_radius_auto		"app.radius.auto"    // EMS_YES
#define CFG_app_radius_addr		"app.radius.addr"
#define CFG_app_radius_port_auth	"app.radius.port.auth"  // 1812
#define CFG_app_radius_port_acct	"app.radius.port.acct"  // 1813
#define CFG_app_radius_shared_key	"app.radius.shared_key" // admin
#define CFG_app_radius_report_period	"app.radius.rp_period"  // 5s
#define CFG_app_radius_retry_times	"app.radius.retry_times"   // 3
#define CFG_app_radius_retry_timeout	"app.radius.retry_timeout" // 5s
#define CFG_app_radius_disconnect	"app.radius.disconnect"  // 300s

/*
   cfg for qos
 */
#define CFG_app_qos_enable	"app.qos.enable" // yes
#define CFG_app_qos_p2p		"app.qos.p2p"
#define CFH_app_qos_down	"app.qos.down"
#define CFG_app_qos_up		"app.qos.up"


/*
   cfg for app bwlist
 */
#define CFG_app_bwlist_url_def		"app.bwlist.def.url"
#define CFG_app_bwlist_url		"app.bwlist.url"
#define CFG_app_bwlist_mac_white	"app.bwlist.mac.white"
#define CFG_app_bwlist_mac_black	"app.bwlist.mac.black"

#define CFG_lan_ifname			"network.lan.ifname"
#define CFG_lan_addr			"network.lan.ipaddr"
#define CFG_lan_mask			"network.lan.netmask"
#define CFG_lan_gw			"network.lan.gateway"
#define CFG_lan_proto			"network.lan.proto"

#define CFG_wan_ifname			"network.wan.ifname"
#define CFG_wan_addr			"network.wan.ipaddr"
#define CFG_wan_mask			"network.wan.netmask"
#define CFG_wan_gw			"network.wan.gateway"
#define CFG_wan_proto			"network.wan.proto"
#define CFG_wan_dns			"network.wan.dns"

#define CFG_wireless_ifname		"wireless.@wifi-iface[0].ifname"
#define CFG_wireless_ssid		"wireless.@wifi-iface[0].ssid"
#define CFG_wireless_encrypt		"wireless.@wifi-iface[0].encryption"
#define CFG_wireless_key		"wireless.@wifi-iface[0].key"

/*
   cfg for wireless
 */
#define CFG_wireless_enable_encrypt	"wireless.enable_encrypt"


/*
   cfg for client
 */
#define CFG_client_retry_period		"client.retry_period"
#define CFG_client_getconf_period	"client.getconf_period"
#define CFG_client_upt_period		"client.upt_period"
#define CFG_client_subdomain_enable	"client.enable_subdomain"


/*
   for ctrl
 */
#define CFG_ctrl_log_level		"ems.ctrl.log.level"

#endif
