
#include "dt.h"
#include "dt_block.h"
#include "dt_utils.h"

#ifndef DEBUG
int startMemoryTrace(int use_thread_safe)
{
	return OK;
}

int stopMemoryTrace()
{
	return OK;
}

void checkMemoryLeak()
{
}

char *m_malloc(u32_t sz)
{
	return (char *)malloc(sz);
}

char *m_relloc(char *bOld, u32_t sz)
{
	return (char *)realloc(bOld, sz);
}

void m_free(char *b)
{
	if ( b)
		free(b);
}

char *m_memdup(u8block_t *s, u32_t len)
{
	char *b = NULL;

	if ( !s)
		return NULL;

	b = m_malloc(len + 2);
	if ( b) {
		memcpy(b, s, len);
		b[len] = '\0';
	}

	return b;
}


#else
#include "dt_log.h"

// 10M at most
#define MAX_MALLOC_SIZE		((1 << 23) | (1 << 21))

typedef struct _memory_map {
	u8block_t *_start;
	u32_t	  _sz;
	char	  *_desc;
	int	  _ref;
	struct _memory_map *_next;
} block_t, *PBLOCK;

typedef struct _memory_block_{
	block_t *_head;
	block_t *_tail;
	int	_use_lock;
	dt_mtx	_mtx;
} memory_block_t;

static memory_block_t *g_block = NULL;

static void lockMemoryBlock(memory_block_t *mb)
{
	if ( mb && mb->_use_lock) {
		dt_mtx_lock(mb->_mtx);
	}
}

static void unlockMemoryBlock(memory_block_t *mb)
{
	if ( mb && mb->_use_lock) {
		dt_mtx_unlock(mb->_mtx);
	}
}

static block_t *newBlock(u8block_t *pb, u32_t sz, const char *desc)
{
	block_t *pBlock = NULL;

	Assert(pb && desc && sz > 0 && "Never show up this");
	pBlock = (block_t *) malloc(sizeof(block_t));
	if ( pBlock) {
		pBlock->_start = pb;
		pBlock->_sz = sz;
		pBlock->_desc = Strdup(desc);
		pBlock->_ref = NO;
		pBlock->_next = NULL;
	}

	return pBlock;
}

static void releaseBlock(block_t *pBlock)
{
	if ( pBlock) {
		if ( pBlock->_desc) {
			free(pBlock->_desc);
			pBlock->_desc = NULL;
		}

		free(pBlock);
		pBlock = NULL;
	}
}

static block_t *getBlock(memory_block_t *mb, u8block_t *b)
{
	block_t *h = NULL;
	u8block_t *end = NULL;

	if ( mb == NULL) {
		return NULL;
	}

	for ( h = mb->_head; h != NULL; h=h->_next) {
		end = h->_start + h->_sz;
		if ( (b >= h->_start) && (b < end) ) {
			break;
		}
	}

	return h;
}

static int validNode(u8block_t *b, u32_t sz)
{
	block_t *h = NULL;
	u8block_t *end = NULL;

	for ( h = g_block->_head; h != NULL; h=h->_next) {
		end = h->_start + h->_sz;
		if ( !((b >= end) || (b + sz <= h->_start))) {
			log_warn("invalid: b: %p, size: %ld, b+size: %p "
			"h_end: %p, h->_start: %p, h_size: %u, desc; %s",
			b, sz, b+sz, end, h->_start, h->_sz, h->_desc);
			break;
		}
	}

	return h ? NO:YES;
}

static int addBlock(u8block_t *pb, u32_t sz, const char *desc)
{
	int rtn  = 0;
	block_t *b = NULL;

	if ( g_block == NULL) {
		return ERR;
	}

	Assert(sz > 0);
	lockMemoryBlock(g_block);
	do {
		rtn = ERR;
		Assert(pb && (sz > 0) && (sz < MAX_MALLOC_SIZE) && desc);
		Assert(validNode(pb, sz));

		b = newBlock(pb, sz, desc);
		if ( b) {
			if ( g_block->_head == NULL) {
				Assert(g_block->_tail == NULL);
				g_block->_head = g_block->_tail = b;
			} else {
				Assert(g_block->_tail != NULL );
				b->_next = g_block->_tail->_next;
				g_block->_tail->_next = b;
				g_block->_tail = b;
			}

			rtn = OK;
		}
	} while (0);
	unlockMemoryBlock(g_block);

	return rtn;
}

