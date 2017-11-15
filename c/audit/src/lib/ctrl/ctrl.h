
#ifndef CTRL_HEADER___001___
#define CTRL_HEADER___001___


typedef struct _audit_ctrl_s   audit_ctrl;

struct _audit_ctrl_s
{
	ems_session     *sess;
	audit_status     st;
	ems_queue        cmd; /* for cmd requests */
};

ems_int ctrl_change_status(audit_ctrl *ctrl, audit_status st);


#endif
