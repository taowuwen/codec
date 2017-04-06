

#ifndef DT_UHTTPD_EVENT_HEADER___
#define DT_UHTTPD_EVENT_HEADER___

#ifdef DT_USE_EPOLL
int dt_uhttpd_evt_send(struct client *cl, int fd);
#endif


#endif
