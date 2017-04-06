
#include "ems_core.h"
#include "ems_client.h"
#include "ems_fw.h"
#include "ems_dns.h"
#include "app.h"

static ems_fw  *fw_new()
{
	return (ems_fw *)ems_malloc(sizeof(ems_fw));
}

static ems_void fw_destroy(ems_fw *fw)
{
	if (fw)
		ems_free(fw);
}

dns_url *dns_url_new()
{
	dns_url *url = NULL;

	url = (dns_url *)ems_malloc(sizeof(dns_url));
	if (url) {
		memset(url, 0, sizeof(dns_url));
		ems_hash_fd_init(&url->h_url);
		str_init(&url->url);
		url->flg    = 0;
		url->n_addr = 0;
		url->addr   = NULL;
		url->mask   = 32;
		ems_queue_init(&url->entry);
	}

	return url;
}

ems_void dns_url_free(dns_url *url)
{
	ems_assert(url && "never show up this line");

	if (url) {
		if (url->addr)
			ems_free(url->addr);

		str_uninit(&url->url);
		ems_hash_fd_uninit(&url->h_url);
		ems_free(url);
	}
}

dns_user *dns_user_new()
{
	dns_user *user = NULL;

	user = (dns_user *)ems_malloc(sizeof(dns_user));

	if (user) {
		memset(user, 0, sizeof(dns_user));
		ems_hash_fd_init(&user->h_msg);
		ems_timeout_init(&user->to);
		user->flg  = 0;
		user->port = 0;
		user->ctx  = NULL;
		ems_buffer_init(&user->buf, EMS_BUFFER_1K);
		ems_queue_init(&user->entry);
	}

	return user;
}

ems_void dns_user_free(dns_user *user)
{
	ems_assert(user && "never show up this line");

	if (user) {
		ems_hash_fd_uninit(&user->h_msg);
		ems_buffer_uninit(&user->buf);

		ems_free(user);
	}
}

dns_user *dns_find_user(ems_fw *fw, ems_short key)
{
	ems_hash_fd *h;
	dns_user    *user = NULL;

	h = ems_hash_find(&fw->hash_msg, ems_hash_key(0xffff & key));
	if (h) {
		user = ems_container_of(h, dns_user, h_msg);
	}

	return user;
}

dns_url *dns_find_url(ems_fw *fw, ems_cchar *key)
{
	ems_hash_fd *h;
	dns_url     *url = NULL;

	h = ems_hash_find(&fw->hash_url, key);
	if (h) {
		url = ems_container_of(h, dns_url, h_url);
	}

	return url;
}

static ems_int fw_init(app_module *mod)
{
	ems_fw *fw = NULL;

	fw = fw_new();

	if (fw) {
		memset(fw, 0, sizeof(ems_fw));

		ems_queue_init(&fw->whitelist);
		ems_queue_init(&fw->subdomain);
		ems_queue_init(&fw->fwd);
		ems_queue_init(&fw->wait);
		ems_queue_init(&fw->out);

		fw->n_subdomain  = 0;

		fw->sess_bind = NULL;
		fw->sess_dns  = NULL;

		ems_hash_init(&fw->hash_msg, 128);
		ems_hash_init(&fw->hash_url, 128);

		mod->ctx = (ems_void *)fw;
	}

	return EMS_OK;
}

ems_int fw_whitelist_clear(ems_fw *fw)
{
	ems_hash_clean(&fw->hash_url);
	ems_queue_clear(&fw->whitelist, dns_url, entry, dns_url_free);
	ems_queue_clear(&fw->subdomain, dns_url, entry, dns_url_free);
	fw->n_subdomain = 0;

	return EMS_OK;
}

static ems_int  fw_uninit(app_module *mod)
{
	ems_fw  *fw = (ems_fw *)mod->ctx;

	if (fw) {
		fw_whitelist_clear(fw);

		ems_queue_clear(&fw->fwd,  dns_user, entry, dns_user_free);
		ems_queue_clear(&fw->wait, dns_user, entry, dns_user_free);
		ems_queue_clear(&fw->out,  dns_user, entry, dns_user_free);

		if (fw->sess_bind) {
			ems_session_shutdown_and_destroy(fw->sess_bind);
			fw->sess_bind = NULL;
		}

		if (fw->sess_dns) {
			ems_session_shutdown_and_destroy(fw->sess_dns);
			fw->sess_dns = NULL;
		}

		ems_hash_uninit(&fw->hash_msg);
		ems_hash_uninit(&fw->hash_url);

		fw_destroy(fw);
	}

	mod->ctx = NULL;

	return EMS_OK;
}

