
#ifndef EMS_HEADER_FOR_RADIUS_____
#define EMS_HEADER_FOR_RADIUS_____

#include <r_config.h>
#include <includes.h>
#include <freeradius-client.h>


#define RADIUS_ERR_SUCCESS		0
#define RADIUS_ERR_CANNOT_CONNECT	0x4000
#define RADIUS_ERR_REJECT		0x4001
#define RADIUS_ERR_NETWORK		0x4002


typedef struct _ems_device_s  ems_device;
typedef struct _ems_radius_s  ems_radius;
struct _ems_device_s {
	ems_queue    entry;
	ems_session *sess_dev;    /* do ping */
	ems_session *sess;        /* do radius stuff */
	ems_void    *ctx;

	struct {
		ems_str  name;
		ems_str  pass;
		ems_str  mac;
		ems_str  ip;
		ems_str  sessid;
		struct   timeval  start;

		off_t    in_bytes;
		off_t    out_bytes;
		off_t    in_pkgs;
		off_t    out_pkgs;
	} user;

	ems_int      retry_times;
	ems_int      retry_timeout;
	ems_int      report_period;
	ems_int      disconnect;
	ems_uchar    vector[AUTH_VECTOR_LEN];     
	ems_int      total_len;
	ems_int      reason;
	ems_int      err;

	/* for ping */
	ems_uint     ntrans;
	ems_int      ident;

	VALUE_PAIR  *vp_out;
	VALUE_PAIR  *vp_in;
	AUTH_HDR    *auth_out;
	AUTH_HDR    *auth_in;
	ems_status   st;
};

struct _ems_radius_s
{
	ems_queue  dev_entry;
	rc_handle *rh;
	ems_uint   seq_nbr;

	ems_int    lasterr;
	ems_str    secret;
	ems_str    auth_addr;
	ems_int    auth_port;
	ems_str    acct_addr;
	ems_int    acct_port;

	ems_int    retry_times;
	ems_int    retry_timeout;
	ems_int    report_period;
	ems_int    disconnect;
};


ems_int dev_change_status(ems_device *dev, ems_status st);
ems_device *ems_device_new();
ems_void    ems_device_destroy(ems_device *dev);

#endif
