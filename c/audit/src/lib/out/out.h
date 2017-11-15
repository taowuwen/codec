
#ifndef MODULES_OUT_HEADER___H
#define MODULES_OUT_HEADER___H

typedef struct _out_plugins out_plugin;
typedef struct _out_core_s  out_core;

typedef enum _out_id_s {
	id_out = 1,
	id_out_file=50,
	id_out_16wifi=51
} audit_out_id;

struct _out_plugins {
	ems_int    id;
	ems_int    mount; /* for post process's mount */
	ems_str    desc;
	ems_void  *ctx;

	ems_int   (*init)(out_plugin *);
	ems_int   (*process)(out_plugin *, ems_uint msgid, ems_void *);
	ems_int   (*uninit)(out_plugin *);

	ems_queue  entry_post; /* for postprocess goes from here */
	ems_queue  entry;
};

struct _out_core_s {
	ems_buffer buf; /**/
	//ems_timeout  to; /* for flush buffer triger */

//	out_plugin *plg;
	ems_queue  flt; /* for output plugins */
};

ems_int out_plug_sendmsg(out_plugin *plg, ems_uint evt, ems_void *arg);
ems_int out_plug_broadcast(ems_queue *list, ems_uint evt, ems_void *arg);
ems_int out_unload_plugins(ems_queue *list);
ems_int out_load_plugins(ems_queue *list, ems_int mount);
ems_int out_sendmsg(ems_int s, ems_int d, ems_uint evt, ems_void *arg);

ems_queue *out_filters();

#endif
