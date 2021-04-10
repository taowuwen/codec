
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

typedef struct {
    int       _size;
    uintptr_t _stack[MAX_STACK_SIZE];
} mem_stack_t;

typedef struct {
    uint64_t    _size;

    uint64_t    _stat_alloc;
    uint64_t    _stat_free;

    uint64_t    _total_node;
    uint64_t    _total_size;
    uint64_t    _total_size_real;

    mem_pool_t *pool;
} mem_class_t;

typedef struct {
    ems_queue_t entry;
    uintptr_t   _ptr;
    uint64_t    _size;
    uint64_t    _size_real;
    mem_stack_t *_stack;
    mem_class_t *_cls;
} mem_node_t;


void *mem_malloc(size_t size);
void mem_free(void *ptr);
void *mem_realloc(size_t *ptr, size_t size);
void *mem_calloc(size_t nmemb, size_t size);

#endif
