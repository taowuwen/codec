
#ifndef TEST_FWD_CM_HEADER___
#define TEST_FWD_CM_HEADER___

#define	T_OK		FWD_OK
#define T_ERR		FWD_ERR

typedef fwd_sock  	t_sock;
typedef fwd_transinfo	t_trans;
typedef fwd_buffer	t_buffer;
typedef fwd_event	t_event;
typedef fwd_timeout	t_timeout;
typedef fwd_event_fd	t_event_fd;

typedef struct _t_session_s	t_session;
typedef struct _t_user_s	t_user;
typedef struct _t_file_s	t_file;
typedef struct _t_worker_s 	t_worker;


typedef enum {
	ST_SESSION_MIN		= -1,
	ST_SESSION_INIT		= 0,
	ST_SESSION_NORMAL	= 1,
	ST_SESSION_ERROR	= 2,
	ST_SESSION_MAX
} t_session_status;


typedef enum {
	ST__CM_MIN	= -1,
	ST__CM_INIT	= 0,
	ST__CM_REGISTER	= 1,
	ST__CM_HB	= 2,  /* peer only */
	ST__CM_SENDHB	= 3,  /* peer only */
	ST__CM_REQUEST_FWD = 4, /* self only */

	ST__CM_MAX
} t_cm_status;

typedef enum {
	ST_FWD_MIN	= -1,
	ST_FWD_INIT	= 0,
	ST_FWD_CONN	= 1,
	ST_FWD_REG	= 2,
	ST_FWD_EST	= 3,
	ST_FWD_HB	= 4,
	ST_FWD_SENDFILE = 5,
	ST_FWD_REREG	= 6,
	ST_FWD_MAX
} t_fwd_status;

struct _t_file_s {
	off_t	sz_fl;
	off_t	left;
	dt_str  marks;
};

struct _t_user_s {
	t_session	*sess;
	t_user		*peer;

	t_sock	 	 cm;
	t_timeout	 cm_to;
	t_event_fd	 cm_evt;
	t_buffer	 cm_buf;
	t_cm_status	 cm_st;
	dt_int		 n_retry;

	dt_str		 ticket;

	t_sock   	 fwd;
	t_timeout	 fwd_to;
	t_event_fd	 fwd_evt;
	t_buffer	 fwd_buf;
	t_fwd_status	 fwd_st;
	t_trans		 fwd_trans;

	dt_int 		 fwd_retry;
	off_t		 fwd_buf_len;
	dt_str		 fwd_arg;

	t_buffer	 fwd_out; /* for output */
	#define fwd_in	fwd_buf

	/* for testing file sending*/
	dt_int		 n_hb; /* heartbeat times */
	dt_int		 flgs;

	t_file		 fl;
};

struct _t_session_s {
	t_user		*user1;
	t_user		*user2;

	t_session_status st;

	t_trans		 trans;
	dt_str		 uuid;
	dt_queue	 list;

	t_worker	*base;
};


#define t_lasterrstr	fwd_lasterrstr
#define t_lasterr	fwd_lasterr

struct _t_worker_s {
	dt_int		st;
	dt_int		run;

	dt_int		n_sess;
	dt_queue	sess;

	dt_int		left;

	dt_threadid	tid;
	t_event	   	event;

	dt_int		spawn;
};



#define T_TIMEOUT_CONNECT_CM	5000 // 10 seconds
#define T_TIMEOUT_CM_HB		3000 // 60 seconds
#define T_TIMEOUT_CM_SEND_HB	5000 // 10 seconds
#define T_TIMEOUT_CONNECT_FWD	5000
#define T_TIMEOUT_GETTICKET	5000 // 8 seconds
#define T_TIMEOUT_DEFAULT	5000 // 10 seconds for default
#define T_TIMEOUT_FWD_SENDMSG	5000 // 5 seconds
#define T_TIMEOUT_FWD_HB	3000 // 3 seconds



/* flgs for control testing */

#define T_INFINITE		-1

#define T_FLG_ALL_CONNECT_THE_SAME_TIME		(1 << 0)
#define T_FLG_SEND_FILE				(1 << 1)
#define T_FLG_RECV_ALL				(1 << 2)



#endif
