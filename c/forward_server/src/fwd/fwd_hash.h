
#ifndef FWD_HASH_TABLES___HEADER____
#define FWD_HASH_TABLES___HEADER____



typedef struct _fwd_hash_fd_s  fwd_hash_fd;
typedef struct _fwd_hash_s     fwd_hash;

typedef dt_int (*fwd_hash_enum_cb)(fwd_hash_fd *fd, dt_void *arg);


struct _fwd_hash_fd_s {
	dt_queue      list;
	dt_int        crc;
	dt_str        key;
};


struct _fwd_hash_s {
	dt_int        n_base;
	dt_queue     *base;
};



fwd_hash     *fwd_hash_create(dt_int n_base);
dt_int        fwd_hash_init(fwd_hash *hash, dt_int base);
dt_void       fwd_hash_uninit(fwd_hash *hash);
dt_void       fwd_hash_destroy(fwd_hash *hash);


dt_int        fwd_hash_insert(fwd_hash *hash, fwd_hash_fd *fd);
fwd_hash_fd  *fwd_hash_remove(fwd_hash_fd *fd);
fwd_hash_fd  *fwd_hash_find(fwd_hash *hash, dt_cchar *key);

dt_int   fwd_hash_fd_init(fwd_hash_fd *fd);
dt_int   fwd_hash_fd_set_ticket(fwd_hash_fd *fd, dt_cchar *key);
dt_void  fwd_hash_fd_uninit(fwd_hash_fd *fd);


dt_int  fwd_hash_walk(fwd_hash *hash, fwd_hash_enum_cb cb, dt_void *arg);
#define fwd_hash_enum   fwd_hash_walk






#endif
