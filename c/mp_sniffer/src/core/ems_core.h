
#ifndef EMS_CLIENT_CORE_HEADER_____
#define EMS_CLIENT_CORE_HEADER_____

#include "ems.h"
#include "ems_main.h"
#include "ems_json.h"
#include <pcap.h>

#define SNIFFER_WORKING_DIR	"/tmp/sniffer"
#define SNIFFER_MAX_FILESIZE	10485760

#define FLG_AUTO_REG		0x00000001
#define FLG_RUN			0x00000002
#define FLG_INITED		0x00000004
#define FLG_ONLINE		0x00000008
#define FLG_MONITOR_MODE	0x00000010
#define FLG_UPLOADING		0x00000020
#define FLG_PAUSE		0x00000040


typedef struct _ems_core_s    		ems_core;
typedef enum   _ems_status_s  		ems_status;
typedef struct _ems_ctrl_s    		ems_ctrl;
typedef struct _ems_capture_s		ems_capture;
typedef struct _ems_do_capture_s	ems_do_capture;
typedef struct _ems_server_s 		ems_server;
typedef struct _ems_file_s		ems_sniffer_file;
typedef struct _ems_output_s		ems_output;
typedef struct _ems_channel_s 		ems_channel;


enum _ems_status_s 
{
	st_min  = -1,
	st_init =  0,
#define st_start	st_init
	st_stopped = 1,
	st_normal = 2,
	st_reg = 3,
	st_err = 4,
	st_connect = 5,
	st_paused = 6,
#define st_running	st_normal /* capture module only */

#define st_capturing	st_normal /* capture interface only*/
#define st_uploading	st_reg    /* capture interface only */

	st_continue = 7,
#define st_process	st_normal /* output module only */
#define st_notify	st_reg	  /* output module only */
	
	st_max
};


struct _ems_ctrl_s
{
	ems_session   *sess;

	/* 
	   start --> connect ---> reg --> normal 
	    |         |	           |	    |
            |         |	           |	    |
            |         |	           |	    |
            V	      V	           |	    |
	   stop<-----err <---------+--------+
	 */
	ems_status     st;
	ems_core      *_core;
};

#if 0
struct _ems_filter_s {
	ems_do_capture *inf;  /* attached inf     */
	ems_do_capture *work; /* worked interface */

	ems_str   inf;
	ems_str   filer;
	ems_queue  list;
};
#endif


struct _ems_channel_s {
	ems_queue   entry;
	ems_int     channel;
};

struct _ems_server_s {
	ems_int type; /* type == 1, ftp, currently for ftp only */
	ems_int compress;
	ems_str host;

	union {
		struct {
			/* ftp://name:pass@host:port/path */
			ems_str url;
		} ftp;
	};
};

struct _ems_file_s {
	ems_str  name;
	ems_str  path;
	off_t filesize;
	ems_uint flg;

	ems_queue  output_entry;
	ems_queue  entry;

	ems_do_capture *inf;
};

struct _ems_do_capture_s {
	ems_queue   entry;
	ems_status  st;
	ems_event_fd  evt; /* for event handler */
	ems_timeout   to;  /* for timeout handler */
	struct tm     tm_start; /* start time */
	ems_uint      flg;

	ems_int     lasterr;
	ems_char    errmsg[512];

	struct {
		pcap_t            *pfd;
		ems_int            fd;
		struct bpf_program fp;
		bpf_u_int32 	   netmask;
		bpf_u_int32 	   network;
#if 0
		struct ems_filter  filter;
		struct ems_filter  excepted_filter;
#else
		ems_str		   filter;
		ems_str            excepted_filter;
#endif
	} pcap;

	struct {
		ems_int       id; /* current file writting id*/
	        pcap_dumper_t *fp; /* for pcap file open */
		ems_str       name;     /* file name */
		ems_str       rootpath; /* file root path*/
		ems_queue     list; /* for captured and uploading not finished */

		off_t      pkgs;      /* current file received pkgs */
		off_t      timeuse;   /* current timeuse */
		off_t      size;      /* current filesize */
	} file;

	off_t  max_filesize;
	off_t  limit_pkg, limit_timeuse, limit_filesize;

	ems_int  radioid, wlanid, type;
	ems_str  inf;

	ems_server   server;
	ems_capture *capture;

