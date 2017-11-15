
#ifndef EMS_NIC_HEADER____
#define EMS_NIC_HEADER____


typedef struct _ems_nic_wired_s  ems_nic_wired;
typedef struct _ems_wifi_iface_s ems_wifi_iface;

#define ems_wifi_section	ems_wifi_iface

typedef struct _ems_nic_wireless_s	ems_nic_wireless;
typedef struct _ems_nic_s		ems_nic;


struct _ems_nic_wired_s {
	ems_int lan_free;
	ems_int iface_id;
};


/*
   get current loaded interface:
	ip link show  | awk '/^[0-9]/{gsub(":", "", $2); if (match($2, "^ra[0-9]$")) print $2 }'
 */

struct _ems_wifi_iface_s {

	struct /* wireless info goes from here*/ {
		ems_int id;
		ems_int disabled;
		ems_int up;

		ems_str ssid;
		ems_str bssid;
		ems_str ifname;
		ems_str nick;
	};

	struct /* for auth */ {
		ems_int enable;
		ems_int offline_disconnect; /* for reauth*/

		struct {
			ems_str addr;
			ems_int port;
			ems_int redirect_port;
			ems_int register_period;
			ems_int heartbeat_period;
		} ptl;

		struct {
			ems_str addr;
			ems_int auth_port;
			ems_int acct_port;
			ems_int acct_period;
			ems_str secret; /* for secret key */
		} radius;

		struct {
			ems_queue whitelist; /* for white list*/
			ems_queue whitemac;  /* for white mac*/
			ems_queue blackmac;  /* for black mac*/
		} bwlist;
	} auth;
};

struct _ems_nic_wireless_s {
	ems_queue       entry;
	ems_queue       msg_entry; /* for wifi_iface inner msg queue */

	ems_nic        *_nic;
	ems_wifi_iface *_iface;
	app_module    **_modl;
};

struct _ems_nic_s {
	ems_str    nick;

	ems_queue  list; /* wireless queue */
	/* for wired config */

	ems_nic_wired  wired;

//	ems_int    num_ssid;
	json_object *jcfg;
	json_object *jhttp;
	json_object *jserv;
};

ems_int nic_sendmsg(ems_nic_wireless *ssid, 
		ems_uint s, ems_uint d, ems_uint evt, json_object *obj);

ems_int nic_processmsg(ems_nic_wireless *ssid, 
		ems_uint s, ems_uint d, ems_uint evt, json_object *obj);


ems_int nic_app_run(ems_nic_wireless *ssid, ems_app_type app);

#endif
