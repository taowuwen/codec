
#ifndef EMS_MEMORY_MANAGEMENT_H_
#define EMS_MEMORY_MANAGEMENT_H_

#ifdef USE_MEM_POOL

int mem_pool_uninit();
int mem_pool_init(int thread_safe, float factor);
void *mem_malloc(int sz);
void mem_free(void *ptr);
void *mem_realloc(void *ptr, int sz);

#endif

#endif
