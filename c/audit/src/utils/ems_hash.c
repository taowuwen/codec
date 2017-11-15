
#include "ems_common.h"
#include "ems_hash.h"


static ems_int 
hash_crc(ems_cchar *src)
{
	ems_char ch;
	ems_int  ems_crc = 0;

	while (*src) {
		ch = *src++;

		ems_crc ^= ((ch << 8) ^ ch);
	}

	return ems_crc;
}

static ems_hash_fd *hash_find(ems_queue *head, ems_cchar *key)
{
	ems_queue    *p;
	ems_hash_fd *fd;

	ems_assert(head && key);

	if (!(head && key))
		return NULL;

	ems_queue_foreach(head, p) {

		fd = ems_queue_data(p, ems_hash_fd, list);

		ems_assert(str_text(&fd->key));

		if (strcmp(str_text(&fd->key), key) == 0)
			return fd;
	}

	return NULL;
}



ems_hash *ems_hash_create(ems_int n_base)
{
	ems_hash *hash = NULL;

	hash = (ems_hash *)ems_malloc(sizeof(ems_hash));

	if (hash) {
		if (ems_hash_init(hash, n_base) != EMS_OK) {
			ems_free(hash);
			hash = NULL;
		}
	}

	return hash;
}

ems_int ems_hash_init(ems_hash *hash, ems_int base)
{
	ems_int i;

	ems_assert(hash && base > 0 && "invalid arg");

	memset(hash, 0, sizeof(ems_hash));

	hash->base = (ems_queue *)ems_malloc(sizeof(ems_queue) * base);
	if (!hash->base)
		return EMS_ERR;

	hash->n_base = base; 

	for (i = 0; i < hash->n_base; i++) {
		ems_queue_init(&(hash->base[i]));
	}

	return EMS_OK;
}

ems_void  ems_hash_uninit(ems_hash *hash)
{
	ems_assert(hash);

	if (hash->base) {
		ems_free(hash->base);
		hash->base = NULL;
	}

	hash->n_base = 0;
}

ems_void ems_hash_destroy(ems_hash *hash)
{
	ems_assert(hash);

	if (hash) {
		ems_hash_uninit(hash);
		ems_free(hash);
	}
}


ems_int ems_hash_insert(ems_hash *hash, ems_hash_fd *fd)
{
	ems_hash_fd *tmp;
	ems_queue    *head;

	ems_assert(hash && fd);

	fd->crc = fd->crc % hash->n_base;

	ems_assert(fd->crc >= 0 && fd->crc < hash->n_base);

	head = &(hash->base[fd->crc]);

	tmp = hash_find(head, str_text(&fd->key));
	if (tmp)
		return EMS_ERR_TICKET_EXIST;

	ems_queue_insert_head(head, &fd->list);
	return EMS_OK;
}

ems_hash_fd *ems_hash_remove(ems_hash_fd *fd)
{
	ems_queue_remove(&fd->list);
	return fd;
}

ems_hash_fd *ems_hash_find(ems_hash *hash, ems_cchar *key)
{
	ems_int     crc;
	ems_queue  *head;

	crc  = hash_crc(key);

	ems_assert(hash->n_base > 2 && hash->base);

	crc = crc % hash->n_base;
	ems_assert(crc >= 0 && crc < hash->n_base);

	head = &hash->base[crc];

	return hash_find(head, key);
}

ems_int ems_hash_fd_init(ems_hash_fd *fd)
{
	ems_assert(fd);

	memset(fd, 0, sizeof(ems_hash_fd));

	str_init(&fd->key);
	ems_queue_init(&fd->list);

	return EMS_OK;
}

ems_int ems_hash_fd_set_key(ems_hash_fd *fd, ems_cchar *key)
{
	ems_assert(fd && key);

	fd->crc = hash_crc(key);
	str_set(&fd->key, key);

	return EMS_OK;
}

ems_void  ems_hash_fd_uninit(ems_hash_fd *fd)
{
	ems_assert(fd);

	str_uninit(&fd->key);

	memset(fd, 0, sizeof(ems_hash_fd));
}


ems_int ems_hash_clean(ems_hash *hash)
{
	ems_int        i;

	for (i = 0; i < hash->n_base; i++) {
		ems_queue_clean(&hash->base[i]);
	}

	return EMS_OK;
}

ems_int ems_hash_walk(ems_hash *hash, ems_hash_enum_cb cb, ems_void *arg)
{
	ems_int        i, ret;
	ems_queue     *p, *n;
	ems_hash_fd  *fd;

	ems_assert(hash && cb);

	if (!(hash && cb))
		return EMS_ERR;

	for (i = 0; i < hash->n_base; i++) {

		ems_queue_foreach_safe(&hash->base[i], p, n) {

			fd = ems_queue_data(p, ems_hash_fd, list);

			ret = cb(fd, arg);

			if (ret != EMS_OK)
				return ret;
		}
	}

	return EMS_OK;
}

ems_cchar *ems_hash_key(ems_uint n)
{
	static ems_char tmp_buf[128] = {0};

	snprintf(tmp_buf, 128, "%x", n);
	return tmp_buf;
}

