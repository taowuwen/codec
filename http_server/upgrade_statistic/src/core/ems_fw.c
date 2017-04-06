
#include "ems_core.h"
#include "ems_client.h"
#include "ems_fw.h"
#include "ems_dns.h"
#include "app.h"


/* for all the chains */
#define C_TABLE_NAT		"nat"
#define LOCUS			"locus_"
#define LOCUS_PRE		LOCUS "pre_"
#define c_nat_fw_pre		LOCUS_PRE "nat"
#define c_mac_black		LOCUS_PRE "mac_black"
#define c_mac_white		LOCUS_PRE "mac_white"
#define c_system_default	LOCUS_PRE "system_def"
#define c_user_white		LOCUS_PRE "user_white"
#define c_url_white		LOCUS_PRE "whitelist"

#define C_TABLE_FILTER		"filter"
#define c_filter_fwd		LOCUS "filter_fwd"

#define RET_ERR_FAILED(A)	if ((A) != 0) return EMS_ERR

static ems_int fw_create_and_insert(ems_char *chain, ems_char *uplink, ems_cchar *tb, ems_int head)
{
	ems_buffer *buf = core_buffer();

	ems_buffer_clear(buf);

	snprintf(buf_wr(buf), buf_left(buf), "iptables -w -t %s -N %s", tb, chain);
	RET_ERR_FAILED(ems_systemcmd(buf_rd(buf)));

	if (head)
		snprintf(buf_wr(buf), buf_left(buf), "iptables -w -t %s -I %s -j %s", tb, uplink, chain);
	else
		snprintf(buf_wr(buf), buf_left(buf), "iptables -w -t %s -A %s -j %s", tb, uplink, chain);

	RET_ERR_FAILED(ems_systemcmd(buf_rd(buf)));

	return EMS_OK;
}

static ems_int fw_apply_defaults()
{
	ems_buffer *buf = core_buffer();

	ems_cchar *gw     = core_gw_addr();
	ems_cchar *ifname = core_gw_ifname();

	if (!gw) {
		ems_assert(0 && "should never be here");
		return EMS_ERR;
	}

	{
		ems_char  *cmd;
		ems_int    len;

		cmd = buf_wr(buf);
		len = buf_left(buf);

		/* for NASGetInfo request */
		snprintf(cmd, len, "iptables -w -i %s -t nat -I %s -p tcp -d %s --dport 80 -j DNAT --to-destination %s:%d",
				ifname, c_nat_fw_pre, NAS_ADDR, gw, EMS_PORT);
		RET_ERR_FAILED(ems_systemcmd(buf_rd(buf)));

		/* for dns*/
		snprintf(cmd, len , "iptables -w -i %s -t nat -A %s -p udp --dport 53 -j  DNAT --to-destination %s:%d",
				ifname, c_system_default, gw, EMS_DNS_BIND);
		RET_ERR_FAILED(ems_systemcmd(buf_rd(buf)));

		/* for dhcp */
		snprintf(cmd, len , "iptables -w -i %s -t nat -A %s -p udp --dport 67 -j ACCEPT", ifname, c_system_default);
		RET_ERR_FAILED(ems_systemcmd(buf_rd(buf)));

		/* dst is gw */
		snprintf(cmd, len , "iptables -w -i %s -t nat -A %s -d %s -j ACCEPT", ifname, c_system_default, gw);
		RET_ERR_FAILED(ems_systemcmd(buf_rd(buf)));

		/* for DEFAULT action */
		snprintf(cmd, len, "iptables -w -i %s -t nat -A %s -p tcp -m multiport --dports 80,8080 -j DNAT --to-destination %s:%d", ifname, c_nat_fw_pre, gw, EMS_PORT);
		RET_ERR_FAILED(ems_systemcmd(buf_rd(buf)));

		/* drop packages  */
		snprintf(cmd, len, "iptables -w -i %s -t nat -A %s  -j DNAT --to-destination 127.0.0.100", ifname, c_nat_fw_pre);
		RET_ERR_FAILED(ems_systemcmd(buf_rd(buf)));
	}

	return EMS_OK;
}


ems_int fw_init_chains(ems_fw *fw)
{
	RET_ERR_FAILED(fw_create_and_insert(c_filter_fwd,    "FORWARD",     C_TABLE_FILTER, EMS_YES));
	RET_ERR_FAILED(fw_create_and_insert(c_nat_fw_pre,    "PREROUTING",  C_TABLE_NAT,    EMS_NO));
	RET_ERR_FAILED(fw_create_and_insert(c_mac_black,      c_nat_fw_pre, C_TABLE_NAT,    EMS_NO));
	RET_ERR_FAILED(fw_create_and_insert(c_mac_white,      c_nat_fw_pre, C_TABLE_NAT,    EMS_NO));
	RET_ERR_FAILED(fw_create_and_insert(c_user_white,     c_nat_fw_pre, C_TABLE_NAT,    EMS_NO));
	RET_ERR_FAILED(fw_create_and_insert(c_system_default, c_nat_fw_pre, C_TABLE_NAT,    EMS_NO));
	RET_ERR_FAILED(fw_create_and_insert(c_url_white,      c_nat_fw_pre, C_TABLE_NAT,    EMS_NO));

	return fw_apply_defaults();
}

