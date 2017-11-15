
#ifndef CLASS_HEADER____
#define CLASS_HEADER____

#include "ems_core.h"

ems_int audit_sendmsg(ems_uint s, ems_uint d, ems_uint evt, ems_uchar *arg);
ems_int audit_send_json(ems_uint s, ems_uint d, ems_uint evt, json_object *obj);

#define audit_postmsg(s, d, evt) audit_send_json(s, d, evt, NULL)

ems_void audit_modules_uninit();
ems_int  audit_modules_init();


#endif
