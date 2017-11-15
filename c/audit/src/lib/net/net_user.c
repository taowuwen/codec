
#include "audit.h"
#include "ems_core.h"
#include "ems_client.h"
#include "net_user.h"

typedef struct _net_user_s net_user;

struct _net_user_s {
	ems_str	    mac;
	ems_str	    nick;
	ems_hash_fd hmac;
};

static net_user *net_user_new()
{
	net_user *user = NULL;
	user = (net_user *)ems_malloc(sizeof(net_user));

	if (user) {
		memset(user, 0, sizeof(net_user));
		str_init(&user->mac);
		str_init(&user->nick);
		ems_hash_fd_init(&user->hmac);
	}

	return user;
}

static ems_void net_user_destroy(net_user *user)
{
	ems_assert(user != NULL);
	if (user) {
		str_uninit(&user->mac);
		str_uninit(&user->nick);
		ems_hash_fd_uninit(&user->hmac);
		ems_free(user);
	}
}

static ems_int net_user_hash_cb(ems_hash_fd *fd, ems_void *arg)
{
	net_user *user = ems_container_of(fd, net_user, hmac);

	ems_hash_remove(fd);
	net_user_destroy(user);

	return EMS_OK;
}

static net_user *net_user_find(ems_hash *h, ems_cchar *mac)
{
	ems_hash_fd *fd = NULL;

	ems_assert(h && mac);

	fd = ems_hash_find(h, mac);
	if (fd)
		return ems_container_of(fd, net_user, hmac);

	return NULL;
}

static ems_int net_user_set(ems_hash *h, ems_cchar *nick, ems_cchar *mac)
{
	net_user    *user;

	if (!(nick && mac))
		return EMS_ERR;

	user = net_user_find(h, mac);

	if (user) {
		ems_l_trace("dev(%s) user update %s --> %s", 
				str_text(&user->mac), str_text(&user->nick), nick);

		str_set(&user->nick, nick);
		str_set(&user->mac,  mac);
	}
	else {
		user = net_user_new();

		if (!user)
			return EMS_ERR;

		str_set(&user->nick, nick);
		str_set(&user->mac,  mac);

		ems_l_trace("set user:(%s: %s)", str_text(&user->nick), str_text(&user->mac));

		ems_hash_fd_set_key(&user->hmac, str_text(&user->mac));
		ems_hash_insert(h, &user->hmac);
	}

	return EMS_OK;
}

ems_int net_user_remove_all(ems_hash *h)
{
	ems_hash_walk(h, net_user_hash_cb, NULL);
	return EMS_OK;
}

ems_int net_user_flush_user(ems_hash *h, json_object *req)
{
	json_object *ary, *jobj;
	ems_int      i;
	ems_str      nick, mac;

	ems_assert(req != NULL);

	ary = json_object_object_get(req, "users");
	if (!json_object_is_type(ary, json_type_array))
		return EMS_ERR;

	net_user_remove_all(h);

	str_init(&nick);
	str_init(&mac);

	for (i = 0; i < json_object_array_length(ary); i++) {
		jobj = json_object_array_get_idx(ary, i);

		if (!(jobj && json_object_is_type(jobj, json_type_object)))
			continue;

		ems_json_get_string_def(jobj, "username", &nick, NULL);
		ems_json_get_string_def(jobj, "usermac",  &mac,  NULL);

		net_user_set(h, str_text(&nick), str_text(&mac));
	}

	str_uninit(&nick);
	str_uninit(&mac);

	return EMS_OK;
}

ems_int net_user_setuser(ems_hash *h, json_object *req)
{
	ems_str nick, mac;

	ems_assert(req);

	str_init(&nick);
	str_init(&mac);

	if (!json_object_is_type(req, json_type_object))
		return EMS_ERR;

	ems_json_get_string_def(req, "username", &nick, NULL);
	ems_json_get_string_def(req, "usermac",  &mac,  NULL);

	net_user_set(h, str_text(&nick), str_text(&mac));

	str_uninit(&nick);
	str_uninit(&mac);
	return EMS_OK;
}

ems_int net_user_deluser(ems_hash *h, json_object *req)
{
	net_user *user = NULL;
	ems_str   mac;

	ems_assert(req);

	str_init(&mac);

	if (!json_object_is_type(req, json_type_object))
		return EMS_ERR;

	ems_json_get_string_def(req, "usermac",  &mac,  NULL);

	user = net_user_find(h, str_text(&mac));

	if (user) {
		ems_l_trace("del user: %s", str_text(&user->nick));
		ems_hash_remove(&user->hmac);
		net_user_destroy(user);
		user = NULL;
	}

	str_uninit(&mac);

	return EMS_OK;
}

ems_cchar *net_user_nick(ems_hash *h, ems_cchar *mac)
{
	net_user *user = NULL;

	user = net_user_find(h, mac);
	if (user)
		return str_text(&user->nick);

	return NULL;
}