static ems_void fw_clean_and_del(ems_cchar *chain, ems_cchar *uplink, ems_cchar *tb)
{
	ems_buffer *buf = core_buffer();

	ems_buffer_clear(buf);

	snprintf(buf_wr(buf), buf_left(buf), "iptables -w -t %s -F %s", tb, chain);
	ems_systemcmd(buf_rd(buf));

	snprintf(buf_wr(buf), buf_left(buf), "iptables -w -t %s -D %s -j %s", tb, uplink, chain);
	ems_systemcmd(buf_rd(buf));

	snprintf(buf_wr(buf), buf_left(buf), "iptables -w -t %s -X %s", tb, chain);
	ems_systemcmd(buf_rd(buf));
}

ems_int fw_uninit_chains(ems_fw *fw)
{
	fw_clean_and_del(c_mac_black,      c_nat_fw_pre, C_TABLE_NAT);
	fw_clean_and_del(c_mac_white,      c_nat_fw_pre, C_TABLE_NAT);
	fw_clean_and_del(c_system_default, c_nat_fw_pre, C_TABLE_NAT);
	fw_clean_and_del(c_user_white,     c_nat_fw_pre, C_TABLE_NAT);
	fw_clean_and_del(c_url_white,      c_nat_fw_pre, C_TABLE_NAT);
	fw_clean_and_del(c_nat_fw_pre,    "PREROUTING",  C_TABLE_NAT);
	fw_clean_and_del(c_filter_fwd,    "FORWARD",     C_TABLE_FILTER);

	return EMS_OK;
}

ems_void fw_clear_all()
{
	fw_uninit_chains(NULL);
}

static ems_int fw_flush_user_mac_white(ems_fw *fw)
{
	json_object *ary, *obj;
	ems_int      i, len;
	ems_char    *cmd;
	ems_buffer  *buf = core_buffer();
	ems_cchar   *ifname = core_gw_ifname();

	ary = cfg_get_json(emscfg(), CFG_user_mac_white);
	if (!ary)
		return EMS_OK;

	ems_l_trace("user defined mac whitelist: %s", json_object_to_json_string(ary));

	cmd = buf_wr(buf);
	len = buf_left(buf);

	snprintf(cmd, len, "iptables -w -t nat -F %s", c_mac_white);
	ems_systemcmd(cmd);

	for (i = 0; i < json_object_array_length(ary); i++) {
		obj = json_object_array_get_idx(ary, i);

		if (obj && json_object_is_type(obj, json_type_string)) 
		{
			snprintf(cmd, len, 
				"iptables -w -i %s -t nat -A %s -m mac --mac-source %s -j ACCEPT",
				ifname, c_mac_white, json_object_get_string(obj));

			ems_systemcmd(cmd);
		}
	}

	json_object_put(ary);

	return EMS_OK;
}

static ems_int fw_flush_user_mac_black(ems_fw *fw)
{
	json_object *ary, *obj;
	ems_buffer   buf;
	ems_int      i, len;
	ems_char    *cmd;
	ems_cchar   *ifname = core_gw_ifname();

	ary = cfg_get_json(emscfg(), CFG_user_mac_black);
	if (!ary)
		return EMS_OK;

	ems_l_trace("user defined mac blacklist: %s", json_object_to_json_string(ary));

	ems_buffer_init(&buf, EMS_BUFFER_1K);

	cmd = buf_wr(&buf);
	len = buf_left(&buf);

	snprintf(cmd, len, "iptables -w -t nat -F %s", c_mac_black);
	ems_systemcmd(cmd);

	for (i = 0; i < json_object_array_length(ary); i++) {
		obj = json_object_array_get_idx(ary, i);

		if (obj && json_object_is_type(obj, json_type_string)) 
		{
			/* drop packages */
			snprintf(cmd, len, 
				"iptables -w -i %s -t nat -A %s -m mac --mac-source %s -j DNAT --to-destination 127.0.0.100",
				ifname, c_mac_black, json_object_get_string(obj));

			ems_systemcmd(cmd);
		}
	}

	ems_buffer_uninit(&buf);
	json_object_put(ary);

	return EMS_OK;
}

static dns_url *fw_find_url(ems_queue *head, ems_cchar *key)
{
	ems_queue *p;
	dns_url   *url;

	ems_queue_foreach(head, p) {
		url = ems_container_of(p, dns_url, entry);

		if (!strcmp(str_text(&url->url), key)) {
			return url;
		}
	}

	return NULL;
}

ems_int fw_remove_urls(ems_fw *fw, ems_queue *head, json_object *ary)
{
	json_object *obj;
	ems_int      i;
	dns_url     *url;

	for (i = 0; i < json_object_array_length(ary); i++) {

		obj = json_object_array_get_idx(ary, i);

		if (obj && json_object_is_type(obj, json_type_string)) 
		{
			ems_cchar *buf = json_object_get_string(obj);

			url = fw_find_url(head, buf);
			if (url) {
				ems_queue_remove(&url->entry);
				ems_hash_remove(&url->h_url);
				fw_url_set_free(fw, url, EMS_NO);
				ems_l_trace("remove url: %s", str_text(&url->url));
				dns_url_free(url);
			}
		}
	}

	return EMS_OK;
}

