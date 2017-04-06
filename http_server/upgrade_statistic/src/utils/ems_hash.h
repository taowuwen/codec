
#ifndef EMS_HASH_TABLES___HEADER____
#define EMS_HASH_TABLES___HEADER____

#define EMS_HASH_BASE_SIZE		4096
#define EMS_HASH_BASE_SMALL		128
#define EMS_HASH_BASE_TINY		64
#define EMS_HASH_BASE_DEFAULT		4096

typedef struct _ems_hash_fd_s  ems_hash_fd;
typedef struct _ems_hash_s     ems_hash;

typedef ems_int (*ems_hash_enum_cb)(ems_hash_fd *fd, ems_void *arg);


struct _ems_hash_fd_s {
	ems_queue      list;
	ems_int        crc;
	ems_str        key;
};


struct _ems_hash_s {
	ems_int        n_base;
	ems_queue     *base;
};


ems_hash  *ems_hash_create(ems_int n_base);
ems_int    ems_hash_init(ems_hash *hash, ems_int base);
ems_void   ems_hash_uninit(ems_hash *hash);
ems_void   ems_hash_destroy(ems_hash *hash);


ems_int      ems_hash_insert(ems_hash *hash, ems_hash_fd *fd);
ems_hash_fd *ems_hash_remove(ems_hash_fd *fd);
ems_hash_fd *ems_hash_find(ems_hash *hash, ems_cchar *key);

ems_int   ems_hash_fd_init(ems_hash_fd *fd);
ems_int   ems_hash_fd_set_key(ems_hash_fd *fd, ems_cchar *key);
ems_void  ems_hash_fd_uninit(ems_hash_fd *fd);

ems_cchar *ems_hash_key(ems_uint n);

ems_int  ems_hash_walk(ems_hash *hash, ems_hash_enum_cb cb, ems_void *arg);
#define ems_hash_enum   ems_hash_walk

ems_int ems_hash_clean(ems_hash *hash);


#endif
