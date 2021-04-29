#include "mempool.h"
#include <assert.h>

typedef struct {
    ems_queue_t entry;
    uintptr_t   _ptr;
    uint64_t    _size;
} pool_block_t;

static pool_node_t *pool_malloc(mem_pool_t *pool, uint32_t size)
{
    assert(pool && size > 0);
    assert(size <= pool->_node_size);

    return NULL;
}

static int pool_free(mem_pool_t *pool, pool_node_t *ptr)
{
    return -1;
}

static pool_node_t *system_malloc(mem_pool_t *pool, uint32_t size)
{
    pool_node_t *pn = NULL;
    uint32_t size_total = size + sizeof(pool_node_t);

    assert(pool && size);

    pn = (pool_node_t *)malloc(size_total);
    if (pn) {
        pn->_size = size;
        pn->_size_real = size_total;
        pn->_ptr = (uintptr_t)(pn + 1);

        atomic_inc(&pool->_total_blocks);
        atomic_add(&pool->_total_size, size_total);

        atomic_inc(&pool->_stat_alloc);
    }

    return pn;
}

static int system_free(mem_pool_t *pool, pool_node_t *pn)
{
    assert(pool && pn && pn->_size && pn->_size_real);

    atomic_inc(&pool->_stat_free);
    atomic_dec(&pool->_total_blocks);
    atomic_sub(&pool->_total_size, pn->_size_real);

    free(pn);

    return 0;
}

int mem_pool_init(mem_pool_t *pool, uint64_t block_size, uint64_t node_size)
{
    if (!pool)
        return -1;

    memset(pool, 0, sizeof(mem_pool_t));

    ems_queue_init(&pool->entry_freelist);
    ems_queue_init(&pool->entry_blocklist);
    mem_lock_init(&pool->_mtx);

    pool->_auto_extend = 1;

    pool->_alloc = system_malloc;
    pool->_free  = system_free;

#ifndef MEM_POOL_DISABLED
    if (node_size < MAX_POOL_NODE_SIZE) {
        pool->_alloc = pool_malloc;
        pool->_free  = pool_free;

        pool->_block_size = block_size;
        pool->_node_size  = node_size;
    }
#endif

    return 0;
}

int mem_pool_uninit(mem_pool_t *pool)
{
    if (!pool) {
        return -1;
    }

    ems_queue_clean(&pool->entry_freelist);
    ems_queue_clear(&pool->entry_blocklist, pool_block_t, entry, free);
    mem_lock_destroy(&pool->_mtx);

    memset(pool, 0, sizeof(mem_pool_t));
    return 0;
}