ems_int fw_dns_start(ems_fw *fw)
{
	fw_dns_be_server(fw);
	fw_dns_be_client(fw);
	return EMS_OK;
}


ems_int fw_dns_stop(ems_fw *fw)
{
	fw_dns_server_stop(fw);
	fw_dns_client_stop(fw);

	return EMS_OK;
}

static ems_int fw_start(ems_fw *fw)
{
	fw_dns_start(fw);
	/* send self a message */
	ems_send_message(ty_fw, ty_fw, EMS_APP_EVT_FW_RELOAD, NULL);

	return EMS_OK;
}

static ems_int fw_stop(ems_fw *fw)
{
	fw_dns_stop(fw);

	fw_whitelist_clear(fw);
	ems_hash_clean(&fw->hash_msg);
	ems_queue_clear(&fw->fwd,  dns_user, entry, dns_user_free);
	ems_queue_clear(&fw->wait, dns_user, entry, dns_user_free);
	ems_queue_clear(&fw->out,  dns_user, entry, dns_user_free);

	fw_uninit_chains(fw);

	return EMS_OK;
}


static ems_int fw_run(app_module *mod, ems_int run)
{
	ems_fw  *fw = (ems_fw *)mod->ctx;

	ems_assert(fw);

	if (run) {
		if (ems_flag_like(mod->flg, FLG_RUN))
			return EMS_OK;

		ems_flag_set(mod->flg, FLG_RUN);
		ems_l_trace("firewall starting...");

		fw_start(fw);
	} else {
		if (ems_flag_unlike(mod->flg, FLG_RUN))
			return EMS_OK;

		ems_flag_unset(mod->flg, FLG_RUN);
		ems_l_trace("firewall stopping...");

		fw_stop(fw);
	}

	return EMS_OK;
}

static ems_int fw_rules_reload(ems_fw *fw, json_object *root)
{
	ems_int retry = 3;

	do {
		ems_l_trace("do init fw, retry times: %d", retry);
		if (fw_init_chains(fw) == EMS_OK) break;

		fw_uninit_chains(fw);
		ems_sleep(1);
	} while (--retry > 0);

	if (retry <= 0) {
		ems_l_trace("do network restarting..");
		ems_systemcmd("/etc/init.d/network restart");
		exit(1);
	}

	fw_update_all_rules(fw);

	return EMS_OK;
}

static ems_int 
fw_app_rules_update(ems_fw *fw, json_object *root)
{
	fw_update_all_rules(fw);
	return EMS_OK;
}

static ems_int fw_rules_address_add(ems_fw *fw, json_object *root)
{
	ems_assert(json_object_is_type(root, json_type_array));

	if (!json_object_is_type(root, json_type_array))
		return EMS_ERR;

	return fw_append_urls(fw, &fw->whitelist, root);
}

static ems_int fw_rules_address_del(ems_fw *fw, json_object *root)
{
	ems_assert(json_object_is_type(root, json_type_array));

	if (!json_object_is_type(root, json_type_array))
		return EMS_ERR;

	return fw_remove_urls(fw, &fw->whitelist, root);
}

static ems_int fw_rules_radius_device_free(ems_fw *fw, json_object *root)
{
	ems_str ip;
	ems_str mac;
	ems_int add;

	ems_assert(json_object_is_type(root, json_type_object));

	str_init(&ip);
	str_init(&mac);

	ems_json_get_string_def(root, "userip",  &ip, NULL);
	ems_json_get_string_def(root, "usermac", &mac, NULL);
	ems_json_get_int_def(root, "add", add, 0);

	if (str_len(&ip) > 0 && str_len(&mac) > 0) {
		fw_device_free(str_text(&ip), str_text(&mac), add);
	}

	str_uninit(&ip);
	str_uninit(&mac);

	return EMS_OK;
}

