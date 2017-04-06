
#include "fwd_main.h"
#include "fwd_bind.h"
#include "fwd_sock.h"
#include "fwd_user.h"

dt_int fwd_bind_init(fwd_bind *fb)
{
	memset(fb, 0, sizeof(fwd_bind));
	fwd_sock_init(&fb->sock);
	fwd_sock_setaddr(&fb->sock, fwd_public_addr());

	if (!fwd_sock_addr(&fb->sock)) {
		log_warn("[bind]can not get public address");
#ifdef DEBUG
		log_info("[bind]reset it into 0.0.0.0 for debug version");
		fwd_sock_setaddr(&fb->sock, "0.0.0.0");
#else
		log_info("[bind]reset it into 0.0.0.0 for tmp version");
		fwd_sock_setaddr(&fb->sock, "0.0.0.0");

		//fwd_bind_uninit(fb);
	//	return FWD_ERR;
#endif
	}

	return FWD_OK;
}

dt_int fwd_bind_uninit(fwd_bind *fb)
{
	fwd_bind_stop(fb);

	fwd_sock_clear(&fb->sock);
	memset(fb, 0, sizeof(fwd_bind));

	return FWD_OK;
}

static dt_int
set_server_sock_opt(dt_int fd)
{
	int on;
	struct linger so_linger;

	on = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (dt_cchar *)&on, sizeof(on));

	so_linger.l_onoff  = 0;
	so_linger.l_linger = 30;
	setsockopt(fd, SOL_SOCKET, SO_LINGER, (dt_cchar *)&so_linger, sizeof(so_linger));

	return FWD_OK;
}

dt_int fwd_bind_start(fwd_bind *fb)
{
	dt_int  ret;
	dt_int  fd;
	dt_uint port;
	struct sockaddr_in addr;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd <= 0)
		return FWD_ERR;

	set_server_sock_opt(fd);

	memset(&addr, 0, sizeof(addr));

	port = FWD_DEFAULT_PORT;
	addr.sin_family = AF_INET;

	dt_assert(fwd_sock_addr(&fb->sock) != NULL);
	addr.sin_addr.s_addr = inet_addr(fwd_sock_addr(&fb->sock));

bind_again:
	addr.sin_port = htons((unsigned short)port);

	ret = bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr));

	if (ret == -1) {
		ret = fwd_lasterr();
		switch(ret) {

		case EADDRINUSE:
			port++;
			goto bind_again;
		default:
			log_err("[bind]bind error:%d, %s", ret, fwd_getlasterrstr(ret));
			break;
		}

		close(fd);
		return FWD_ERR;
	}
	
	if (listen(fd, SOMAXCONN)) {
		if (fwd_lasterr() == EADDRINUSE)
			goto bind_again;

		log_err("[bind]listen error: %s", fwd_lasterrstr());
		close(fd);
		return FWD_ERR;
	}

	fwd_sock_setport(&fb->sock, port);
	fwd_sock_setfd(&fb->sock, fd);

	log_info("[bind] socket: %d bind @address %s:%d ready", 
						fwd_sock_fd(&fb->sock),
						fwd_sock_addr(&fb->sock),
						fwd_sock_port(&fb->sock)); 
	return FWD_OK;
}

dt_int fwd_bind_stop(fwd_bind *fb)
{
	dt_assert(fb != NULL);

	fwd_sock_close(&fb->sock);

	return fwd_main_event_cancel(&fb->event);
}


static dt_int 
fwd_bind_tmpuser_in(dt_int sock, dt_cchar *addr, dt_int port)
{
	fwd_user *user = NULL;

	user = fwd_user_create();
	if (!user)
		return FWD_ERR;

	fwd_sock_setaddr(&user->sock, addr);
	fwd_sock_setport(&user->sock, port);
	fwd_sock_setfd(&user->sock, sock);

	fwd_main_user_in(user);

	return FWD_OK;
}

static dt_int 
fwd_bind_accept_next(dt_int sock)
{
	dt_int    ret;
	socklen_t len;
	struct sockaddr_in addr;

	len = sizeof(addr);
	memset(&addr, 0, sizeof(addr));

	ret = accept(sock, (struct sockaddr *)&addr, &len);

	if (ret < 0) {
		switch(fwd_lasterr()) {
		
		case EINTR:
		case ECONNABORTED:
			return YES;

		case EAGAIN:
			return NO;

		default:
			log_err("[bind]accept error: %s", fwd_lasterrstr());
			return NO;
		}
	}

	log_info("[bind]client(%d) from: %s:%d in", 
				ret, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

	fwd_bind_tmpuser_in(ret, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

	return YES;
}


static dt_void
fwd_bind_accept_cb(fwd_event_fd *evt, dt_int flags)
{
	dt_int next;

	do {
		next = fwd_bind_accept_next(evt->fd);
	} while (next);
}

dt_int fwd_bind_setevent(fwd_bind *fb)
{
	dt_assert((fb != NULL) && "bug");

	fb->event.fd = fwd_sock_fd(&fb->sock);

	return fwd_main_event_set(
			     &fb->event,
			     FWD_EVT_READ | FWD_EVT_EDGE_TRIGGER, 
			     fwd_bind_accept_cb);
}