	ems_timeout  channel_timeout; /* for update channel */
	ems_int    channel_internal; /* for radio capture channel change: 2s defaults */
	ems_queue  channel_lists;
	ems_queue *cur_channel;
};

struct _ems_capture_s {
	ems_queue   list_capture_inf;

	ems_status  st; /* capture status */

	ems_queue   list_filters;

	off_t mem_total; /* total memory could be use */
	off_t mem_left;  /* total memory for currently left */
	off_t mem_rsrv;  /* total memory rsrv on system */
	off_t mem_maxsingle; /* max single file size*/

	ems_int     max_capture_number; /* max number for currently captures */
	ems_int     n_capture;   /* currently capture number */
	ems_int     type;

	ems_str     mac; /* ap mac address */

	ems_core   *_core;
};

struct _ems_output_s {
	ems_queue         list_files;

	ems_uint          lasterr;
	ems_timeout       to;  /* timeout check process stop*/
	pid_t             pid; /* for uploading process id */
	ems_status        st;  /* output status */
	ems_sniffer_file *curfl; /* current file */

	ems_core   *_core;
};

struct _ems_core_s
{
	ems_ctrl     *ctrl;
	ems_capture  *capture;
	ems_output   *output;
	ems_event     evt;
	ems_uint      flg;
	ems_buffer    buf; // for buffer control, for speed up
};

ems_int ems_core_init(ems_core *core);
ems_int ems_core_uninit(ems_core *core);
ems_int ems_core_main(ems_core *core, ems_int argc, ems_char **argv);

ems_int core_pack_rsp(ems_session *sess, ems_uint tag, ems_int st);
ems_int core_pack_req(ems_session *sess, ems_uint tag);
ems_core  *emscorer();

ems_cchar *ems_strcat(ems_cchar *s1, ems_cchar *s2);
ems_cchar *ems_popen_get(ems_cchar *fmt, ...);
ems_int    ems_systemcmd(ems_cchar *fmt, ...);

ems_buffer *core_buffer();

ems_int ctrl_change_status(ems_ctrl *ctrl,      ems_status st);
ems_int capture_change_status(ems_capture *cap, ems_status st);
ems_int output_change_status(ems_output *out,   ems_status st);


ems_cchar *ems_status_str(ems_status st);



/*
   api for capture module
 */
ems_int capture_handle_msg_start(ems_capture *capture, json_object *req);
ems_int capture_handle_msg_stop(ems_capture *capture, json_object *req);
ems_int capture_handle_msg_stopall(ems_capture *capture, json_object *req);
ems_int capture_cap_exit(ems_capture *capture, ems_do_capture *cap);
ems_int capture_continue_capturing(ems_capture *capture);

/* 
   api for do capture module
 */
ems_do_capture *ems_do_capture_new();
ems_void ems_do_capture_destroy(ems_do_capture *cap);
ems_int cap_change_status(ems_do_capture *cap, ems_status st);
ems_int cap_file_upload_finished(ems_do_capture *, ems_sniffer_file *, ems_int);

/*
   api for ctrl module
 */
ems_int ctrl_send_msg(ems_ctrl *ctrl, ems_uint tag, json_object *req);

#define ctrl_send_event_to_apcfg(ctrl, req)	ctrl_send_msg(ctrl, MSG_EVT_SEND_TO_APCFG, req)
#define ctrl_send_event_to_wtp(ctrl, req)	ctrl_send_msg(ctrl, MSG_EVT_SEND_TO_WTP, req)


json_object *core_build_wtp_event(
		ems_int evt, /* evt =[1-3]*/ 
		ems_int ctype,
		ems_int wlanid,
		ems_int    radioid,
		ems_cchar *interface,
		ems_int    errcode,
		ems_cchar *errmsg);


json_object *core_build_apcfg_event(
		ems_int    evt,
		ems_cchar *args,
		ems_int    errcode,
		ems_cchar *errmsg);

/*
   api for output module
 */
ems_int output_upload_file(ems_output *out, ems_sniffer_file *fl);

ems_channel *ems_channel_new();
ems_void ems_channel_destroy(ems_channel *ch);
ems_do_capture *capture_get_cap(ems_capture *capture, ems_cchar *inf);
ems_int cap_omit_channel_update(ems_do_capture *cap, ems_int channel);

#endif
