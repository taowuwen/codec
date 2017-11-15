
#ifndef MEMERYMAP_BLOCK_INEWV
#define MEMERYMAP_BLOCK_INEWV

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef WIN32
typedef unsigned int u32_t;
typedef unsigned char u8block_t;
#define Strdup	_strdup
#else
typedef unsigned int  u32_t;
typedef void u8block_t;
#define Strdup	strdup
#endif

#ifndef YES
#define YES	1
#endif

#ifndef NO
#define NO	0
#endif

#ifndef ERR
#define ERR	-1
#endif

#ifndef OK
#define OK	0
#endif

#ifdef WIN32

#if defined (_MSC_VER) && (_MSC_VER >= 1500)
#	define snprintf(A, B, C, ...)	_snprintf_s(A, B, _TRUNCATE, C, ##__VA_ARGS__)
#	define vsnprintf(A,B,C,...) _vsnprintf_s(A, B, _TRUNCATE, C, ##__VA_ARGS__)
#else
#	define snprintf		_snprintf
#	define vsnprintf	_vsnprintf
#endif

#	define strncasecmp _strnicmp
#endif

#ifdef DEBUG

#define Garbage     0xae
void m_assert(const char *func, unsigned line);
void *m_malloc(u32_t sz, const char *fl, unsigned l);
void *m_realloc(void *bOld, u32_t sz, const char *fl, unsigned l);
void m_free(void *b, const char *fl, unsigned l);
char *m_strdup(const char *s, const char *fl, unsigned l);
void *m_memdup(u8block_t *b, u32_t len, const char *fl, unsigned l);
void *m_calloc(int n, int sz, const char *fl, unsigned l);


#ifndef __FUNCTION__
#define __FUNCTION__	__FILE__
#endif

#define Malloc(A)	m_malloc(A, __FILE__, __LINE__)
#define Realloc(A,B)	m_realloc((char *)A, B, __FILE__, __LINE__)
#define Free(A)		m_free((char *)(A), __FILE__, __LINE__)
#define Assert(A) \
            if (A) {} \
            else {\
                    m_assert(__FUNCTION__, __LINE__);\
            }

#define STRDup(A)	m_strdup(A, __FILE__, __LINE__)
#define MEMDup(A, Len)	m_memdup((u8block_t*)A, (u32_t) Len, __FILE__, __LINE__)
#define Calloc(A,B)	m_calloc(A, B, __FILE__, __LINE__)

int	m_valid_block(char *b, const char *func, unsigned line);
u32_t	sizeofBlock(char *b);
int	setRefBlock(char *b);
const char *descBlock(char *b);

#define validBlock(A)	m_valid_block((char *)A, __FUNCTION__, __LINE__)

#else

#define Garbage     0x00
void *m_malloc(u32_t sz);
void *m_realloc(void *bOld, u32_t sz);
void m_free(void *b);
void *m_memdup(u8block_t *b, u32_t len);
void *m_calloc(int n, int sz);

#define Malloc		m_malloc
#define Realloc(A,B)	m_realloc(A, B)
#define Free(A)		m_free(A)
#define Assert(A)
#define Calloc		m_calloc

#define STRDup		Strdup
#define MEMDup(A,B)	m_memdup((u8block_t*)A, (u32_t)B)

#endif

int startMemoryTrace(int use_thread_safe);
void checkMemoryLeak();
int stopMemoryTrace();

#endif
