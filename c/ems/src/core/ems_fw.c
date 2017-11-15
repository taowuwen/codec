
#include "ems_core.h"
#include "ems_client.h"
#include "app.h"
#include "app_nic.h"
#include "ems_fw.h"
#include "ems_dns.h"
#include <stdarg.h>
#include "ems_split.h"


#define FW_T_MANGLE_PREROUTING		FW_PREFIX "prerouting"
#define FW_T_NAT_PREROUTING		FW_PREFIX "prerouting"
#define FW_T_NAT_PORTAL_PRE		FW_PREFIX "pre"
#define FW_T_NAT_PORTAL_GLOBAL		FW_PREFIX "global"
#define FW_T_NAT_PORTAL_POST		FW_PREFIX "post"

#define FW_T_NAT_PORTAL_GLOBAL_BLACKMAC	FW_T_NAT_PORTAL_GLOBAL "blackmac"
#define FW_T_NAT_PORTAL_GLOBAL_WHITEMAC	FW_T_NAT_PORTAL_GLOBAL "whitemac"

#define FW_T_MANGLE_PHYDEV		FW_PREFIX "phydev"
#define FW_T_MANGLE_PHYDEV_MARK		FW_PREFIX "phydev_mark"

#define FW_T_NAT_PHYDEV_PRE		FW_PREFIX "phydev_pre"
#define FW_T_NAT_PHYDEV_POST		FW_PREFIX "phydev_post"

/* for pre handle */
#define FW_T_NAT_PRE_PRE		FW_PREFIX "pre_pre"
#define FW_T_NAT_PRE_MAC_BLACK		FW_PREFIX "pre_blackmac"
#define FW_T_NAT_PRE_MAC_WHITE		FW_PREFIX "pre_whitemac"
#define FW_T_NAT_PRE_DEFAULT		FW_PREFIX "pre_default"

/*
   for post handle 
 */
#define FW_T_NAT_POST_BLACKLIST		FW_PREFIX "post_blacklist"
#define FW_T_NAT_POST_AUTHED		FW_PREFIX "post_authed"
#define FW_T_NAT_POST_WHITELIST		FW_PREFIX "post_whitelist"
#define FW_T_NAT_POST_DEFAULT		FW_PREFIX "post_default"
#define FW_T_NAT_POST_POST		FW_PREFIX "post_post"



/* for all the chains */
#define C_TABLE_FILTER		"filter"
#define C_TABLE_NAT		"nat"
#define C_TABLE_MANGLE		"mangle"
#define C_FILTER_ACCOUNT	FW_PREFIX "account"

#define RET_ERR_FAILED(A)	if ((A) != 0) { ems_l_warn("[fw]err: %s", ems_lasterrmsg()); return EMS_ERR; }

static ems_cchar *fw_chain(ems_fw *fw, ems_cchar *prefix, ems_char *chain, ems_int l_chain)
{
	ems_assert(fw && prefix && chain && l_chain > 0);
	snprintf(chain, l_chain, "%s_%d", prefix, fw->mark);
	return chain;
}

static ems_int fw_create_chain(ems_cchar *tb, ems_cchar *chain)
{
	ems_int ret;

	ret = fw_ipt_exec("iptables -w -t %s -N %s", tb, chain);

	if (ret != EMS_OK) {
		ems_cchar *res = NULL;

		res = ems_popen_get("iptables -S -t %s | awk '/%s/ {print $0; exit}'", tb, chain); 

		if (ems_strlen(res) <= 0) {
			ems_l_trace("[fw] create chain (%s, %s) failed", tb, chain);
			return EMS_ERR;
		}
	}

	return EMS_OK;
}

static ems_int fw_create_and_insert(ems_char *chain, ems_char *uplink, ems_cchar *tb, ems_int head)
{
	RET_ERR_FAILED(fw_create_chain(tb, chain));

	if (head) {
		fw_ipt_exec("iptables -w -t %s -I %s -j %s", tb, uplink, chain);
	}
	else {
		fw_ipt_exec("iptables -w -t %s -A %s -j %s", tb, uplink, chain);
	}

	return EMS_OK;
}

