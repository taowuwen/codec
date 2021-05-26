#include "mem.h"
#include <sys/sysinfo.h>

typedef struct {
    uint32_t n_size;
    mem_mgmt_t *mgmt;
} mem_sched_t;

static mem_sched_t *g_sched;

static inline uint32_t getenv_uint32(const char *key, uint32_t def)
{
    char *val = NULL;
//  val = secure_getenv(key);
    val = getenv(key);
    return val ? (uint32_t)atoi(val) : def;
}

static int mem_mgmt_init(mem_mgmt_t *mgmt)
{
    int i = 0;

    memset(mgmt, 0, sizeof(mem_mgmt_t));
    mgmt->enable = 1;
    mgmt->enable_backtrace = 0;

    mgmt->n_pool = 0;
    mgmt->pool = (mem_pool_t *)malloc(sizeof(mem_pool_t) * mgmt->n_pool);
    if (!mgmt->pool) {
        return -1;
    }

    mgmt->n_cls = 0;
    mgmt->cls = (mem_class_t *)malloc(sizeof(mem_class_t) * mgmt->n_cls);

    if (!mgmt->cls) {
        free(mgmt->pool);
        mgmt->pool = NULL;
        return -1;
    }

    for (i = 0; i < mgmt->n_pool; i++) {
        ;
    }

    for (i = 0; i < mgmt->n_cls; i++) {
        ;
    }

    return 0;
}

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
    uint32_t i = 0;
    if (!g_sched) {
        return 0;
    }

    g_sched = (mem_sched_t *)malloc(sizeof(mem_sched_t));
    if (!g_sched) {
        return -1;
    }

    g_sched->n_size = getenv_uint32("MEMPOOL_SCHED_SIZE", get_nprocs());
    if (g_sched->n_size <= 0 || g_sched->n_size >= 1000) {
        g_sched->n_size = get_nprocs();
    }

    g_sched->mgmt = (mem_mgmt_t *) malloc(sizeof(mem_mgmt_t) * g_sched->n_size);
    if (!g_sched->mgmt) {
        free(g_sched);
        g_sched = NULL;
        return -1;
    }

    for (i = 0; i < g_sched->n_size; i++) {
        mem_mgmt_init(g_sched->mgmt + i);
    }

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