static ems_int fw_server_rules_update(ems_fw *fw, json_object *root)
{
	return fw_flush_whiltelist(fw);
}

static ems_int fw_url_is_subdomain(ems_fw *fw, json_object *root)
{
	ems_int rtn;
	ems_str url;

	str_init(&url);

	rtn = EMS_NO;
	ems_json_get_string_def(root, "url", &url, NULL);

	if (str_len(&url) > 0)
		rtn = fw_url_in_whitelist(fw, str_text(&url));

	str_uninit(&url);

	return rtn;
}

static ems_int fw_param_is_apple_com(ems_fw *fw, json_object *root)
{
	ems_int rtn;
	ems_str arg;
	ems_str ip;
	dns_url *url;
	struct in_addr   addr;

	str_init(&ip);
	str_init(&arg);

	ems_json_get_string_def(root, "ip",  &ip,  NULL);
	ems_json_get_string_def(root, "arg", &arg, NULL);

	do {
		rtn = EMS_NO;
		if (! (str_len(&ip) > 0 && str_len(&arg))) break;

		memset(&addr, 0, sizeof(addr));
		addr.s_addr = inet_addr(str_text(&ip));
		if (addr.s_addr == INADDR_NONE) break;

#define APPLE_COM	"apple.com"
		url = dns_find_url(fw, APPLE_COM);
		if (!url) break;

		if (!strstr(str_text(&arg), APPLE_COM)) break;

		url = dns_url_new();
		if (!url) break;

		str_set(&url->url, str_text(&ip));

		url->addr = (struct in_addr *)ems_malloc(sizeof(addr));
		if (url->addr) {
			memcpy(url->addr, &addr, sizeof(addr));
			url->n_addr = 1;
			ems_flag_set(url->flg, FLG_URL_IS_IPADDRESS);
			fw_url_set_free(fw, url, EMS_YES);
			fw_dns_subdomain_append(fw, url);

			rtn = EMS_YES;
		}
	} while (0);


	str_uninit(&arg);
	str_uninit(&ip);

	return rtn;
}

static ems_int 
fw_process(app_module *mod, ems_uint s, ems_uint d, ems_uint evt, json_object *root)
{
	ems_l_trace("[fw] evt: 0x%x, from: 0x%x, args: %s", 
			evt, s, root?json_object_to_json_string(root):"");
	switch(evt) {

	case EMS_APP_START:
		return fw_run(mod, EMS_YES);

	case EMS_APP_STOP:
		return fw_run(mod, EMS_NO);

	case EMS_APP_FW_CLEAR:
		return fw_uninit_chains(NULL);

	default:
		break;
	}

	if (!ems_flag_like(mod->flg, FLG_RUN))
		return EMS_OK;

	switch(evt) {
	case EMS_APP_CHECK_PARAM_APPLE_COM:
		return fw_param_is_apple_com((ems_fw *)mod->ctx, root);

	case EMS_APP_CHECK_SUBDOMAIN:
		return fw_url_is_subdomain((ems_fw *)mod->ctx, root);

	case EMS_APP_RULES_UPDATE:
		return fw_app_rules_update((ems_fw *)mod->ctx, root);

	case EMS_APP_SERVER_RULES_UPDATE:
		return fw_server_rules_update((ems_fw *)mod->ctx, root);

	case EMS_APP_EVT_FW_RELOAD:
		return fw_rules_reload((ems_fw *)mod->ctx, root);

	case EMS_APP_FW_ADDRESS_ADD:
		return fw_rules_address_add((ems_fw *)mod->ctx, root);

	case EMS_APP_FW_ADDRESS_DEL:
		return fw_rules_address_del((ems_fw *)mod->ctx, root);

	case EMS_APP_FW_RADIUS_DEVICE_FREE:
		return fw_rules_radius_device_free((ems_fw *)mod->ctx, root);

	default:
		break;
	}

	return EMS_OK;
}


app_module app_fw = {
	.ty      = ty_fw,
	.desc    = ems_string("firewall"),
	.ctx     = NULL,
	.flg     = 0,

	.init    = fw_init,
	.uninit  = fw_uninit,
	.run     = fw_run,
	.process = fw_process,
	.process_rule  = NULL,
	.version_match = NULL,
	.install       = NULL
};
