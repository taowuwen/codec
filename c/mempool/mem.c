#include "mem.h"

typedef struct {
    uint32_t n_size;
    mem_mgmt_t *mgmt;
} mem_sched_t;

/*
 * memory pool build method, default is factor:
 *      mannual, factor, step
 *
 * factor: default = 2
 * step: default: 128
 * mannual: default use factor to create
 *
 * scheduler? RR, W?
 *
 * constant hash?
 * */
int mem_init()
{
    // secure_getenv(MEMORY_POOL_BUILD_METHOD);
    // secure_getenv(MEMORY_POOL_FACTOR);
    return 0;
}

int mem_uninit()
{
    return 0;
}

int mem_leak_check()
{
    return 0;
}

void *mem_malloc(size_t size)
{
    return malloc(size);
}

void mem_free(void *ptr)
{
    free(ptr);
}

void *mem_realloc(size_t *ptr, size_t size)
{
    return realloc(ptr, size);
}

void *mem_calloc(size_t nmemb, size_t size)
{
    return calloc(nmemb, size);
}
