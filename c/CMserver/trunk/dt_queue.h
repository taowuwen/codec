
#ifndef _DT_QUEUE_H_INCLUDED_
#define _DT_QUEUE_H_INCLUDED_

#include <stddef.h>


typedef struct dt_queue_s  dt_queue_t, dt_list, dt_queue;

struct dt_queue_s {
    dt_queue_t  *prev;
    dt_queue_t  *next;
};


#define dt_queue_init(q)                                                     \
    (q)->prev = q;                                                            \
    (q)->next = q


#define dt_queue_empty(h)                                                    \
    (h == (h)->prev)


#define dt_queue_insert_head(h, x)                                           \
    (x)->next = (h)->next;                                                    \
    (x)->next->prev = x;                                                      \
    (x)->prev = h;                                                            \
    (h)->next = x


#define dt_queue_insert_after   dt_queue_insert_head


#define dt_queue_insert_tail(h, x)                                           \
    (x)->prev = (h)->prev;                                                    \
    (x)->prev->next = x;                                                      \
    (x)->next = h;                                                            \
    (h)->prev = x


#define dt_queue_head(h)                                                     \
    (h)->next


#define dt_queue_last(h)                                                     \
    (h)->prev


#define dt_queue_sentinel(h)                                                 \
    (h)


#define dt_queue_next(q)                                                     \
    (q)->next


#define dt_queue_prev(q)                                                     \
    (q)->prev


#if (DEBUG)

#define dt_queue_remove(x)                                                   \
    (x)->next->prev = (x)->prev;                                              \
    (x)->prev->next = (x)->next;                                              \
    (x)->prev = NULL;                                                         \
    (x)->next = NULL

#else

#define dt_queue_remove(x)                                                   \
    (x)->next->prev = (x)->prev;                                              \
    (x)->prev->next = (x)->next

#endif


#define dt_queue_split(h, q, n)                                              \
    (n)->prev = (h)->prev;                                                    \
    (n)->prev->next = n;                                                      \
    (n)->next = q;                                                            \
    (h)->prev = (q)->prev;                                                    \
    (h)->prev->next = h;                                                      \
    (q)->prev = n;


#define dt_queue_add(h, n)                                                   \
    (h)->prev->next = (n)->next;                                              \
    (n)->next->prev = (h)->prev;                                              \
    (h)->prev = (n)->prev;                                                    \
    (h)->prev->next = h;


#define dt_queue_data(q, type, link)                                         \
    (type *) ((unsigned char *) q - offsetof(type, link))


#define dt_container_of	 dt_queue_data

#define dt_queue_clear(queue, type, link, destroy) do {	\
	dt_queue *n;					\
	while (!dt_queue_empty(queue)) {		\
		n = dt_queue_head(queue);		\
		dt_queue_remove(n);			\
		destroy(dt_queue_data(n, type, link));	\
	}						\
}while(0); 


#define dt_queue_clean(queue) do {		\
	dt_queue *n;				\
	while (!dt_queue_empty(queue)) { 	\
		n = dt_queue_head(queue);	\
		dt_queue_remove(n);		\
	}					\
} while (0);


/*
 * well, you gonna not delete a node
 * */
#define dt_queue_foreach(q, n) 			\
	for (n  = dt_queue_head(q); 		\
	     n != dt_queue_sentinel(q); 	\
	     n  = dt_queue_next(n))

#define dt_queue_foreach_safe(q, cur, next)	\
	for (cur = dt_queue_head(q), next = dt_queue_next(cur);  \
	     cur != dt_queue_sentinel(q);			 \
	     cur = next, next = dt_queue_next(cur))


#endif
