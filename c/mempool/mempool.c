#include "mempool.h"
#include <assert.h>

typedef struct {
    ems_queue_t entry;
    uint64_t    _size;
    uintptr_t   _ptr;
} pool_block_t;

static int pool_expand(mem_pool_t *pool, uint64_t ptr_start, uint64_t ptr_end)
{
    pool_node_t *pn;

    for(; ptr_start + pool->_node_size <= ptr_end; ptr_start += pool->_node_size) {

        pn = (pool_node_t *)(uintptr_t)ptr_start;

        ems_queue_insert_tail(&pool->entry_freelist, &pn->entry);
    }

    return 0;
}

static int mempool_expand(mem_pool_t *pool)
{
    pool_block_t *blk = NULL;

    blk = (pool_block_t *)malloc(sizeof(pool_block_t) + pool->_block_size);

    if (!blk)
        return -1;

    blk->_size = pool->_block_size;
    blk->_ptr = (uint64_t)(blk + 1);
    ems_queue_insert_tail(&pool->entry_blocklist, &blk->entry);

    pool_expand(pool, blk->_ptr, blk->_ptr + blk->_size);

    return 0;
}

static pool_node_t *pool_malloc(mem_pool_t *pool, uint32_t size)
{
    pool_node_t *pn = NULL;
    ems_queue_t *list;
    assert(pool && size > 0);
    assert(size <= pool->_node_size);

    mem_lock_lock(&pool->_mtx);

    if (ems_queue_empty(&pool->entry_freelist)) {
        if (!pool->_auto_extend && pool->_total_blocks) {
            mem_lock_unlock(&pool->_mtx);
            return NULL;
        }

        if (mempool_expand(pool) == -1) {
            mem_lock_unlock(&pool->_mtx);
            return NULL;
        }
    }

    list = ems_queue_head(&pool->entry_freelist);
    ems_queue_remove(list);
    mem_lock_unlock(&pool->_mtx);

    pn = ems_container_of(list, pool_node_t, entry);

    pn->_size = size;
    pn->_size_real = pool->_node_size;

    atomic_inc(&pool->_stat_alloc);
    atomic_inc(&pool->_total_used_nodes);

    return pn;
}

static int pool_free(mem_pool_t *pool, pool_node_t *ptr)
{
    assert(pool && ptr);

    mem_lock_lock(&pool->_mtx);
    ems_queue_insert_tail(&pool->entry_freelist, &ptr->entry);
    mem_lock_unlock(&pool->_mtx);

    ptr->_ptr = (uintptr_t)ptr;

    atomic_inc(&pool->_stat_free);
    atomic_dec(&pool->_total_used_nodes);

    return 0;
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
