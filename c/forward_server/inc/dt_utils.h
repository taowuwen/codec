
#ifndef  DT_NETWORK_UTILS__HEADER___
#define  DT_NETWORK_UTILS__HEADER___


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
typedef DWORD				dt_threadid;
typedef PVOID				dt_threadarg;
typedef DWORD				dt_processid;
#else
typedef pthread_t			dt_threadid;
typedef void*				dt_threadarg;
typedef long				dt_processid;

#endif

#ifdef WIN32
#define dt_getpid()		((unsigned int)GetCurrentProcessId())
#define dt_gettid()		((unsigned int)GetCurrentThreadId())
#elif defined (__APPLE__)
#define dt_getpid()		((unsigned int)getpid())
#define dt_gettid()		(unsigned long)pthread_self()
#else
#define dt_getpid()		((unsigned int)getpid())
#define dt_gettid()		(unsigned long)pthread_self()
#endif


#ifdef WIN32
	#define dt_mtx		HANDLE	
#else
	#define dt_mtx		pthread_mutex_t	
#endif

#ifdef WIN32
	#define dt_mtx_lock(A)		WaitForSingleObject(A, INFINITE)
	#define dt_mtx_unlock(A)	ReleaseMutex(A)
	#define dt_mtx_init(A)		(A = CreateMutex(NULL, FALSE, NULL))
	#define dt_mtx_destroy(A)	CloseHandle(A)
#else
	#define dt_mtx_lock(A)		pthread_mutex_lock(&A)
	#define dt_mtx_unlock(A)	pthread_mutex_unlock(&A)
	#define dt_mtx_init(A)		pthread_mutex_init(&A, NULL)
	#define dt_mtx_destroy(A)	pthread_mutex_destroy(&A)
#endif

#define OK	0
#define ERR	-1
#define YES	1
#define NO	0

dt_char *dt_trim(dt_char *src);


/*
 * for threads control
 * */
typedef void  *(*thread_entry)(dt_threadarg);
dt_int dt_threadcreate(dt_threadid *tid, thread_entry func, dt_threadarg arg);
dt_int dt_threadjoin(dt_threadid tid);
dt_int dt_setnonblocking(dt_int sockfd, dt_int yes);
dt_int dt_gethostbyname(dt_cchar *domain, struct sockaddr_in *dst);

dt_int dt_bin2str(dt_cchar *s, dt_int len, dt_char *d, dt_int d_l);
dt_int dt_str2bin(dt_cchar *s, dt_char *d, dt_int d_l);

#endif
