
#ifndef FWD_CHILD_PROCESS_MAIN__HEADER__
#define FWD_CHILD_PROCESS_MAIN__HEADER__

#include "fwd.h"
#include "fwd_event.h"
#include "fwd_hash.h"

typedef struct _fwd_session_s   fwd_session;
typedef struct _fwd_user_s      fwd_user;
typedef struct _fwd_transinfo_s fwd_transinfo;
typedef struct _fwd_time_s      fwd_time;
typedef struct _fwd_child_s     fwd_child;
typedef struct _fwd_bind_s      fwd_bind;
typedef struct _fwd_cm_s        fwd_cm;
typedef struct _fwd_buffer_s    fwd_buffer;
typedef struct _fwd_sock_s      fwd_sock;


typedef enum {
        ST_MIN      = -1,
	ST_INITED   = 0,
	ST_AUTH     = 1,
	ST_WATTING  = 2,
	ST_EST      = 3,
	ST_ERROR    = 4,
	ST_CLEAN    = 5,
	ST_ACCEPTED = 6,
	ST_MAX
} fwd_status;

typedef enum {
	ST_CM_MIN  = -1,
	ST_CM_INIT = 0,
	ST_CM_EST  = 1,
	ST_CM_ERR  = 2,
	ST_CM_STOP = 3,
	ST_CM_MAX
} fwd_cm_status;

struct _fwd_sock_s {
	dt_str   addr;
	dt_int   port;
	dt_int   fd;
};

struct _fwd_buffer_s
{
	dt_char        *head;
	dt_char        *tail;
	dt_char        *rd;
	dt_char        *wr;
};

struct _fwd_time_s
{
	struct timeval  access;
	struct timeval  modify;
	struct timeval  born;
};

struct _fwd_transinfo_s
{
	off_t          real_time_speed;
	off_t          total_bytes;
	off_t          total_read;
	off_t          total_write;
	off_t          read_speed;
	off_t          write_speed;
	off_t          average_speed;
	fwd_time       time;   
};


struct _fwd_session_s
{
	fwd_user       *user1;
	fwd_user       *user2;
	fwd_status      st;
	fwd_timeout     timeout; // timeout
	fwd_transinfo   trans;
	fwd_hash_fd     hash;
	dt_str          ticket;
	dt_queue        list;
	fwd_child       *base; // binding on a child, that is which run's him
};

struct _fwd_user_s
{
	fwd_session     *sess;
	fwd_user        *peer;
	fwd_status       st;
	fwd_timeout      timeout;
	fwd_event_fd     evt;
	fwd_buffer       buffer;
	fwd_sock         sock;
	dt_int           reason;
};

struct _fwd_bind_s {
	fwd_event_fd  event;
	fwd_sock      sock;
};

struct _fwd_cm_s {
	fwd_event_fd     event;
	fwd_timeout      timeout;
	fwd_sock         sock;
	fwd_cm_status    st;
	fwd_buffer       buf_out;
	fwd_buffer       buf_in;
};


#define FWD_CHILD_EXITED	0
#define FWD_CHILD_RUN		1
#define FWD_CHILD_STOPPED	2

struct _fwd_child_s {
	dt_int      inited;
	dt_int      n_sess;
	dt_queue    sess; // session;

	dt_int      n_wait;
	dt_queue    wait;    // for waitted handle queue
	dt_mtx      mtx;

	dt_threadid tid;
	dt_int      st;

	fwd_event   evt;
};


#ifndef DEBUG

#define FWD_TIMEOUT_SESSION_INIT	(120000) // 2mins
#define FWD_TIMEOUT_USER_ACCEPTED	(30000)  // 30 secs
#define FWD_TIMEOUT_WAITTING            (15000)  // 15 secs
#define FWD_TIMEOUT_ESTABLISHED		(120000) // 2 mins 
#define FWD_TIMEOUT_SENDMSG		(10000)  // 10 secs, for send out the last msgs
#define FWD_TIMEOUT_RETRY		(60000)  // 1mins, for retry connect, handle...
#define FWD_TIMEOUT_HEARTBEAT		(30000)	 // heart beat with CM
#define FWD_TIMEOUT_AUTH		(30000)	 // 30 secs, for auth
#else

#define FWD_TIMEOUT_SESSION_INIT	(12000) // 12s
#define FWD_TIMEOUT_USER_ACCEPTED	(3000)  // 3s
#define FWD_TIMEOUT_WAITTING            (1500)  // 1.5s 
#define FWD_TIMEOUT_ESTABLISHED		(12000) // 12s 
#define FWD_TIMEOUT_SENDMSG		(3000)   // 1 secs, for send out the last msgs
#define FWD_TIMEOUT_RETRY		(6000)  // 6s , for retry connect, handle...
#define FWD_TIMEOUT_HEARTBEAT		(3000)	 // 3s heart beat with CM
#define FWD_TIMEOUT_AUTH		(3000)	 // 3 secs, for auth

#endif

#define FWD_CM_PORT			6165
#define FWD_DEFAULT_PORT		7788

#define FWD_BUFFER_1K			1024
#define FWD_BUFFER_2K			2048
#define FWD_BUFFER_4k			4096
#define FWD_BUFFER_8k			8192
#define FWD_BUFFER_16k			16384
#define FWD_BUFFER_DEFAULT_SIZE		FWD_BUFFER_4k

#define FWD_HASH_BASE_SIZE		4096

#define FWD_MAX_NOFILE			61000


dt_int fwd_main_user_in(fwd_user *user);
dt_int fwd_main_user_fired(fwd_user *user);
dt_int fwd_main_ticket_in(dt_cchar *ticket);
dt_int fwd_main_session_fired(fwd_session *sess);
dt_int fwd_main_session_down(fwd_session *sess);
fwd_session *fwd_main_session_get(dt_cchar *ticket);
dt_int fwd_main_session_release_ctrl(fwd_session *sess);


dt_int fwd_main_event_set(fwd_event_fd  *fd, 
                          dt_uint        flags,
			  fwd_event_cb   cb);

dt_int fwd_main_event_cancel(fwd_event_fd *fd);


dt_int fwd_main_timeout_set(fwd_timeout    *to,
			    dt_int          msecs,
			    fwd_timeout_cb  cb,
			    dt_uint         pos);

dt_int fwd_main_timeout_cancel(fwd_timeout *to);

dt_cchar *fwd_main_bind_addr();
dt_int    fwd_main_bind_port();


#endif
