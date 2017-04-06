
#ifndef EMS_STATISTIC_HEADER___
#define EMS_STATISTIC_HEADER___

typedef struct _stat_upgrade_s 	  st_upgrade;
typedef struct _stat_device_s 	  st_device;
typedef struct _stat_statistics_  st_statistic;

struct _stat_upgrade_s {

	ems_str        ip;
	ems_str	       ty;
	ems_str        ver;
	ems_str	       status;

	struct timeval create;
	struct timeval access;

	ems_timeout    to;
	ems_uint       flg;
	st_device     *dev;

	ems_queue      entry;
};


struct _stat_device_s {
	ems_str    sn;
	ems_uint   flg;

	st_statistic  *st;
	ems_queue  entry_upg;
	ems_queue  entry;
};

struct _stat_statistics_ {
	ems_queue  online;
	ems_queue  offline;
};


#endif
