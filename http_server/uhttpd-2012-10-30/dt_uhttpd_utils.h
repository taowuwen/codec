
#ifndef DT_UTILS_HEADER___
#define DT_UTILS_HEADER___

#define NO	0
#define YES	1
#define OK	0
#define ERR	-1

int dt_fillbuffer(struct client *cl);
int dt_http_send(struct client *cl, const char *buf, int len);
int dt_uhttpd_write(struct client *cl);


#endif
