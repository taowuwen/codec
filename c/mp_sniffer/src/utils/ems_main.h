
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