static ems_int fw_apply_defaults(ems_fw *fw)
{
	ems_char chain[512];

	fw_chain(fw, FW_T_NAT_POST_POST, chain, 512);
	fw_ipt_exec("iptables -w -i %s -t nat -A %s  -j DNAT --to-destination 127.0.0.100", core_gw_ifname(), chain);

	return EMS_OK;
}

static ems_int fw_mangle_create_and_insert(ems_fw *fw)
{
	ems_char chain[512];

	fw_chain(fw, FW_T_MANGLE_PHYDEV, chain, 512);

	RET_ERR_FAILED(fw_create_chain(C_TABLE_MANGLE, chain));

	fw_ipt_exec("iptables -w -t mangle -A %s -m physdev --physdev-in %s -j %s",
			FW_T_MANGLE_PREROUTING, str_text(&fw->ssid->_iface->ifname), chain);

	fw_ipt_exec("iptables -w -t mangle -A %s -j MARK --set-mark %d", chain, fw->mark);

	return EMS_OK;
}

static ems_int 
fw_nat_physdev_create_and_insert(ems_fw *fw, ems_cchar *chain, ems_cchar *uplink, ems_int insert)
{
	ems_char c_chain[512];

	fw_chain(fw, chain,  c_chain,  512);

	RET_ERR_FAILED(fw_create_chain(C_TABLE_NAT, c_chain));

	if (insert) {
		fw_ipt_exec("iptables -w -i %s -t nat -I %s -m mark --mark %d -j %s",
				core_gw_ifname(), uplink, fw->mark, c_chain);
	} else {
		fw_ipt_exec("iptables -w -i %s -t nat -A %s -m mark --mark %d -j %s",
				core_gw_ifname(), uplink, fw->mark, c_chain);
	}

	return EMS_OK;
}

static ems_int
fw_nat_create_and_insert(ems_fw *fw, ems_cchar *chain, ems_cchar *uplink, ems_int insert)
{
	ems_char c_chain[512];
	ems_char c_uplink[512];

	fw_chain(fw, chain,  c_chain,  512);
	fw_chain(fw, uplink, c_uplink, 512);

	RET_ERR_FAILED(fw_create_chain(C_TABLE_NAT, c_chain));

	if (insert) {
		fw_ipt_exec("iptables -w -i %s -t nat -I %s -j %s", core_gw_ifname(), c_uplink, c_chain);
	} else {
		fw_ipt_exec("iptables -w -i %s -t nat -A %s -j %s", core_gw_ifname(), c_uplink, c_chain);
	}

	return EMS_OK;
}

