#include <stdio.h>
#include <stdlib.h>
#include "mem.h"

typedef struct {
    size_t n_times;
    size_t n_malloc;
    size_t n_free;

    uint64_t tm_total;
    uint64_t tm_malloc;
    uint64_t tm_free;
} mem_request_t;

void *thread_main(void *arg)
{
    return NULL;
}

static void do_testing(size_t n_threads, size_t n_times, size_t n_malloc, size_t n_free)
{
    mem_request_t *req = NULL;
    mem_request_t *r = NULL;
    pthread_t     *pthreads = NULL;
    int i = 0;

    if (n_threads >= 1000) {
        n_threads = 1000;
        printf("warning: max threads number = 1000\n");
    }

    req = (mem_request_t *)mem_malloc(sizeof(mem_request_t) * n_threads);
    pthreads = (pthread_t *)mem_malloc(sizeof(pthread_t) * n_threads);

    if (!req || !pthreads)
        goto err_out;


    for (i = 0; i < n_threads; i++) {
        r = req + i;
        memset(r, 0, sizeof(mem_request_t));
        r->n_times = n_times;
        r->n_malloc = n_malloc;
        r->n_free = n_free;

        pthread_create(pthreads + i, NULL, thread_main, (void *)r);
    }

    for (i = 0; i < n_threads; i++) {
        if (pthreads[i] != 0) {
            pthread_join(pthreads[i], NULL);
        }
    }

err_out:
    if (req) {
        mem_free(req);
    }

    if (pthreads) {
        mem_free(pthreads);
    }
}

int main(int argc, char **argv)
{
    long n_threads;
    long n_times;
    long n_malloc;
    long n_free;

    if (argc != 5) {
        printf("usages: %s number_of_threads round_times malloc_times_per_round free_times_per_round\n", argv[0]);
        return 1;
    }

    n_threads = strtol(argv[1], NULL, 10);
    n_times   = strtol(argv[2], NULL, 10);
    n_malloc  = strtol(argv[3], NULL, 10);
    n_free    = strtol(argv[4], NULL, 10);

    if (n_threads <= 0 || n_times <= 0 || n_malloc <= 0 || n_free <= 0) {
        printf("Invalid arguments\n");
        return 1;
    }

    mem_init();

    printf("memory malloc and free testing: threads: %ld times: %ld malloc: %ld free: %ld\n",
            n_threads, n_times, n_malloc, n_free);
    do_testing(n_threads, n_times, n_malloc, n_free);

    mem_leak_check();
    mem_uninit();

    return 0;
}
