
#include "fwd_hash.h"


static dt_int 
hash_crc(dt_cchar *src)
{
	dt_char ch;
	dt_int  fwd_crc = 0;

	while (*src) {
		ch = *src++;

		fwd_crc ^= ((ch << 8) ^ ch);
	}

	return fwd_crc;
}

static fwd_hash_fd *hash_find(dt_queue *head, dt_cchar *key)
{
	dt_queue    *p;
	fwd_hash_fd *fd;

	dt_assert(head && key);

	if (!(head && key))
		return NULL;

	dt_queue_foreach(head, p) {

		fd = dt_queue_data(p, fwd_hash_fd, list);

		dt_assert(str_text(&fd->key));

		if (strcmp(str_text(&fd->key), key) == 0)
			return fd;
	}

	return NULL;
}



fwd_hash *fwd_hash_create(dt_int n_base)
{
	fwd_hash *hash = NULL;

	hash = (fwd_hash *)dt_malloc(sizeof(fwd_hash));

	if (hash) {
		if (fwd_hash_init(hash, n_base) != FWD_OK) {
			dt_free(hash);
			hash = NULL;
		}
	}

	return hash;
}

dt_int fwd_hash_init(fwd_hash *hash, dt_int base)
{
	dt_int i;

	dt_assert(hash && base > 0 && "invalid arg");

	memset(hash, 0, sizeof(fwd_hash));

	hash->base = (dt_queue *)dt_malloc(sizeof(dt_queue) * base);
	if (!hash->base)
		return FWD_ERR;

	hash->n_base = base; 

	for (i = 0; i < hash->n_base; i++) {
		dt_queue_init(&(hash->base[i]));
	}

	return FWD_OK;
}

dt_void  fwd_hash_uninit(fwd_hash *hash)
{
	dt_assert(hash);

	if (hash->base) {
		dt_free(hash->base);
		hash->base = NULL;
	}

	hash->n_base = 0;
}

dt_void fwd_hash_destroy(fwd_hash *hash)
{
	dt_assert(hash);

	if (hash) {
		fwd_hash_uninit(hash);
		dt_free(hash);
	}
}


dt_int fwd_hash_insert(fwd_hash *hash, fwd_hash_fd *fd)
{
	fwd_hash_fd *tmp;
	dt_queue    *head;

	dt_assert(hash && fd);

	fd->crc = fd->crc % hash->n_base;

	dt_assert(fd->crc >= 0 && fd->crc < hash->n_base);

	head = &(hash->base[fd->crc]);

	tmp = hash_find(head, str_text(&fd->key));
	if (tmp)
		return FWD_ERR_TICKET_EXIST;

	dt_queue_insert_head(head, &fd->list);
	return FWD_OK;
}

fwd_hash_fd *fwd_hash_remove(fwd_hash_fd *fd)
{
	dt_queue_remove(&fd->list);
	return fd;
}

fwd_hash_fd *fwd_hash_find(fwd_hash *hash, dt_cchar *key)
{
	dt_int     crc;
	dt_queue  *head;

	crc  = hash_crc(key);

	dt_assert(hash->n_base > 2 && hash->base);

	crc = crc % hash->n_base;
	dt_assert(crc >= 0 && crc < hash->n_base);

	head = &hash->base[crc];

	return hash_find(head, key);
}

dt_int fwd_hash_fd_init(fwd_hash_fd *fd)
{
	dt_assert(fd);

	memset(fd, 0, sizeof(fwd_hash_fd));

	str_init(&fd->key);
	dt_queue_init(&fd->list);

	return FWD_OK;
}

dt_int fwd_hash_fd_set_ticket(fwd_hash_fd *fd, dt_cchar *key)
{
	dt_assert(fd && key);

	fd->crc = hash_crc(key);
	str_set(&fd->key, key);

	return FWD_OK;
}

dt_void  fwd_hash_fd_uninit(fwd_hash_fd *fd)
{
	dt_assert(fd);

	str_uninit(&fd->key);

	memset(fd, 0, sizeof(fwd_hash_fd));
}


dt_int fwd_hash_walk(fwd_hash *hash, fwd_hash_enum_cb cb, dt_void *arg)
{
	dt_int        i, ret;
	dt_queue     *p, *n;
	fwd_hash_fd  *fd;

	dt_assert(hash && cb);

	if (!(hash && cb))
		return FWD_ERR;

	for (i = 0; i < hash->n_base; i++) {

		dt_queue_foreach_safe(&hash->base[i], p, n) {

			fd = dt_queue_data(p, fwd_hash_fd, list);

			ret = cb(fd, arg);

			if (ret != FWD_OK)
				return ret;
		}
	}

	return FWD_OK;
}

