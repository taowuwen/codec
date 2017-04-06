
#ifndef EMS_FW_BALIST___HEADER____
#define EMS_FW_BALIST___HEADER____

ems_int fw_init_chains(ems_fw *fw);
ems_int fw_uninit_chains(ems_fw *fw);
ems_int fw_update_all_rules(ems_fw *fw);
ems_int fw_whitelist_clear(ems_fw *fw);
ems_int fw_url_set_free(ems_fw *fw, dns_url *url, ems_int append);
ems_int fw_append_urls(ems_fw *fw, ems_queue *head, json_object *ary);
ems_int fw_remove_urls(ems_fw *fw, ems_queue *head, json_object *ary);
ems_int fw_device_free(ems_cchar *ip, ems_cchar *mac, ems_int yes);
ems_int fw_flush_whiltelist(ems_fw *fw);
ems_void fw_clear_all();

struct _ems_fw_s {
	struct /* bwlist */ {
		ems_queue  whitelist; // for white list for ems servers's url
		ems_int    n_subdomain; // for both user and server defined
		ems_queue  subdomain; 
	};

	ems_session  *sess_bind;
	ems_session  *sess_dns;

	ems_hash      hash_msg;
	ems_hash      hash_url;

	ems_queue     fwd;  /* forward to local dns server */
	ems_queue     wait; /* dns request waiting for  response */
	ems_queue     out;  /* for sendout*/
};


#define MAX_SUB_DOMAIN		300

#endif
