
#ifndef __MEMMORY_HEADER__
#define __MEMMORY_HEADER__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <execinfo.h>
#include <stdint.h>
#include "ems_queue.h"
#include "mempool.h"

#define MAX_STACK_SIZE  20

typedef struct _mem_mgmt_s mem_mgmt_t;

typedef struct {
    int       _size;
    uintptr_t _stack[MAX_STACK_SIZE];
} mem_stack_t;

typedef struct {
    uint64_t    size;

    uint64_t    stat_alloc;
    uint64_t    stat_free;

    uint64_t    total_node;
    uint64_t    total_size;
    uint64_t    total_size_real;

    mem_pool_t *pool;
    mem_mgmt_t *mgmt;
} mem_class_t;

typedef struct {
    ems_queue_t entry;
    uintptr_t   _ptr;
    uint64_t    _size;
    uint64_t    _size_real;
    mem_stack_t *_stack;
    mem_class_t *_cls;
} mem_node_t;

struct _mem_mgmt_s {
    int enable;
    int enable_backtrace;

    int          n_pool;
    mem_pool_t  *pool;

    int          n_cls;
    mem_class_t *cls;

    uint64_t total_node;
    uint64_t total_size;
    uint64_t total_size_real;

    uint64_t stat_alloc;
    uint64_t stat_free;
};

int mem_init();
int mem_uninit();
int mem_leak_check();

void *mem_malloc(size_t size);
void mem_free(void *ptr);
void *mem_realloc(size_t *ptr, size_t size);
void *mem_calloc(size_t nmemb, size_t size);

#endif