ems_int fw_append_urls(ems_fw *fw, ems_queue *head, json_object *ary)
{
	json_object *obj;
	ems_int      i;
	dns_url     *url;

	for (i = 0; i < json_object_array_length(ary); i++) {

		obj = json_object_array_get_idx(ary, i);

		if (obj && json_object_is_type(obj, json_type_string)) 
		{
			ems_cchar *buf = json_object_get_string(obj);
			if (fw_find_url(head, buf))
				continue;

			url = dns_url_new();
			if (url) {
				str_set(&url->url, buf);
				/* lower case */

				ems_hash_fd_set_key(&url->h_url, buf);
				ems_hash_insert(&fw->hash_url, &url->h_url);

				ems_l_trace("append url: %s", str_text(&url->url));
				ems_queue_insert_tail(head, &url->entry);
			}
		}
	}

	return EMS_OK;
}

ems_int fw_flush_whiltelist(ems_fw *fw)
{
	json_object *ary, *obj;

	fw_whitelist_clear(fw);

	ems_systemcmd("iptables -w -t nat -F " c_url_white);

	do {
		ary = cfg_get_json(emscfg(), CFG_user_url_white);
		if (ary) {
			ems_l_trace("user bwlist: %s", json_object_to_json_string(ary));
			fw_append_urls(fw, &fw->whitelist, ary);
			json_object_put(ary);
		}

		obj = json_object_new_object();

		ems_app_process(ty_fw, ty_bwlist, EMS_APP_SERVER_BWLIST, obj);
		ary = json_object_object_get(obj, "white");
		if (ary) {
			ems_l_trace("server bwlist: %s", json_object_to_json_string(ary));
			fw_append_urls(fw, &fw->whitelist, ary);
		}

		json_object_put(obj);

		ems_app_process(ty_fw, ty_portal, EMS_APP_EVT_FW_RELOAD, NULL);

	} while (0);

	/* triger dns query maybe */
	fw_dns_query_triger(fw);

	return EMS_OK;
}

/*
   1. clear all bwlists including subdomains
   2. clear all whitelists
   3. append user defined lists
   4. append server defined lists
   5. triger dns query
   6. done
 */
ems_int fw_update_all_rules(ems_fw *fw)
{
	if (fw) {
		fw_flush_user_mac_white(fw);
		fw_flush_user_mac_black(fw);
		fw_flush_whiltelist(fw);
	}

	return EMS_OK;
}

ems_char *fw_mac_update(ems_char *dst, ems_cchar *src)
{
	while (*src) {
		*dst = *src++;

		if (*dst == '-')
			*dst = ':';
		dst++;
	}

	return dst;
}


ems_int fw_device_free(ems_cchar *userip, ems_cchar *usermac, ems_int yes)
{
	ems_char cmd[256] = {0};
	ems_char mac[32]  = {0};

	fw_mac_update(mac, usermac);

	if (yes) {
		snprintf(cmd, sizeof(cmd), 
			"iptables -w -i %s -t nat -A %s -m mac --mac-source %s -s %s -j ACCEPT",
			core_gw_ifname(), c_user_white, mac, userip);
	} else {
		snprintf(cmd, sizeof(cmd), 
			"iptables -w -i %s -t nat -D %s -m mac --mac-source %s -s %s -j ACCEPT",
			core_gw_ifname(), c_user_white, mac, userip);
	}

	ems_systemcmd(cmd);

	/* for accouting user's */
	if (yes) {
		snprintf(cmd, sizeof(cmd),
			"iptables -w -t filter -A %s -j ACCOUNT --addr %s/32 --tname locus_%s",
			c_filter_fwd, userip, usermac);
	} else {
		snprintf(cmd, sizeof(cmd),
			"iptables -w -t filter -D %s -j ACCOUNT --addr %s/32 --tname locus_%s",
			c_filter_fwd, userip, usermac);
	}

	ems_systemcmd(cmd);

	return EMS_OK;
}

ems_int fw_url_set_free(ems_fw *fw, dns_url *url, ems_int append)
{
	ems_int i;
	ems_char cmd[256] = {0};
	ems_cchar *insert = "-I";

	if (!append)
		insert = "-D";

	if (url && url->addr) {
		for (i = 0; i < url->n_addr; i++) 
		{
			if (url->addr[i].s_addr == 0) continue;

			snprintf(cmd, sizeof(cmd), 
				"iptables -w -i %s -t nat %s " c_url_white " -d %s/%d -j ACCEPT -m comment --comment '%s'",
					core_gw_ifname(),
					insert,
					inet_ntoa(url->addr[i]),
					url->mask,
					str_text(&url->url));
			if (ems_systemcmd(cmd) != 0) {
				/* retry to set it next time */
				memset(&url->addr[i], 0, sizeof(struct in_addr));
			}
		}
	}

	return EMS_OK;
}