ems_int fw_init_chains(ems_fw *fw)
{
	RET_ERR_FAILED(fw_mangle_create_and_insert(fw));

	RET_ERR_FAILED(fw_nat_physdev_create_and_insert(fw, FW_T_NAT_PHYDEV_PRE, FW_T_NAT_PORTAL_PRE, EMS_NO));
	RET_ERR_FAILED(fw_nat_create_and_insert(fw, FW_T_NAT_PRE_PRE,       FW_T_NAT_PHYDEV_PRE, EMS_NO));
	RET_ERR_FAILED(fw_nat_create_and_insert(fw, FW_T_NAT_PRE_MAC_BLACK, FW_T_NAT_PHYDEV_PRE, EMS_NO));
	RET_ERR_FAILED(fw_nat_create_and_insert(fw, FW_T_NAT_PRE_MAC_WHITE, FW_T_NAT_PHYDEV_PRE, EMS_NO));
	RET_ERR_FAILED(fw_nat_create_and_insert(fw, FW_T_NAT_PRE_DEFAULT,   FW_T_NAT_PHYDEV_PRE, EMS_NO));

	RET_ERR_FAILED(fw_nat_physdev_create_and_insert(fw, FW_T_NAT_PHYDEV_POST, FW_T_NAT_PORTAL_POST, EMS_NO));
	RET_ERR_FAILED(fw_nat_create_and_insert(fw, FW_T_NAT_POST_BLACKLIST, FW_T_NAT_PHYDEV_POST, EMS_NO));
	RET_ERR_FAILED(fw_nat_create_and_insert(fw, FW_T_NAT_POST_AUTHED,    FW_T_NAT_PHYDEV_POST, EMS_NO));
	RET_ERR_FAILED(fw_nat_create_and_insert(fw, FW_T_NAT_POST_WHITELIST, FW_T_NAT_PHYDEV_POST, EMS_NO));
	RET_ERR_FAILED(fw_nat_create_and_insert(fw, FW_T_NAT_POST_DEFAULT,   FW_T_NAT_PHYDEV_POST, EMS_NO));
	RET_ERR_FAILED(fw_nat_create_and_insert(fw, FW_T_NAT_POST_POST,      FW_T_NAT_PHYDEV_POST, EMS_NO));

	return fw_apply_defaults(fw);
}

static ems_void fw_clean_and_del(ems_fw *fw, ems_cchar *chain, ems_cchar *uplink, ems_cchar *tb)
{
	ems_char c_chain[512];
	ems_char c_uplink[512];

	fw_chain(fw, chain,  c_chain,  512);
	fw_chain(fw, uplink, c_uplink, 512);

	fw_ipt_exec("iptables -w -t %s -F %s", tb, c_chain);
	fw_ipt_exec("iptables -w -t %s -D %s -j %s", tb, c_uplink, c_chain);
	fw_ipt_exec("iptables -w -t %s -X %s", tb, c_chain);
}

static ems_void fw_mangle_clean(ems_fw *fw)
{
	ems_char c_chain[512];
	fw_chain(fw, FW_T_MANGLE_PHYDEV, c_chain, 512);
	fw_ipt_exec("iptables -w -t mangle -F %s", c_chain);
	fw_ipt_exec("iptables -w -t mangle -D %s -m physdev --physdev-in %s -j %s",
			FW_T_MANGLE_PREROUTING, str_text(&fw->ssid->_iface->ifname), c_chain);
	fw_ipt_exec("iptables -w -t mangle -X %s", c_chain);
}

static ems_void fw_nat_clean_physdev(ems_fw *fw, ems_cchar *chain, ems_cchar *uplink)
{
	ems_char c_chain[512];

	fw_chain(fw, chain,  c_chain,  512);

	fw_ipt_exec("iptables -w -t nat -F %s", c_chain);
	fw_ipt_exec("iptables -w -i %s -t nat -D %s -m mark --mark %d -j %s", core_gw_ifname(), uplink, fw->mark, c_chain);
	fw_ipt_exec("iptables -w -t nat -X %s", c_chain);
}

