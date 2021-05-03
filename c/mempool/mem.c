#include "mem.h"

int mem_init()
{
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
