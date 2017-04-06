
#include "t_main.h"
#include "t_status.h"


dt_cchar *t_session_status_str(t_session_status st)
{
	switch(st) {

	case ST_SESSION_INIT:
		return "SESS_INIT";
	
	case ST_SESSION_NORMAL:
		return "SESS_NORMAL";

	case ST_SESSION_ERROR:
		return "SESS_ERROR";

	default:
		break;
	}

	return "SESS_UNKNOWN";
}


dt_cchar *t_cm_status_str(t_cm_status st)
{
	switch(st) {

	case ST__CM_INIT:
		return "CM_INIT";

	case ST__CM_REGISTER:
		return "CM_REGISTER";

	case ST__CM_HB:
		return "CM_HB";

	case ST__CM_SENDHB:
		return "CM_SEND_HB";

	case ST__CM_REQUEST_FWD:
		return "CM_REQUEST_FWD";

	default:
		break;
	}

	return "CM_UNKNOWN";
}


dt_cchar *t_fwd_status_str(t_fwd_status st)
{
	switch (st) {

	case ST_FWD_INIT:
		return "FWD_INIT";

	case ST_FWD_CONN:
		return "FWD_CONN";

	case ST_FWD_REG:
		return "FWD_REG";

	case ST_FWD_EST:
		return "FWD_EST";

	case ST_FWD_HB:
		return "FWD_HB";

	case ST_FWD_SENDFILE:
		return "FWD_SENDFILE";

	default:
		break;
	}

	return "FWD_UNKNOWN";
}