ems_int fw_uninit_chains(ems_fw *fw)
{
	fw_mangle_clean(fw);

	fw_nat_clean_physdev(fw, FW_T_NAT_PHYDEV_PRE, FW_T_NAT_PORTAL_PRE);
	fw_nat_clean_physdev(fw, FW_T_NAT_PHYDEV_POST, FW_T_NAT_PORTAL_POST);

	fw_clean_and_del(fw, FW_T_NAT_PRE_PRE,       FW_T_NAT_PHYDEV_PRE, C_TABLE_NAT);
	fw_clean_and_del(fw, FW_T_NAT_PRE_MAC_BLACK, FW_T_NAT_PHYDEV_PRE, C_TABLE_NAT);
	fw_clean_and_del(fw, FW_T_NAT_PRE_MAC_WHITE, FW_T_NAT_PHYDEV_PRE, C_TABLE_NAT);
	fw_clean_and_del(fw, FW_T_NAT_PRE_DEFAULT,   FW_T_NAT_PHYDEV_PRE, C_TABLE_NAT);

	fw_clean_and_del(fw, FW_T_NAT_POST_BLACKLIST, FW_T_NAT_PHYDEV_POST, C_TABLE_NAT);
	fw_clean_and_del(fw, FW_T_NAT_POST_AUTHED,    FW_T_NAT_PHYDEV_POST, C_TABLE_NAT);
	fw_clean_and_del(fw, FW_T_NAT_POST_WHITELIST, FW_T_NAT_PHYDEV_POST, C_TABLE_NAT);
	fw_clean_and_del(fw, FW_T_NAT_POST_DEFAULT,   FW_T_NAT_PHYDEV_POST, C_TABLE_NAT);
	fw_clean_and_del(fw, FW_T_NAT_POST_POST,      FW_T_NAT_PHYDEV_POST, C_TABLE_NAT);

	fw_nat_clean_physdev(fw, FW_T_NAT_PHYDEV_PRE, FW_T_NAT_PORTAL_PRE);
	fw_nat_clean_physdev(fw, FW_T_NAT_PHYDEV_POST, FW_T_NAT_PORTAL_POST);

	return EMS_OK;
}

ems_void fw_clear_all()
{
	fw_uninit_chains(NULL);
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
				ems_l_trace("[fw] remove url: %s", str_text(&url->url));
				dns_url_free(url);
			}
		}
	}

	return EMS_OK;
}

ems_int fw_append_urls(ems_fw *fw, ems_queue *head, json_object *ary, ems_int isblack)
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
				if (isblack)
					ems_flag_set(url->flg, FLG_DNS_IS_BLACKLIST);

				ems_hash_fd_set_key(&url->h_url, buf);
				ems_hash_insert(&fw->hash_url, &url->h_url);

				ems_l_trace("[fw] append url: %s", str_text(&url->url));
				ems_queue_insert_tail(head, &url->entry);
			}
		}
	}

	return EMS_OK;
}

static ems_int fw_user_whitemac_cb(ems_void *arg, ems_cchar *mac)
{
	ems_cchar *chain = (ems_cchar *)arg;

	ems_assert(arg && mac);

	if (!(arg && mac))
		return EMS_ERR;

	fw_ipt_exec("iptables -w -i %s -t nat -A %s -m mac --mac-source %s -j ACCEPT",
		core_gw_ifname(), chain, mac);

	return EMS_OK;
}

static ems_int fw_flush_user_mac_white(ems_fw *fw)
{
	ems_char chain[512];

	ems_wifi_iface *iface;
	iface = fw->ssid->_iface;

	fw_chain(fw, FW_T_NAT_PRE_MAC_WHITE, chain, 512);

	fw_ipt_exec("iptables -w -t nat -F %s", chain);
	ems_split_foreach(&iface->auth.bwlist.whitemac, fw_user_whitemac_cb, (ems_void *)chain);

	return EMS_OK;
}

static ems_int fw_user_blackmac_cb(ems_void *arg, ems_cchar *mac)
{
	ems_cchar *chain = (ems_cchar *)arg;

	ems_assert(arg && mac);

	if (!(arg && mac))
		return EMS_ERR;

	fw_ipt_exec(
		"iptables -w -i %s -t nat -A %s -m mac --mac-source %s -j DNAT --to-destination 127.0.0.100",
		core_gw_ifname(), chain, mac);

	return EMS_OK;
}


static ems_int fw_flush_user_mac_black(ems_fw *fw)
{
	ems_char chain[512];

	ems_wifi_iface *iface;
	iface = fw->ssid->_iface;

	fw_chain(fw, FW_T_NAT_PRE_MAC_BLACK, chain, 512);

	fw_ipt_exec("iptables -w -t nat -F %s", chain);
	ems_split_foreach(&iface->auth.bwlist.blackmac, fw_user_blackmac_cb, (ems_void *)chain);

	return EMS_OK;
}

