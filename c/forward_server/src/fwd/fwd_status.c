
#include "fwd_main.h"
#include "fwd_status.h"

dt_cchar *fwd_status_str(fwd_status st)
{
	switch(st) {

	case ST_ACCEPTED:
		return "ACCEPTED";

	case ST_INITED:
		return "INITED";

	case ST_AUTH:
		return "AUTH";

	case ST_WATTING:
		return "WAITTING";
		
	case ST_EST:
		return "ESTABLISHED";

	case ST_CLEAN:
		return "CLEAN";

	case ST_ERROR:
		return "ERROR";

	default:
		break;
	}

	return "ST_UNKNOWN";
}

