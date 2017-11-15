
#ifndef EMS_NET_TYPES_HEADERS___
#define EMS_NET_TYPES_HEADERS___



typedef int		ems_int;
typedef unsigned int 	ems_uint;
typedef void		ems_void;
typedef unsigned char	ems_uchar;
typedef const char 	ems_cchar;
typedef char		ems_char;
typedef unsigned long	ems_ulong;
typedef long		ems_long;
typedef unsigned short  ems_ushort;
typedef short 		ems_short;



#define EMS_ERR_SUCCESS				0
#define EMS_ERR_UNKNOWN				-1
#define EMS_ERR_TICKET_EXIST			-2
#define EMS_ERR_TICKET_NOT_EXIST		-3
#define EMS_ERR_ACTIVE_CODD_ALREADY_REGISTED	-4
#define EMS_ERR_CONNECT_TIMEOUT			-5
#define EMS_ERR_WAIT_TIMEOUT			-6
#define EMS_ERR_HANDLE_OVERTIME			-7
#define EMS_ERR_INVALID_ARG			-8



#define EMS_OK		EMS_ERR_SUCCESS
#define EMS_ERR		EMS_ERR_UNKNOWN

ems_cchar *ems_geterrmsg(ems_int err);
#define ems_lasterr()		errno
#define ems_lasterrmsg()	ems_geterrmsg(ems_lasterr())


#endif
