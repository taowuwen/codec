
#ifndef FWD_HEADER___ALL_____
#define FWD_HEADER___ALL_____


#include "dt.h"
#include "fwd_err.h"
#include "fwd_utils.h"


typedef struct _fwd_request_s  fwd_request;
typedef struct _fwd_response_s fwd_response;


struct _fwd_request_s {
	unsigned short len;
	unsigned short ver;
	unsigned short cmd;
	unsigned short rsv;
};

struct _fwd_response_s {
	unsigned short len;
	unsigned short ver;
	unsigned short cmd;
	unsigned short st;
};


#define FWD_VERSION	1
#define FWD_CONTINUE	0xff11aabb
#define FWD_BUFFER_FULL	0xff11aabc

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

#define fwd_start_memory_trace	startMemoryTrace
#define fwd_stop_memory_trace	stopMemoryTrace

#define fwd_container_of	dt_container_of

#define fwd_strlen(a)	(a?strlen(a):0)

#endif