static ems_int fw_user_whitelist_cb(ems_void *arg, ems_cchar *url)
{
	json_object *ary = (json_object *)arg;

	ems_assert(arg && url);

	if ( ary && url)
		json_object_array_add(ary, json_object_new_string(url));

	return EMS_OK;
}

static json_object *fw_user_whitelist(ems_fw *fw)
{
	json_object *ary;

	ary = json_object_new_array();

	ems_split_foreach(&fw->ssid->_iface->auth.bwlist.whitelist, fw_user_whitelist_cb, (ems_void *)ary);

	return ary;
}

ems_int fw_flush_whiltelist(ems_fw *fw)
{
	ems_char chain[512];
	json_object *ary, *obj;

	fw_whitelist_clear(fw);

	fw_ipt_exec("iptables -w -t nat -F %s", fw_chain(fw, FW_T_NAT_POST_WHITELIST, chain, 512));
	fw_ipt_exec("iptables -w -t nat -F %s", fw_chain(fw, FW_T_NAT_POST_BLACKLIST, chain, 512));

	do {
		/* user bwlist */
		ary = fw_user_whitelist(fw);
		if (ary) {
			ems_l_trace("[fw]user bwlist: %s", json_object_to_json_string(ary));
			fw_append_urls(fw, &fw->whitelist, ary, EMS_NO);
			json_object_put(ary);
		}

		obj = json_object_new_object();

		nic_processmsg(fw->ssid, ty_fw, ty_bwlist, EMS_APP_SERVER_BWLIST, obj);
		ary = json_object_object_get(obj, "white");
		if (ary) {
			ems_l_trace("[fw]server whitelist: %s", json_object_to_json_string(ary));
			fw_append_urls(fw, &fw->whitelist, ary, EMS_NO);
		}

		ary = json_object_object_get(obj, "black");
		if (ary) {
			ems_l_trace("[fw]server blacklist: %s", json_object_to_json_string(ary));
			fw_append_urls(fw, &fw->whitelist, ary, EMS_YES);
		}

		json_object_put(obj);
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
	ems_char chain[512];
	if (fw) {
		fw_flush_user_mac_white(fw);
		fw_flush_user_mac_black(fw);
		fw_flush_whiltelist(fw);

		nic_sendmsg(fw->ssid, ty_fw, ty_portal, EMS_APP_EVT_FW_RELOAD, NULL);

		fw_ipt_exec("iptables -t nat -F %s", fw_chain(fw, FW_T_NAT_POST_AUTHED, chain, 512)); /* for radius */
		fw_ipt_exec("iptables -t nat -F %s", fw_chain(fw, FW_T_NAT_PRE_DEFAULT, chain, 512)); /* for http nasget info */
		fw_ipt_exec("iptables -t nat -F %s", fw_chain(fw, FW_T_NAT_POST_DEFAULT,chain, 512)); /* for http redirect */
		fw_ipt_exec("iptables -t nat -F %s", fw_chain(fw, FW_T_NAT_PRE_PRE,     chain, 512)); /* for dns*/

		nic_sendmsg(fw->ssid, ty_fw, ty_radius, EMS_APP_EVT_FW_RELOAD, NULL);
		nic_sendmsg(fw->ssid, ty_fw, ty_http,   EMS_APP_EVT_FW_RELOAD, NULL);

		if (fw->sess_bind)
			fw_dns_set_free(fw, ems_sock_port(&fw->sess_bind->sock), EMS_YES);
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


ems_int fw_device_free(ems_fw *fw, ems_cchar *userip, ems_cchar *usermac, ems_int yes)
{
	ems_char mac[32]  = {0};
	ems_char chain[512];

	fw_mac_update(mac, usermac);
	fw_chain(fw, FW_T_NAT_POST_AUTHED, chain, 512);

	if (yes) {
		fw_ipt_exec("iptables -w -i %s -t nat -A %s -m mac --mac-source %s -s %s -j ACCEPT",
			core_gw_ifname(), chain, mac, userip);
	} else {
		fw_ipt_exec("iptables -w -i %s -t nat -D %s -m mac --mac-source %s -s %s -j ACCEPT",
			core_gw_ifname(), chain, mac, userip);
	}

	/* for accouting user's */
	if (yes) {
		fw_ipt_exec("iptables -w -t filter -A %s -j ACCOUNT --addr %s/32 --tname " FW_PREFIX "%s",
			C_FILTER_ACCOUNT, userip, usermac);
	} else {
		fw_ipt_exec("iptables -w -t filter -D %s -j ACCOUNT --addr %s/32 --tname " FW_PREFIX "%s",
			C_FILTER_ACCOUNT, userip, usermac);
	}

	return EMS_OK;
}

ems_int fw_url_set_free(ems_fw *fw, dns_url *url, ems_int append)
{
	ems_int i, ret;
	ems_char chain[512] = {0};
	ems_cchar *opt = "-I";

	if (!append)
		opt = "-D";

	if (url && url->addr) {
		for (i = 0; i < url->n_addr; i++) 
		{
			if (url->addr[i].s_addr == 0) continue;

			if (ems_flag_like(url->flg, FLG_DNS_IS_BLACKLIST)) {
				ret = fw_ipt_exec(
					"iptables -w -i %s -t nat %s %s -d %s/%d -j DNAT --to-destination 127.0.0.100 -m comment --comment '%s'",
					core_gw_ifname(),
					opt,
					fw_chain(fw, FW_T_NAT_POST_BLACKLIST, chain, 512),
					inet_ntoa(url->addr[i]),
					url->mask,
					str_text(&url->url));
			} else {
				ret = fw_ipt_exec(
					"iptables -w -i %s -t nat %s %s -d %s/%d -j ACCEPT -m comment --comment '%s'",
					core_gw_ifname(),
					opt,
					fw_chain(fw, FW_T_NAT_POST_WHITELIST, chain, 512),
					inet_ntoa(url->addr[i]),
					url->mask,
					str_text(&url->url));
			}

			if (ret != EMS_OK)
				memset(&url->addr[i], 0, sizeof(struct in_addr));
		}
	}

	return EMS_OK;
}

ems_int fw_nas_set_free(ems_fw *fw, ems_cchar *dst, ems_int srcport, ems_int dstport, ems_int append)
{
	ems_char chain[512] = {0};
	ems_cchar *opt = "-I";

	if (!append)
		opt = "-D";

	fw_chain(fw, FW_T_NAT_PRE_DEFAULT, chain, 512);
	fw_ipt_exec("iptables -w -i %s -t nat %s %s -p tcp -d %s --dport %d -j DNAT --to-destination %s:%d",
			core_gw_ifname(), opt, chain, dst,  srcport, core_gw_addr(), dstport);

	return EMS_OK;
}


ems_int fw_dns_set_free(ems_fw *fw, ems_int dstport, ems_int append)
{
	ems_char chain[512] = {0};
	ems_cchar *opt = "-I";

	if (!append)
		opt = "-D";

	fw_chain(fw, FW_T_NAT_PRE_PRE, chain, 512);
	fw_ipt_exec("iptables -w -i %s -t nat %s %s -p udp --dport 53 -j DNAT --to-destination %s:%d",
			core_gw_ifname(), opt, chain, core_gw_addr(), dstport);

	return EMS_OK;
}

ems_int fw_http_set_free(ems_fw *fw, ems_int srcport, ems_int dstport, ems_int append)
{
	ems_char chain[512] = {0};
	ems_cchar *opt = "-I";

	if (!append)
		opt = "-D";

	fw_chain(fw, FW_T_NAT_POST_DEFAULT, chain, 512);
	fw_ipt_exec("iptables -w -i %s -t nat %s %s -p tcp --dport %d -j DNAT --to-destination %s:%d",
			core_gw_ifname(), opt, chain, srcport, core_gw_addr(), dstport);

	return EMS_OK;
}

/*
   for firewall global 
 */
static ems_int g_fw_flush_user_mac_white()
{
	json_object *ary, *obj;
	ems_int      i;

	ary = cfg_get_json(emscfg(), CFG_user_mac_white);
	if (!ary)
		return EMS_OK;

	ems_l_trace("[g_fw] user defined mac WHITE list: %s", json_object_to_json_string(ary));


	fw_ipt_exec("iptables -w -t nat -F %s", FW_T_NAT_PORTAL_GLOBAL_WHITEMAC);

	for (i = 0; i < json_object_array_length(ary); i++) {
		obj = json_object_array_get_idx(ary, i);

		if (obj && json_object_is_type(obj, json_type_string)) 
		{
			fw_ipt_exec("iptables -w -i %s -t nat -A %s -m mac --mac-source %s -j ACCEPT",
				core_gw_ifname(), FW_T_NAT_PORTAL_GLOBAL_WHITEMAC, json_object_get_string(obj));
		}
	}

	json_object_put(ary);

	return EMS_OK;
}

static ems_int g_fw_flush_user_mac_black()
{
	json_object *ary, *obj;
	ems_int      i;

	ary = cfg_get_json(emscfg(), CFG_user_mac_black);
	if (!ary)
		return EMS_OK;

	ems_l_trace("[g_fw] user defined mac BLACK list: %s", json_object_to_json_string(ary));

	fw_ipt_exec("iptables -w -t nat -F %s", FW_T_NAT_PORTAL_GLOBAL_BLACKMAC);

	for (i = 0; i < json_object_array_length(ary); i++) {
		obj = json_object_array_get_idx(ary, i);

		if (obj && json_object_is_type(obj, json_type_string)) 
		{
			/* drop packages */
			fw_ipt_exec("iptables -w -i %s -t nat -A %s -m mac --mac-source %s -j DNAT --to-destination 127.0.0.100",
				core_gw_ifname(), FW_T_NAT_PORTAL_GLOBAL_BLACKMAC, json_object_get_string(obj));
		}
	}

	json_object_put(ary);

	return EMS_OK;
}

static ems_int g_fw_do_init_rules()
{
	/* for dhcp */
	fw_ipt_exec("iptables -w -i %s -t nat -A %s -p udp --dport 67 -j ACCEPT", core_gw_ifname(), FW_T_NAT_PORTAL_GLOBAL);
	fw_ipt_exec("iptables -w -i %s -t nat -A %s -d %s -j ACCEPT", core_gw_ifname(), FW_T_NAT_PORTAL_GLOBAL, core_gw_addr());

	g_fw_flush_user_mac_white();
	g_fw_flush_user_mac_black();

	return EMS_OK;
}

ems_int g_fw_start()
{
	RET_ERR_FAILED(fw_create_and_insert(C_FILTER_ACCOUNT, "FORWARD", C_TABLE_FILTER, EMS_YES));
	RET_ERR_FAILED(fw_create_and_insert(FW_T_MANGLE_PREROUTING, "PREROUTING", C_TABLE_MANGLE, EMS_NO));

	RET_ERR_FAILED(fw_create_and_insert(FW_T_NAT_PREROUTING,   "PREROUTING",         C_TABLE_NAT, EMS_NO));
	RET_ERR_FAILED(fw_create_and_insert(FW_T_NAT_PORTAL_PRE,    FW_T_NAT_PREROUTING, C_TABLE_NAT, EMS_NO));
	RET_ERR_FAILED(fw_create_and_insert(FW_T_NAT_PORTAL_GLOBAL, FW_T_NAT_PREROUTING, C_TABLE_NAT, EMS_NO));
	RET_ERR_FAILED(fw_create_and_insert(FW_T_NAT_PORTAL_POST,   FW_T_NAT_PREROUTING, C_TABLE_NAT, EMS_NO));

	RET_ERR_FAILED(fw_create_and_insert(FW_T_NAT_PORTAL_GLOBAL_BLACKMAC,   FW_T_NAT_PORTAL_GLOBAL, C_TABLE_NAT, EMS_YES));
	RET_ERR_FAILED(fw_create_and_insert(FW_T_NAT_PORTAL_GLOBAL_WHITEMAC,   FW_T_NAT_PORTAL_GLOBAL, C_TABLE_NAT, EMS_NO));

	return g_fw_do_init_rules();
}

static ems_void g_fw_clean_and_del(ems_cchar *chain, ems_cchar *uplink, ems_cchar *tb)
{
	fw_ipt_exec("iptables -w -t %s -F %s", tb, chain);
	fw_ipt_exec("iptables -w -t %s -D %s -j %s", tb, uplink, chain);
	fw_ipt_exec("iptables -w -t %s -X %s", tb, chain);
}

ems_int g_fw_stop()
{
	g_fw_clean_and_del(C_FILTER_ACCOUNT, "FORWARD", C_TABLE_FILTER);
	g_fw_clean_and_del(FW_T_MANGLE_PREROUTING, "PREROUTING", C_TABLE_MANGLE);

	g_fw_clean_and_del(FW_T_NAT_PORTAL_GLOBAL_BLACKMAC, FW_T_NAT_PORTAL_GLOBAL, C_TABLE_NAT);
	g_fw_clean_and_del(FW_T_NAT_PORTAL_GLOBAL_WHITEMAC, FW_T_NAT_PORTAL_GLOBAL, C_TABLE_NAT);

	g_fw_clean_and_del(FW_T_NAT_PORTAL_PRE,     FW_T_NAT_PREROUTING, C_TABLE_NAT);
	g_fw_clean_and_del(FW_T_NAT_PORTAL_GLOBAL,  FW_T_NAT_PREROUTING, C_TABLE_NAT);
	g_fw_clean_and_del(FW_T_NAT_PORTAL_POST,    FW_T_NAT_PREROUTING, C_TABLE_NAT);
	g_fw_clean_and_del(FW_T_NAT_PREROUTING,    "PREROUTING",         C_TABLE_NAT);

	return EMS_OK;
}

ems_int g_fw_flush_bwlist()
{
	g_fw_flush_user_mac_white();
	g_fw_flush_user_mac_black();

	return EMS_OK;
}


ems_int fw_wired_nic_attach(ems_int id, ems_int attach)
{
	ems_char chain[512];

	snprintf(chain, sizeof(chain), FW_T_MANGLE_PHYDEV "_lan");

	ems_l_trace("[fw] wired nic attached(%s) %d", attach?"yes":"no", id);

	if (attach) {
		ems_char c_chain[512];
		snprintf(c_chain, sizeof(c_chain), FW_T_MANGLE_PHYDEV "_%d", id);

		RET_ERR_FAILED(fw_create_chain(C_TABLE_MANGLE, chain));
		fw_ipt_exec("iptables -w -t mangle -A %s -m physdev --physdev-in eth0.1 -j %s", FW_T_MANGLE_PREROUTING, chain);
		fw_ipt_exec("iptables -w -t mangle -A %s -j %s", chain, c_chain);
	} else {
		fw_ipt_exec("iptables -w -t mangle -F %s", chain);
		fw_ipt_exec("iptables -w -t mangle -D %s -m physdev --physdev-in eth0.1 -j %s", FW_T_MANGLE_PREROUTING, chain);
		fw_ipt_exec("iptables -w -t mangle -X %s", chain);
	}

	return EMS_OK;
}
