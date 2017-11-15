
#ifndef CTRL_HEADER_MSG___
#define CTRL_HEADER_MSG___

ems_int msg_e6wifi_ftp(audit_ctrl *, ems_session *sess, json_object *req);
ems_int msg_e6wifi_citycode(audit_ctrl *, ems_session *sess, json_object *req);
ems_int msg_userinfo(audit_ctrl *, ems_session *sess, json_object *req);
ems_int msg_setuser(audit_ctrl *, ems_session *sess, json_object *req);
ems_int msg_deluser(audit_ctrl *, ems_session *sess, json_object *req);
ems_int msg_mod_ctrl(audit_ctrl *, ems_session *sess, json_object *req);
ems_int msg_mod_info(audit_ctrl *, ems_session *sess, json_object *req);
ems_int msg_mod_set(audit_ctrl *, ems_session *sess, json_object *req);
ems_int msg_mod_get(audit_ctrl *, ems_session *sess, json_object *req);

#endif
