
#ifndef NET_USER_INFO_HEADER___
#define NET_USER_INFO_HEADER___

ems_int net_user_remove_all(ems_hash *h);
ems_int net_user_flush_user(ems_hash *h, json_object *req);
ems_int net_user_setuser(ems_hash *h, json_object *req);
ems_int net_user_deluser(ems_hash *h, json_object *req);
ems_cchar *net_user_nick(ems_hash *h, ems_cchar *mac);

#endif
