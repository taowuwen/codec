
#include "audit.h"

ems_cchar *audit_status_str(audit_status st)
{
	switch(st) {
	case st_start:
		return "start";

	case st_init:
		return "init";

	case st_normal:
		return "normal";

	case st_update:
		return "update";

	case st_error:
		return "error";

	case st_stop:
		return "stop";

	default:
		break;
	}

	return "UNKNOWN";
}