static int deleteBlock(memory_block_t *mb, block_t *b)
{
	block_t *prev = NULL;
	int rtn = ERR;

	Assert(mb && b && "Never show up this line");

	if ( mb->_head == b) {
		mb->_head = mb->_head->_next;
		if ( b == mb->_tail)
			mb->_tail = NULL;
		b->_next = NULL;
	} else  {
		for (prev = mb->_head; prev != NULL; prev = prev->_next) {
			if ( prev->_next == b) {
				if ( b == mb->_tail)
					mb->_tail = prev;
				prev->_next = b->_next;
				b->_next = NULL;

				break;
			}
		}
	}

	releaseBlock(b);

	return rtn;
}

static int delBlock(u8block_t *b)
{
	int rtn = ERR;
	block_t *bk = NULL;

	if ( g_block == NULL) {
		return ERR;
	}

	lockMemoryBlock(g_block);
	do {
		rtn = ERR;
		bk = getBlock(g_block, b);
		if ( bk) {
			rtn = deleteBlock(g_block, bk);
		}
	} while (0);
	unlockMemoryBlock(g_block);

	return rtn;
}

int m_valid_block(char *pb, const char *func, unsigned line)
{
	int rtn = YES;
	block_t *b = NULL;

	lockMemoryBlock(g_block);
	rtn = YES;
	b = getBlock(g_block, pb);
	if ( !b) {
		Assert(0 && "Call startMemoryTrace firstly");
		log_warn("Invalid block: [%s: %u], block address: %p",
				func, line, pb);
		rtn = NO;
	}
	unlockMemoryBlock(g_block);

	return rtn;
}

u32_t sizeofBlock(char *pb)
{
	u32_t sz = 0;
	block_t *b = NULL;

	lockMemoryBlock(g_block);
	b = getBlock(g_block, pb);
	if ( b) {
		sz = b->_sz;
	}
	unlockMemoryBlock(g_block);

	return sz;
}

int setRefBlock(char *pb)
{
	block_t *b = NULL;

	lockMemoryBlock(g_block);
	b = getBlock(g_block, pb);
	if ( b) {
		b->_ref = YES;
	}
	unlockMemoryBlock(g_block);
	return OK;
}

const char *descBlock(char *pb)
{
	static char buf[1024] = {0};
	block_t *b = NULL;

	lockMemoryBlock(g_block);
	b = getBlock(g_block, pb);
	if ( b)
		snprintf(buf, 1024, "%s", b->_desc);
	unlockMemoryBlock(g_block);

	return buf;
}

#ifdef WIN32
typedef USHORT (WINAPI *CaptureStackBackTraceType)(ULONG,ULONG,PVOID*,PULONG);
static CaptureStackBackTraceType _gCapture = NULL;
#endif


int startMemoryTrace(int use_thread_safe)
{
	if ( !g_block) {
		g_block = (memory_block_t *) malloc(sizeof(memory_block_t));
		if (g_block) {
			memset(g_block, Garbage, sizeof(memory_block_t));
			g_block->_head = NULL;
			g_block->_tail = NULL;
			g_block->_use_lock = use_thread_safe?YES:NO;
			dt_mtx_init(g_block->_mtx);
		}
	}

#ifdef WIN32
	if ( !_gCapture) {
		_gCapture = (CaptureStackBackTraceType)
				(GetProcAddress(LoadLibrary("kernel32.dll"),
						"RtlCaptureStackBackTrace"));
	}
#endif

	return OK;
}

int stopMemoryTrace()
{
	block_t *tmp = NULL;
	memory_block_t *tmp_mb = NULL;

	if ( g_block ) {
		checkMemoryLeak();
		lockMemoryBlock(g_block);
		tmp_mb = g_block;
		g_block = NULL;

		while ( NULL != (tmp = tmp_mb->_head) ) {
			tmp_mb->_head = tmp->_next;
			tmp->_next = NULL;
			releaseBlock(tmp);
		}

		tmp_mb->_head = tmp_mb->_tail = NULL;
		unlockMemoryBlock(tmp_mb);

		dt_mtx_destroy(tmp_mb->_mtx);
		memset(tmp_mb, Garbage, sizeof(memory_block_t));
		free(tmp_mb);
		tmp_mb = NULL;
	}

	return OK;
}

void checkMemoryLeak()
{
	block_t *tmp = NULL;

	lockMemoryBlock(g_block);
	if ( g_block) {
		for ( tmp = g_block->_head; tmp != NULL; tmp = tmp->_next) {
			if ( tmp->_ref == NO) {
				log_flt("**memory leak***[%s]", tmp->_desc);
			}
		}
	}
	unlockMemoryBlock(g_block);
}


#ifdef WIN32
#include <windows.h>

#if defined (_MSC_VER) && (_MSC_VER >= 1500) 
#include <DbgHelp.h>

#pragma comment(lib, "Dbghelp.lib")

