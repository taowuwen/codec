
#ifndef  EMS_HEADERE_CLIENT_FOR_BRIGDE___
#define  EMS_HEADERE_CLIENT_FOR_BRIGDE___

typedef struct _ems_bridge_s ems_bridge;

struct _ems_bridge_s 
{
	ems_status  st;

	ems_session *sess;
};

ems_int br_change_status(ems_bridge *br, ems_status st);


#endif
