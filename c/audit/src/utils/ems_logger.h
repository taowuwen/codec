
#ifndef EMS_LOGGER_LOGGER_HEADER___
#define EMS_LOGGER_LOGGER_HEADER___

typedef struct ems_logger_s_  ems_logger;

struct ems_logger_s_ 
{
	FILE      *fp;
	ems_int    level;
	ems_buffer buf;
};

#define EMS_LOG_TRACE	5
#define EMS_LOG_INFO	4
#define EMS_LOG_WARN	3
#define EMS_LOG_ERR	2
#define EMS_LOG_FAULT	1


#ifdef DEBUG
ems_void _ems_logger(
		ems_int level, 
		ems_cchar *fl, 
		ems_uint l, 
		ems_logger *log, 
		ems_cchar *fmt, ...);


#define ems_logger_trace(log, fmt, args...)	\
	_ems_logger(EMS_LOG_TRACE, __FUNCTION__, __LINE__, log, fmt, ##args)

#define ems_logger_info(log,  fmt, args...) \
	_ems_logger(EMS_LOG_INFO, __FUNCTION__, __LINE__, log, fmt, ##args)

#define ems_logger_warn(log,  fmt, args...) \
	_ems_logger(EMS_LOG_WARN, __FUNCTION__, __LINE__, log, fmt, ##args)

#define ems_logger_error(log, fmt, args...)	\
	_ems_logger(EMS_LOG_ERR,  __FUNCTION__, __LINE__, log,fmt, ##args)

#define ems_logger_fault(log, fmt, args...)	\
	_ems_logger(EMS_LOG_FAULT, __FUNCTION__, __LINE__, log, fmt, ##args)

#else

ems_void _ems_logger(ems_int level, ems_logger *log, ems_cchar *fmt, ...);
#define ems_logger_trace(log, fmt, args...)	_ems_logger(EMS_LOG_TRACE, log, fmt, ##args)
#define ems_logger_info(log,  fmt, args...)	_ems_logger(EMS_LOG_INFO, log, fmt, ##args)
#define ems_logger_warn(log,  fmt, args...)	_ems_logger(EMS_LOG_WARN, log, fmt, ##args)
#define ems_logger_error(log, fmt, args...)	_ems_logger(EMS_LOG_ERR,  log, fmt, ##args)
#define ems_logger_fault(log, fmt, args...)	_ems_logger(EMS_LOG_FAULT,log, fmt, ##args)
#endif



ems_int ems_logger_init(ems_logger *log, FILE *fp, ems_int level);
ems_int  ems_logger_reset(ems_logger *log);
ems_void ems_logger_uninit(ems_logger *log);

#define ems_logger_set_fp(log, f)	((log)->fp=(f))
#define ems_logger_set_level(log, l)	((log)->level = (l))



#endif
