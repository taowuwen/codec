#ifndef EMS_CONTROL_HEADER___
#define EMS_CONTROL_HEADER___

typedef struct _ems_ctrl_s   ems_ctrl;

struct _ems_ctrl_s
{
	ems_session   *sess;
	ems_status     st;
	ems_queue      cmd; /* for cmd requests */
};

typedef struct _ems_ddns_s	ems_ddns;

struct _ems_ddns_s {
	ems_str param;
	ems_str	port;
};

ems_int ctrl_change_status(ems_ctrl *ctrl, ems_status st);

#endif