#define getBackTrace(A,B) do {						\
	unsigned int   i;						\
	void         * stack[16];					\
	unsigned short frames;						\
	SYMBOL_INFO  * symbol;						\
	HANDLE         process;						\
	int rtn = 0;							\
	char *pch = NULL;						\
	int nleft = 0;							\
									\
	if ( !_gCapture) {pch = A; *pch ='\0';break;}			\
									\
	process = GetCurrentProcess();					\
									\
	SymInitialize( process, NULL, TRUE );				\
									\
	frames = _gCapture( 0, 16, stack, NULL );			\
	symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO)+256*sizeof(char),1);\
	if ( symbol) {							\
		symbol->MaxNameLen   = 255;				\
		symbol->SizeOfStruct = sizeof( SYMBOL_INFO );		\
									\
		nleft = B;						\
		pch = A;						\
									\
		for( i = 0; i < frames; i++ ) {				\
			SymFromAddr( process, (DWORD64)(stack[i]), 0, symbol);\
			rtn = snprintf(pch, nleft, "###%i: %s-0x%0X\n",	\
					frames - i - 1, symbol->Name,   \
					symbol->Address);		\
			pch += rtn;					\
			nleft -= rtn;					\
		}							\
		free( symbol );						\
	}								\
}while(0)
#else
#define getBackTrace(A,B) 
#endif

#else
#include <execinfo.h>

#define getBackTrace(A, B) do {					\
	void *array[16];					\
	size_t size;						\
	char **traceString;					\
	size_t i;						\
	int nleft = 0;						\
	char *pch = NULL;					\
	int rtn = 0;						\
								\
	size = backtrace (array, 16);				\
	traceString = backtrace_symbols (array, size);		\
								\
	pch = A;						\
	nleft = B;						\
								\
	if ( traceString) {					\
		for (i = 0; i < size && nleft >0; i++) {	\
			rtn = snprintf(pch, nleft,		\
					"\t\t\t\t->### %s\n", 	\
					traceString[i]);	\
			pch += rtn;				\
			nleft -= rtn;				\
		}						\
								\
		free (traceString);				\
	}							\
}while(0)
#endif

#define _alignedSize(sz)	do {			\
	u32_t	n = 0, int_s;				\
	int_s = sizeof(int);				\
	n = sz % int_s;					\
	if ( n) {					\
		sz += int_s - n;			\
	}						\
} while(0)

char *m_malloc(u32_t sz, const char *fl, unsigned l)
{
	char *b = NULL;
	char desc[1024] = {0};
	char trace[1024] = {0};

	Assert((sz > 0) && (sz < MAX_MALLOC_SIZE));
	_alignedSize(sz);
	b = (char *)malloc(sz);
	if ( b) {
		memset(b, Garbage, sz);
		getBackTrace(trace, 1024);

		snprintf(desc, 1024, "threadid: 0x%08lx file: %s, "
				"line: %u, size: %u, address: %p\n%s",
				dt_gettid(), fl, l, sz, b, trace);
		addBlock(b, sz, desc);
	}

	return (char *)b;
}

char *m_relloc(char *bOld, u32_t sz, const char *fl, unsigned l)
{
	u8block_t *b = NULL;

	Assert(m_valid_block(bOld, fl, l) && (sz >= 0) && (sz<MAX_MALLOC_SIZE));
	if ( sz > 0) {
		b = m_malloc(sz, fl, l);
		if ( !b)
			return NULL;
		memcpy(b, bOld, sizeofBlock(bOld));
	}

	m_free(bOld, fl, l);
	return b;
}

void m_free(char *b, const char *fl, unsigned l)
{
	if ( b) {
		Assert(m_valid_block(b, fl, l) && "Never show up this");

		memset(b, Garbage, sizeofBlock(b));
		delBlock(b);
		free(b);
		b = NULL;
	}
}

char *m_strdup(const char *s, const char *fl, unsigned l)
{
	if ( !s)
		return NULL;

	return m_memdup((u8block_t *)s, strlen(s), fl, l);
}

char *m_memdup(u8block_t *s, u32_t len, const char *fl, unsigned l)
{
	u8block_t *b = NULL;

	if ( !s)
		return NULL;

	b = (u8block_t *)m_malloc(len + 2, fl, l);
	if ( b) {
		memcpy(b, s, len);
		((char *)b)[len] = '\0';
	}

	return b;
}

void m_assert(const char *func, unsigned l)
{
	char trace[1024] = {0};

	getBackTrace(trace, 1024);

	log_warn("Assertionn failed: function: %s, line: %u\n%s\n", func, l, trace);
}


#endif

