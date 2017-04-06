
#ifdef DT_USE_SELECT

#include "uhttpd.h"
#include "uhttpd-utils.h"

#ifdef HAVE_TLS
#include "uhttpd-tls.h"
#endif

#include "dt_uhttpd_utils.h"
#include "dt_uhttpd_select.h"
#include "libubox/list.h"
#include <pthread.h>

typedef struct list_head dt_list;

typedef struct {
	dt_list queue;
	struct  client *cl;
} dt_client;


struct dt_uhttpd{
	dt_list    sock_queue;
	int 	   run;
	int        inited;
	pthread_t  tid;
	pthread_mutex_t mtx;
};


static struct dt_uhttpd _gdthttp = {
	.inited = 0,
	.run    = NO,
	.tid    = 0 
};

static void http_lock(struct dt_uhttpd *http)
{
	if (http)
		pthread_mutex_lock(&http->mtx);

	return;
}

static void http_unlock(struct dt_uhttpd *http)
{
	if (http)
		pthread_mutex_unlock(&http->mtx);

	return;
}

static dt_list *http_list(struct dt_uhttpd *http)
{
	return &http->sock_queue;
}

static int http_run(struct dt_uhttpd *http)
{
	return http->run;
}

static int http_setrun(struct dt_uhttpd *http, int run)
{
	return http->run = run;
}


static dt_client *dt_new_client(struct client *cl, int fd)
{
	dt_client *client = NULL;

	client = (dt_client *)malloc(sizeof(dt_client));
	if (client) {
		INIT_LIST_HEAD(&client->queue);
		cl->file = fd;
		client->cl = cl;
		fd_nonblock(cl->fd.fd);
	}

	return client;
}

static void dt_destroy_clent(dt_client *client)
{
	struct client *cl;
	if (client && client->cl) {
		if ( client->cl) {
			cl = client->cl;

			if (cl->file > 0)
				close(cl->file);

			uh_client_destroy(cl);
		}

		free(client);
	}
}

static int dt_build_select_list(struct dt_uhttpd *http, fd_set *wr)
{
	dt_list     *node;
	dt_client   *client;
	int high;

	high = 0;
	FD_ZERO(wr);

	http_lock(http);
	list_for_each(node, http_list(http)) {
		client = list_entry(node, dt_client, queue);
		FD_SET(client->cl->fd.fd, wr);
		if (client->cl->fd.fd > high)
			high = client->cl->fd.fd;
	}
	http_unlock(http);

	return high;
}

static int dt_sendtoclient(dt_client *client)
{
	return dt_uhttpd_write(client->cl);
}

static int 
dt_handle_send(struct dt_uhttpd *http, fd_set *wr)
{
	dt_list     *node, *next;
	dt_client   *client;


	http_lock(http);
	list_for_each_safe(node, next, http_list(http)) {
		client = list_entry(node, dt_client, queue);

		if (FD_ISSET(client->cl->fd.fd, wr)) {
			if (dt_sendtoclient(client) != OK) {

				list_del(node);
				dt_destroy_clent(client);
			}
		}

	}

	http_unlock(http);

	return OK;
}


static int dt_send_files(struct dt_uhttpd *http)
{
	struct timeval timeout;
	fd_set set_wr;
	int rtn;
	int high;


	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	high = dt_build_select_list(http, &set_wr);

	rtn = select(high+1, NULL, &set_wr, NULL, &timeout);
	
	if (rtn < 0) {
		D("select error: %s\n", strerror(errno));
		return ERR;
	}

	if (rtn > 0)
		return dt_handle_send(http, &set_wr);

	return OK;
}


static void *dt_uhttpd_main(void *arg)
{
	struct dt_uhttpd  *http = (struct dt_uhttpd *)arg;

	D("thread dt select started: %d(%x)\n", http->tid, http->tid);

	while (http_run(http)) {

		dt_send_files(http);

		http_lock(http);
		if (list_empty(http_list(http))) 
			http_setrun(http, NO);
		http_unlock(http);
	}

	D("thread dt select stopped: %d(%x)\n", http->tid, http->tid);

	return NULL;
}

int dt_uhttpd_send_content(struct client *cl, int fd)
{
	int rtn;
	dt_client *client = NULL;

	if (!_gdthttp.inited) {
		D("call dt_uhttpd_init first\n");
		return ERR;
	}

	if (!(cl && (fd >0)))
		return ERR;

	client = dt_new_client(cl, fd);
	if (!client)
		return ERR;

	rtn = OK;

	http_lock(&_gdthttp);

	list_add_tail(&client->queue, http_list(&_gdthttp));

	if (!http_run(&_gdthttp)) {
		if (_gdthttp.tid > 0) {
			pthread_join(_gdthttp.tid, NULL);
			_gdthttp.tid = 0;
		}

		http_setrun(&_gdthttp, YES);
		if (pthread_create(&_gdthttp.tid, NULL, dt_uhttpd_main, 
							(void *)&_gdthttp)) {
			D("pthread_create failed: %s\n", strerror(errno));
			http_setrun(&_gdthttp, NO);
			rtn = ERR;
		}
	}

	http_unlock(&_gdthttp);

	return rtn;
}


int dt_uhttpd_init()
{
	if (_gdthttp.inited)
		return YES;

	memset(&_gdthttp, 0 , sizeof(_gdthttp));

	INIT_LIST_HEAD(&_gdthttp.sock_queue);
	_gdthttp.inited = YES;
	_gdthttp.run = NO;
	_gdthttp.tid = 0;
	pthread_mutex_init(&_gdthttp.mtx, NULL);

	return OK;
}

int dt_uhttpd_uninit()
{
	if (_gdthttp.inited) {
		_gdthttp.run = NO;

		if ( _gdthttp.tid > 0) {
			pthread_join(_gdthttp.tid, NULL);
			_gdthttp.tid = 0;
		}

		pthread_mutex_destroy(&_gdthttp.mtx);
		_gdthttp.inited = NO;
	}

	return OK;
}
#endif
