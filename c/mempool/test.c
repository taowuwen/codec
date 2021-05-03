#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "mem.h"

typedef struct {
    size_t n_times;
    size_t n_malloc;
    size_t n_free;

    uint64_t tm_total;
    uint64_t tm_malloc;
    uint64_t tm_free;
} mem_request_t;

uint64_t time_diff(struct timeval *tm1, struct timeval *tm2)
{
    return (uint64_t)((tm1->tv_sec - tm2->tv_sec) * 1000000 + tm1->tv_usec - tm2->tv_usec);
}

void *thread_main(void *arg)
{
    struct timeval tm_start;
    struct timeval tm_end;
    struct timeval tm_start_total;
    struct timeval tm_end_total;
    int i, j;
    uint64_t *ptrs = NULL;

    mem_request_t *req = (mem_request_t *) arg;

    ptrs = (uint64_t *)mem_malloc(sizeof(uint64_t) * req->n_malloc);
    if (!ptrs)
        goto err_out;

    gettimeofday(&tm_start_total, NULL);

    for (i = 0; i < req->n_times; i++) {
        gettimeofday(&tm_start, NULL);
        for (j = 0; j < req->n_malloc; j++) {
            ptrs[j] = (uint64_t)mem_malloc(random() % 4096);
        }
        gettimeofday(&tm_end, NULL);
        req->tm_malloc += time_diff(&tm_end, &tm_start);

        gettimeofday(&tm_start, NULL);
        for (j = 0; j < req->n_malloc; j++) {
            if (ptrs[j] != 0) {
                mem_free((void *)ptrs[j]);
                ptrs[j] = 0;
            }
        }
        gettimeofday(&tm_end, NULL);
        req->tm_free += time_diff(&tm_end, &tm_start);
    }

    gettimeofday(&tm_end_total, NULL);

    req->tm_total = time_diff(&tm_end_total, &tm_start_total);

err_out:
    printf("Thread %lu finished: (%lu, %lu, %lu)\n", pthread_self(), req->tm_total, req->tm_malloc, req->tm_free);

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

    {
        uint64_t time_total = 0;
        uint64_t time_total_alloc = 0;
        uint64_t time_total_free = 0;

        for(i = 0; i < n_threads; i++) {
            r = req + i;
            time_total += r->tm_total;
            time_total_alloc += r->tm_malloc;
            time_total_free  += r->tm_free;
        }

        printf("summery information -> total: %lu, total alloc: %lu, total free: %lu\n",
                time_total, time_total_alloc, time_total_free);
        printf("summery information -> AVG: total: %lf, total alloc: %lf, total free: %lf\n",
                (double)time_total/r->n_times / 1000000,
                (double)time_total_alloc/r->n_times / 1000000,
                (double)time_total_free/r->n_times / 1000000);

        printf("summery information -> AVG ALL: total: %lf, total alloc: %lf, total free: %lf\n",
                (double)time_total/(r->n_times * n_threads) / 1000000,
                (double)time_total_alloc/(r->n_times * n_threads) / 1000000,
                (double)time_total_free/(r->n_times * n_threads) / 1000000);
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

    srand(time(NULL));

    mem_init();

    printf("memory malloc and free testing: threads: %ld times: %ld malloc: %ld free: %ld\n",
            n_threads, n_times, n_malloc, n_free);
    do_testing(n_threads, n_times, n_malloc, n_free);

    mem_leak_check();
    mem_uninit();

    return 0;
}
