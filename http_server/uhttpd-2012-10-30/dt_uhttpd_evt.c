
#ifdef DT_USE_EPOLL

#include "uhttpd.h"
#include "uhttpd-utils.h"

#ifdef HAVE_TLS
#include "uhttpd-tls.h"
#endif

#include "dt_uhttpd_utils.h"
#include "dt_uhttpd_evt.h"

static int dt_evt_do_read(struct client *cl)
{
	int rtn;
	static char buf[128];

	while (1) {
		rtn = recv(cl->fd.fd, buf, sizeof(buf), 0);

		if (rtn > 0)
			continue;

		if (rtn == 0)
			break;

		switch(errno) {
		case EINTR:
		case EAGAIN:
#ifdef WIN32
		case EWOULDBLOCK:
#endif
			return OK;
		}
		break;
	}

	return ERR;
}

static void 
dt_evt_cb(struct uloop_fd *u, unsigned int events)
{
	struct client *cl = container_of(u, struct client, fd);

	if (events & ULOOP_READ) {
		if ( dt_evt_do_read(cl) != OK) {
			D("Session down: %d With(%d)\n",cl->fd.fd, cl->file);
			if (cl->file > 0)
				close(cl->file);
			uh_client_shutdown(cl);
		}

		return;
	}

	if (dt_uhttpd_write(cl) != OK) {
		D("Session finished: %d with %d\n",cl->fd.fd, cl->file);
		if (cl->file > 0)
			close(cl->file);
		uh_client_shutdown(cl);
	}
}

int dt_uhttpd_evt_send(struct client *cl, int fd)
{
	int evt;
	if (!(cl && fd > 0))
		return ERR;

	cl->file = fd;

	evt = ULOOP_WRITE;
#ifdef HAVE_TLS
	if (!cl->tls)
		evt = evt | ULOOP_READ;
#else
	evt = evt | ULOOP_READ;
#endif
	uh_ufd_add(&cl->fd, dt_evt_cb, evt);

	return OK;
}
#endif
