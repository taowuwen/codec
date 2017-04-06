
#include "dt.h"
#include "fwd_err.h"

dt_int   fwd_lasterr()
{
	return errno;
}


dt_cchar *fwd_getlasterrstr(dt_int err)
{
	static dt_char buf_err[128] = {0};

	snprintf(buf_err, 128, "(%d): %s", err, strerror(err));

	return buf_err;
}

dt_cchar *fwd_errstr(dt_int err)
{
	static dt_char buf[128];

	switch(err) {
	case FWD_ERR_SUCCESS:
		snprintf(buf, sizeof(buf), "FWD_ERR_SUCCESS(%d:0x%x)", err, err);
		break;

	case FWD_ERR_UNKNOWN:
		snprintf(buf, sizeof(buf), "FWD_ERR_UNKNOWN(%d:0x%x)", err, err);
		break;

	case FWD_ERR_TICKET_NOT_EXIST:
		snprintf(buf, sizeof(buf), "FWD_ERR_TICKET_NOT_EXIST(%d:0x%x)", err, err);
		break;
	case FWD_ERR_TICKET_EXIST:
		snprintf(buf, sizeof(buf), "FWD_ERR_TICKET_EXIST(%d:0x%x)", err, err);
		break;
	case FWD_ERR_FWD_ALREADY_REGISTED:
		snprintf(buf, sizeof(buf), "FWD_ERR_FWD_ALREADY_REGISTED(%d:0x%x)", err, err);
		break;
	case FWD_ERR_CONNECT_TIMEOUT:
		snprintf(buf, sizeof(buf), "FWD_ERR_CONNECT_TIMEOUT(%d:0x%x)", err, err);
		break;
	case FWD_ERR_WAIT_TIMEOUT:
		snprintf(buf, sizeof(buf), "FWD_ERR_WAIT_TIMEOUT(%d:0x%x)", err, err);
		break;
	case FWD_ERR_HANDLE_OVERTIME:
		snprintf(buf, sizeof(buf), "FWD_ERR_HANDLE_OVERTIME(%d:0x%x)", err, err);
		break;

	case FWD_ERR_INVALID_ARG:
		snprintf(buf, sizeof(buf), "FWD_ERR_INVALID_ARG(%d:0x%x)", err, err);
		break;

	default:
		snprintf(buf, sizeof(buf), "FWD_ERR**unknown**(%d:0x%x)", err, err);
		break;
	}

	return buf;
}
