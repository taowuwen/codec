#include "mempool.h"


typedef struct {
    ems_queue_t entry;
    uintptr_t   _ptr;
    uint64_t    _size;
} pool_block_t;

static pool_node_t *pool_malloc(mem_pool_t *pool, uint32_t size)
{
    return NULL;
}

static int pool_free(mem_pool_t *pool, pool_node_t *ptr)
{
    return -1;
}

static pool_node_t *system_malloc(mem_pool_t *pool, uint32_t size)
{
    return NULL;
}

static int system_free(mem_pool_t *pool, pool_node_t *ptr)
{
    // assert ptr in pool

    return -1;
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
