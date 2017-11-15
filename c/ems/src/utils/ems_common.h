
#ifndef EMS_HEADER___ALL_____
#define EMS_HEADER___ALL_____

#include "ems.h"

#ifdef _LIBC
# include <endian.h>
# if __BYTE_ORDER == __BIG_ENDIAN
#  define WORDS_BIGENDIAN 1
# endif
#endif


#pragma pack(push, 1)

typedef union _ems_request_s  ems_request;
typedef union _ems_response_s ems_response;


union _ems_request_s {
	ems_uchar val[8];
	struct {
		union {
			ems_uint val;
			struct {
#ifdef WORDS_BIGENDIAN
				union {
					unsigned short mod;
					struct {
						ems_uchar :7;
						ems_uchar ty:1;
					};
				};
				ems_ushort msg;
#else
				ems_ushort msg;
				union {
					unsigned short mod;
					struct {
						ems_uchar :7;
						ems_uchar ty:1;
					};
				};
#endif
			};
		} tag;
		ems_uint  len;
	};
};

union _ems_response_s {
	ems_uchar val[12];
	struct {
		union {
			ems_uint val;
			struct {
#ifdef WORDS_BIGENDIAN
				union {
					unsigned short mod;
					struct {
						ems_uint :7;
						ems_uint ty:1;
					};
				};
				ems_ushort msg;
#else
				ems_ushort msg;
				union {
					unsigned short mod;
					struct {
						ems_uint :7;
						ems_uint ty:1;
					};
				};
#endif
			};
		} tag;
		ems_uint  len;
		ems_uint  st;
	};
};


#define EMS_CONTINUE	0xff11aabb
#define EMS_BUFFER_FULL	0xff11aabc
#define EMS_BUFFER_INSUFFICIENT	 0xff11aabd
#define EMS_REQUEST	0
#define EMS_RESPONSE	1
#define EMS_SLOW_DOWN		0xff11aabe

#define SIZE_REQUEST	sizeof(ems_request)
#define SIZE_RESPONSE	sizeof(ems_response)

#define INTSIZE		4
#define ALIGNLEN(l)	((l)%INTSIZE==0)?(l):(((l)/INTSIZE + 1) * INTSIZE);

#define getword(buf, w) do {				\
	w = (unsigned int)ntohl(*(unsigned int *)buf);	\
	buf += INTSIZE;					\
} while (0)

#define putword(buf, w) do {				\
	*(unsigned long *)buf = htonl(w);		\
	buf += INTSIZE;					\
} while (0)

#define putmem(buf, m, l) do {			\
	putword(buf, l);			\
	memcpy(buf, m, l);			\
	buf += ALIGNLEN(l);			\
} while (0)

#define getmem(buf, m, l) do {			\
	getword(buf, l);			\
	memcpy(m, buf, l);			\
	buf += ALIGNLEN(l);			\
} while (0)

#define putstr(buf, str) do {				\
	int	l = (str == NULL?0:strlen(str));	\
	putmem(buf, str, l);				\
} while (0)

#define getstr(buf, str) do {			\
	int l;					\
	getmem(buf, str, l);			\
} while (0)


#pragma pack(pop)

#define ems_start_memory_trace	startMemoryTrace
#define ems_stop_memory_trace	stopMemoryTrace

#define ems_strlen(a)	(a?strlen(a):0)

#ifndef EMS_YES
#define EMS_YES		1
#endif

#ifndef EMS_NO
#define EMS_NO		0
#endif

#define EMS_FLG_DEFAULT		(1 << 0)
#define EMS_FLG_ENABLE		(1 << 1)
#define EMS_FLG_INHERIET	(1 << 2)
#define EMS_FLG_KICKOFF		(1 << 3)
#define EMS_FLG_ONLINE		(1 << 31)


#define ems_flag_like(fld, flg)		((fld) & (flg))
#define ems_flag_unlike(fld, flg)	!ems_flag_like(fld, flg)
#define ems_flag_set(fld, flg)		((fld) |= (flg))
#define ems_flag_unset(fld, flg)	if(ems_flag_like(fld, flg)) ((fld) ^= (flg))
#define ems_flag_test			ems_flg_like


#define EMS_MODULE_WEB		0x01
#define EMS_MODULE_AC		0x02
#define EMS_MODULE_LOGIC	0x03
#define EMS_MODULE_AAA		0x04


#define EMS_TICKET_LEN		20

#endif
