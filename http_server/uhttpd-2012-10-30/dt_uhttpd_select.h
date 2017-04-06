
#ifndef DT_UHTTPD_SELECT_EXTEND_HEADER
#define DT_UHTTPD_SELECT_EXTEND_HEADER

#ifdef DT_USE_SELECT

int dt_uhttpd_init();

int dt_uhttpd_send_content(struct client *cl, int fd);

int dt_uhttpd_uninit();

#endif


#endif
