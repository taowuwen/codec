#ifndef EMS_SPLITOR_HEADER___H___
#define EMS_SPLITOR_HEADER___H___

typedef struct _ems_split_s ems_split;

struct _ems_split_s {
	ems_queue entry;
	ems_str   str;
};


ems_split *ems_split_new();
ems_void   ems_split_destroy(ems_split *sp);
ems_int    ems_string_split(ems_queue *list, ems_cchar *src, ems_cchar *sep);
ems_void   ems_split_clear(ems_queue *list);

ems_split *ems_split_find(ems_queue *list, ems_cchar *key);
ems_cchar *ems_split_get_str(ems_queue *list, ems_int ind);
ems_int    ems_split_len(ems_queue *list);

typedef int (*psplit_cb)(ems_void *arg, ems_cchar *item);
ems_int    ems_split_foreach(ems_queue *list,  psplit_cb cb, ems_void *arg);


#endif
