
#ifndef EMS_CHILD_PROCESS_MAIN__HEADER__
#define EMS_CHILD_PROCESS_MAIN__HEADER__

#include "ems_common.h"
#include "ems_timer.h"
#include "ems_hash.h"
#include "ems_buffer.h"
#include "ems_sock.h"
#include "ems_event.h"
#include "ems_logger.h"
#include "ems_msg.h"
#include "ems_session.h"

typedef struct _ems_time_s      ems_time;
typedef enum _ems_app_type_s    ems_app_type;

enum _ems_app_type_s
{
	ty_min    = -1,
	ty_fw	  = 0, /* for firewall */
	ty_portal,
	ty_radius,
	ty_bwlist, /* black white list */
	ty_downlink, /* for bridge uplink, check downlink device number */
	ty_bridge, /* for bridge downlink*/
	ty_ctrl,  /* for user api */
	ty_net,   /* for network status detect */
	ty_client, /* for updator*/
	ty_max
};

struct _ems_time_s
{
	struct timeval  create;
	struct timeval  modify;
	struct timeval  expire;
};



#define EMS_TIMEOUT_SESSION_INIT	(12000) // 12s
#define EMS_TIMEOUT_USER_ACCEPTED	(3000)  // 3s
#define EMS_TIMEOUT_WAITTING            (1500)  // 1.5s 
#define EMS_TIMEOUT_ESTABLISHED		(12000) // 12s 
#define EMS_TIMEOUT_SENDMSG		(3000)   // 1 secs, for send out the last msgs
#define EMS_TIMEOUT_RETRY		(6000)  // 6s , for retry connect, handle...
#define EMS_TIMEOUT_HEARTBEAT		(3000)	 // 3s heart beat with CM
#define EMS_TIMEOUT_AUTH		(3000)	 // 3 secs, for auth

#define EMS_LOGIC_PORT			8196
#define EMS_AAA_PORT			6918
#define EMS_MAX_NOFILE			61000

/*
 * both logic and aaa should reality's api
 * */

ems_logger  *logger();
ems_event   *eventer();
ems_queue   *timeouter();


#define ems_l_trace(fmt, args...) ems_logger_trace(logger(), fmt, ##args)
#define ems_l_info(fmt, args...)  ems_logger_info(logger(), fmt, ##args)
#define ems_l_warn(fmt, args...)  ems_logger_warn(logger(), fmt, ##args)
#define ems_l_error(fmt, args...) ems_logger_error(logger(), fmt, ##args)
#define ems_l_fault(fmt, args...) ems_logger_fault(logger(), fmt, ##args)

#define ems_traceline()	ems_l_trace("file: %s, line: %u", __FILE__, __LINE__)


#endif
