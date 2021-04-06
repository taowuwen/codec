
#ifndef MEM_LOCK_HEADER__
#define MEM_LOCK_HEADER__

#include <pthread.h>

#ifdef USE_SPIN_LOCK

#define mem_lock_t       pthread_spinlock_t
#define mem_lock_init(A) pthread_spin_init(A, 0)
#define mem_lock_destroy pthread_spin_destroy
#define mem_lock_lock    pthread_spin_lock
#define mem_lock_unlock  pthread_spin_unlock

#else

#define mem_lock_t       pthread_mutex_t
#define mem_lock_init(A) pthread_mutex_init(A, NULL)
#define mem_lock_destroy pthread_mutex_destroy
#define mem_lock_lock    pthread_mutex_lock
#define mem_lock_unlock  pthread_mutex_unlock

#endif


#endif
