
#ifndef  EMS_NETWORK_UTILS__HEADER___
#define  EMS_NETWORK_UTILS__HEADER___


#ifdef WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#ifdef WIN32
typedef DWORD				ems_threadid;
typedef PVOID				ems_threadarg;
typedef DWORD				ems_processid;
#else
typedef pthread_t			ems_threadid;
typedef void*				ems_threadarg;
typedef long				ems_processid;

#endif

#ifdef WIN32
#define ems_getpid()		((unsigned int)GetCurrentProcessId())
#define ems_gettid()		((unsigned int)GetCurrentThreadId())
#elif defined (__APPLE__)
#define ems_getpid()		((unsigned int)getpid())
#define ems_gettid()		(unsigned long)pthread_self()
#else
#define ems_getpid()		((unsigned int)getpid())
#define ems_gettid()		(unsigned long)pthread_self()
#endif


#ifdef WIN32
	#define ems_mtx		HANDLE	
#else
	#define ems_mtx		pthread_mutex_t	
#endif

#ifdef WIN32
	#define ems_mtx_lock(A)		WaitForSingleObject(A, INFINITE)
	#define ems_mtx_unlock(A)	ReleaseMutex(A)
	#define ems_mtx_init(A)		(A = CreateMutex(NULL, FALSE, NULL))
	#define ems_mtx_destroy(A)	CloseHandle(A)
#else
	#define ems_mtx_lock(A)		pthread_mutex_lock(&A)
	#define ems_mtx_unlock(A)	pthread_mutex_unlock(&A)
	#define ems_mtx_init(A)		pthread_mutex_init(&A, NULL)
	#define ems_mtx_destroy(A)	pthread_mutex_destroy(&A)
#endif

#define OK	0
#define ERR	-1
#define YES	1
#define NO	0

ems_char *ems_trim(ems_char *src);

#ifdef USE_MULTI_THREAD

/*
 * for threads control
 * */
typedef void  *(*thread_entry)(ems_threadarg);
ems_int ems_threadcreate(ems_threadid *tid, thread_entry func, ems_threadarg arg);
ems_int ems_threadjoin(ems_threadid tid);
#endif
ems_int ems_setnonblocking(ems_int sockfd, ems_int yes);
ems_int ems_gethostbyname(ems_cchar *domain, struct sockaddr_in *dst);

ems_int ems_bin2str(ems_cchar *s, ems_int len, ems_char *d, ems_int d_l);
ems_int ems_str2bin(ems_cchar *s, ems_char *d, ems_int d_l);

ems_int ems_cpucore();
ems_int ems_pagesize();
ems_void ems_printhex(ems_cchar *s, ems_int len);
ems_int ems_memusage();
ems_int ems_cpuusage();
ems_int ems_reset_rlimit();

ems_cchar *ems_itoa(ems_int i);
ems_int    ems_atoi(ems_cchar *a);
long long  ems_atoll(ems_cchar *str);
ems_long   ems_atol(ems_cchar *a);

ems_char   *url_encode(ems_cchar *src, ems_int lstr);
ems_char   *url_decode(ems_char *src, ems_int lstr);

ems_int rm_rf(ems_cchar *path);
ems_int mkdir_p(ems_cchar *path);

#endif
