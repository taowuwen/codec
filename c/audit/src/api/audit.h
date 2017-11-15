
#ifndef AUDIT_HEADER___FIRST____
#define AUDIT_HEADER___FIRST____

#include "ems_main.h"
#include "audit_msg.h"
#include "audit_class.h"

typedef enum _audit_status_e 
{
	st_min = -1,
	st_start  = 0,
	st_init   = 1,
	st_normal = 2,
	st_update = 3,
	st_error  = 4,
	st_stop   = 5,
	st_max 
} audit_status;

ems_cchar *audit_status_str(audit_status st);

#define AUDIT_ADDR	"127.0.0.1"
#define AUDIT_PORT	9110

ems_int output_log(ems_uint src, ems_uint ty, ems_cchar *log);


#endif
