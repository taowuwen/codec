
#ifndef _EMS_QUEUE_H_INCLUDED_
#define _EMS_QUEUE_H_INCLUDED_

#include <stddef.h>


typedef struct ems_queue_s  ems_queue_t, ems_queue;

struct ems_queue_s {
    ems_queue_t  *prev;
    ems_queue_t  *next;
};


#define ems_queue_init(q)                                                     \
    (q)->prev = q;                                                            \
    (q)->next = q


#define ems_queue_empty(h)                                                    \
    (h == (h)->prev)


#define ems_queue_insert_head(h, x)                                           \
    (x)->next = (h)->next;                                                    \
    (x)->next->prev = x;                                                      \
    (x)->prev = h;                                                            \
    (h)->next = x


#define ems_queue_insert_after   ems_queue_insert_head


#define ems_queue_insert_tail(h, x)                                           \
    (x)->prev = (h)->prev;                                                    \
    (x)->prev->next = x;                                                      \
    (x)->next = h;                                                            \
    (h)->prev = x


#define ems_queue_head(h)                                                     \
    (h)->next


#define ems_queue_last(h)                                                     \
    (h)->prev


#define ems_queue_sentinel(h)                                                 \
    (h)


#define ems_queue_next(q)                                                     \
    (q)->next


#define ems_queue_prev(q)                                                     \
    (q)->prev


#if (DEBUG)

#define ems_queue_remove(x)                                                   \
    (x)->next->prev = (x)->prev;                                              \
    (x)->prev->next = (x)->next;                                              \
    (x)->prev = NULL;                                                         \
    (x)->next = NULL

#else

#define ems_queue_remove(x)                                                   \
    (x)->next->prev = (x)->prev;                                              \
    (x)->prev->next = (x)->next

#endif


#define ems_queue_split(h, q, n)                                              \
    (n)->prev = (h)->prev;                                                    \
    (n)->prev->next = n;                                                      \
    (n)->next = q;                                                            \
    (h)->prev = (q)->prev;                                                    \
    (h)->prev->next = h;                                                      \
    (q)->prev = n;


#define ems_queue_add(h, n)                                                   \
    (h)->prev->next = (n)->next;                                              \
    (n)->next->prev = (h)->prev;                                              \
    (h)->prev = (n)->prev;                                                    \
    (h)->prev->next = h;


#define ems_queue_data(q, type, link)                                         \
    (type *) ((unsigned char *) q - offsetof(type, link))

#define ems_queue_len(list, len) do{						\
	ems_queue *p; 								\
	len = 0;								\
	ems_queue_foreach(list, p) { len++; }					\
} while (0)


#define ems_container_of	 ems_queue_data

#define ems_queue_clear(queue, type, link, destroy) do {	\
	ems_queue *n;					\
	while (!ems_queue_empty(queue)) {		\
		n = ems_queue_head(queue);		\
		ems_queue_remove(n);			\
		destroy(ems_queue_data(n, type, link));	\
	}						\
}while(0)


#define ems_queue_clean(queue) do {		\
	ems_queue *n;				\
	while (!ems_queue_empty(queue)) { 	\
		n = ems_queue_head(queue);	\
		ems_queue_remove(n);		\
	}					\
} while (0)


/*
 * well, you gonna not delete a node
 * */
#define ems_queue_foreach(q, n) 			\
	for (n  = ems_queue_head(q); 		\
	     n != ems_queue_sentinel(q); 	\
	     n  = ems_queue_next(n))

#define ems_queue_foreach_safe(q, cur, next)	\
	for (cur = ems_queue_head(q), next = ems_queue_next(cur);  \
	     cur != ems_queue_sentinel(q);			 \
	     cur = next, next = ems_queue_next(cur))


#endif
