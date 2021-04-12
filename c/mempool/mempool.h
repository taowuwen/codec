#ifndef MEM_POOL_HEADER__H
#define MEM_POOL_HEADER__H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "ems_queue.h"
#include "mem_lock.h"

typedef struct mem_pool_s  mem_pool_t;
typedef struct pool_node_s pool_node_t;

typedef pool_node_t *(*pfunc_malloc)(mem_pool_t *pool, uint32_t size);
typedef int (*pfunc_free)(mem_pool_t *pool, pool_node_t *ptr);

struct pool_node_s {
    union {
        ems_queue_t entry;
        struct {
            uint32_t    _size;
            uint32_t    _size_real;
        };
    };
    uintptr_t   _ptr;
};

struct mem_pool_s {

    ems_queue_t entry_freelist;
    ems_queue_t entry_blocklist;

    mem_lock_t  _mtx;

    int         _auto_extend;

    uint64_t    _node_size;
    uint64_t    _total_nodes;
    uint64_t    _total_used_nodes;

    uint64_t    _block_size;
    uint64_t    _total_blocks;
    uint64_t    _total_size;

    uint64_t    _stat_alloc;
    uint64_t    _stat_free;

    pfunc_malloc    _alloc;
    pfunc_free      _free;

};

int mem_pool_init(mem_pool_t *pool, uint64_t block_size, uint64_t node_size);
int mem_pool_uninit(mem_pool_t *pool);

static inline pool_node_t *mem_pool_alloc(mem_pool_t *pool, uint32_t size)
{
    return pool->_alloc(pool, size);
}

static inline int mem_pool_free(mem_pool_t *pool, pool_node_t *ptr)
{
    return pool->_free(pool, ptr);
}

/*
 * max pool node size: 32k
 * min pool node size: 32 Bytes
 * */
#define MAX_POOL_NODE_SIZE      (1024 * 32)
#define MIN_POOL_NODE_SIZE      32

#define MAX_BLOCK_SIZE          (2 * 1024 * 1024)

#endif
