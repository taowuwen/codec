
#ifndef DT_FWD_ERROR_CODE_HEADER___
#define DT_FWD_ERROR_CODE_HEADER___

#include "dt.h"


#define FWD_ERR_SUCCESS				0
#define FWD_ERR_UNKNOWN				-1
#define FWD_ERR_TICKET_EXIST			-2
#define FWD_ERR_TICKET_NOT_EXIST		-3
#define FWD_ERR_FWD_ALREADY_REGISTED		-4
#define FWD_ERR_CONNECT_TIMEOUT			-5
#define FWD_ERR_WAIT_TIMEOUT			-6
#define FWD_ERR_HANDLE_OVERTIME			-7
#define FWD_ERR_INVALID_ARG			-8
#define FWD_ERR_INVALID_TAG			-9

#define FWD_OK		FWD_ERR_SUCCESS
#define FWD_ERR		FWD_ERR_UNKNOWN



dt_int   fwd_lasterr();
dt_cchar *fwd_getlasterrstr(dt_int err);
dt_cchar *fwd_errstr(dt_int err);

#define fwd_lasterrstr()	fwd_getlasterrstr(fwd_lasterr())

#endif
