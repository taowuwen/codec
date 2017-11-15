
#include "audit.h"
#include "ems_core.h"
#include "ems_client.h"
#include "class.h"
#include "ctrl.h"
#include "out.h"
#include "net.h"


ems_int msg_e6wifi_ftp(audit_ctrl *ctrl, ems_session *sess, json_object *req)
{
	if (!req)
		return EMS_ERR;

	if (!json_object_is_type(req, json_type_object))
		return EMS_ERR;

	return out_sendmsg(mod_ctrl, id_out_16wifi, MSG_E6WIFI_FTP, req);
}

ems_int msg_e6wifi_citycode(audit_ctrl *ctrl, ems_session *sess, json_object *req)
{
	if (!req)
		return EMS_ERR;

	if (!json_object_is_type(req, json_type_object))
		return EMS_ERR;

	/* for now, just send to id_http module */
	return net_sendmsg(mod_ctrl, id_http, MSG_E6WIFI_CITYCODE, req);
}

ems_int msg_userinfo(audit_ctrl *ctrl, ems_session *sess, json_object *req)
{
	if (!req)
		return EMS_ERR;

	if (!json_object_is_type(req, json_type_object))
		return EMS_ERR;

	return audit_sendmsg(mod_ctrl, mod_net, MSG_USERINFO, (ems_uchar *)req);
}

ems_int msg_setuser(audit_ctrl *ctrl, ems_session *sess, json_object *req)
{
	if (!req)
		return EMS_ERR;

	if (!json_object_is_type(req, json_type_object))
		return EMS_ERR;

	return audit_sendmsg(mod_ctrl, mod_net, MSG_SETUSER, (ems_uchar *)req);
}

ems_int msg_deluser(audit_ctrl *ctrl, ems_session *sess, json_object *req)
{
	if (!req)
		return EMS_ERR;

	if (!json_object_is_type(req, json_type_object))
		return EMS_ERR;

	return audit_sendmsg(mod_ctrl, mod_net, MSG_DELUSER, (ems_uchar *)req);
}

ems_int msg_mod_ctrl(audit_ctrl *ctrl, ems_session *sess, json_object *req)
{
	if (!req)
		return EMS_ERR;

	if (!json_object_is_type(req, json_type_object))
		return EMS_ERR;

	audit_sendmsg(mod_ctrl, mod_net, MSG_MODULE_CTRL, (ems_uchar *)req);
	audit_sendmsg(mod_ctrl, mod_sys, MSG_MODULE_CTRL, (ems_uchar *)req);
	audit_sendmsg(mod_ctrl, mod_out, MSG_MODULE_CTRL, (ems_uchar *)req);

	return EMS_OK;
}

ems_int msg_mod_info(audit_ctrl *ctrl, ems_session *sess, json_object *req)
{
	json_object *root;

	root = json_object_new_object();

	audit_sendmsg(mod_ctrl, mod_net, MSG_MODULE_INFO, (ems_uchar *)root);
	audit_sendmsg(mod_ctrl, mod_sys, MSG_MODULE_INFO, (ems_uchar *)root);
	audit_sendmsg(mod_ctrl, mod_out, MSG_MODULE_INFO, (ems_uchar *)root);

	sess_response_set(sess, root);
	return EMS_OK;
}

ems_int msg_mod_set(audit_ctrl *ctrl, ems_session *sess, json_object *req)
{
	if (!req)
		return EMS_ERR;

	if (!json_object_is_type(req, json_type_object))
		return EMS_ERR;

	audit_sendmsg(mod_ctrl, mod_net, MSG_MODULE_SET, (ems_uchar *)req);
	audit_sendmsg(mod_ctrl, mod_sys, MSG_MODULE_SET, (ems_uchar *)req);
	audit_sendmsg(mod_ctrl, mod_out, MSG_MODULE_SET, (ems_uchar *)req);

	return EMS_OK;
}

ems_int msg_mod_get(audit_ctrl *ctrl, ems_session *sess, json_object *req)
{
	json_object *root;

	root = json_object_new_object();

	audit_sendmsg(mod_ctrl, mod_net, MSG_MODULE_GET, (ems_uchar *)root);
	audit_sendmsg(mod_ctrl, mod_sys, MSG_MODULE_GET, (ems_uchar *)root);
	audit_sendmsg(mod_ctrl, mod_out, MSG_MODULE_GET, (ems_uchar *)root);

	sess_response_set(sess, root);
	return EMS_OK;
}
