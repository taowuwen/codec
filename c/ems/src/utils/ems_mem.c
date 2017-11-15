
#ifdef USE_MEM_POOL

#include "ems.h"
#include "ems_block.h"
#include "ems_utils.h"
#include "ems_mem.h"

typedef struct _mem_slab_s mem_slab;
typedef struct _mem_cls_s  mem_class;
typedef struct _mem_mgmt_s mem_mgmt;

struct _mem_slab_s {
	ems_void *_head;
	ems_void *_tail;
	ems_void *_buf;

	struct {
		ems_uchar *super;
		ems_int    cur;
		ems_int    left;
	} map;

	mem_class *cls;
	ems_queue  entry;
};


struct _mem_cls_s {
	ems_int sz_slab;
	ems_int sz_chunk;
	ems_int perslab; /* total chunks */

#ifdef USE_MULTI_THREAD
	ems_int thread_safe;
	ems_mtx mtx;
#endif
	mem_mgmt *mgmt;
	ems_queue list;  /* slab lists */
	ems_queue entry;
};

struct _mem_mgmt_s {
	ems_queue list;   /* for class lists */
	float     factor; /* growth factor */

	ems_int   max_chunk;
	ems_int   min_chunk;

	ems_int   maxcls;
	ems_int   thread_safe;
};

#define BLOCK_FREE	0
#define BLOCK_USED	1

#define MAX_CHUNK_SIZE	2048
#define BLOCK_SIZE	65536 /* 64 * 1024 == 65536 ==  64K  0xffff*/

#ifdef USE_MULTI_THREAD
static void mem_class_lock(mem_class *cls)
{
	if (cls->thread_safe)
		ems_mtx_lock(cls->mtx);

}

static void mem_class_unlock(mem_class *cls)
{
	if (cls->thread_safe)
		ems_mtx_unlock(cls->mtx);
}
#else
#define mem_class_lock(A)
#define mem_class_unlock(A)
#endif

#ifdef MEM_POOL_TRACE
#define mem_fprintf	fprintf
#else
#define mem_fprintf(...)
#endif


static void mem_pool_slab_uninit(mem_slab *slab)
{
	ems_assert(slab != NULL);

	ems_assert(slab->map.left == slab->cls->perslab);

	if (slab->_buf) {
		free(slab->_buf);
		slab->_buf = NULL;
	}

	if (slab->map.super) {
		free(slab->map.super);
		slab->map.super = NULL;
	}

	slab->cls = NULL;
}

static void mem_pool_uninit_class(mem_class *cls)
{
	ems_queue *p, *q;
	mem_slab  *slab;

	ems_assert(cls != NULL);
#ifdef DEBUG
	{
		int len = 0;
		ems_queue_len(&cls->list, len);
		ems_assert(len <= 1);
	}
#endif

	ems_queue_foreach_safe(&cls->list, p, q) {
		slab = ems_container_of(p, mem_slab, entry);
		ems_queue_remove(p);

		mem_pool_slab_uninit(slab);
		free(slab);
	}

#ifdef USE_MULTI_THREAD
	ems_mtx_unlock(cls->mtx);
#endif
}

static int mem_pool_init_class(mem_mgmt *mgmt)
{
	ems_int    id;
	mem_class *cls;

	for ( id = 0; mgmt->max_chunk <= MAX_CHUNK_SIZE && id < mgmt->maxcls; id++) {

		cls = (mem_class *)malloc(sizeof(mem_class));

		if (!cls)
			return EMS_ERR;

		cls->sz_slab  = BLOCK_SIZE;
		cls->sz_chunk = mgmt->max_chunk;
		cls->perslab  = cls->sz_slab / cls->sz_chunk;

#ifdef USE_MULTI_THREAD
		cls->thread_safe = mgmt->thread_safe;
		ems_mtx_init(cls->mtx);
#endif
		cls->mgmt     = mgmt;
		ems_queue_init(&cls->list);
		ems_queue_init(&cls->entry);

		mem_fprintf(stderr, "[mem] %2d size: %10d, chunk: %10d, perslab: %10d\n",
				id, cls->sz_slab, cls->sz_chunk, cls->perslab);

		/* it's a sorted list */
		ems_queue_insert_tail(&mgmt->list, &cls->entry);

		mgmt->max_chunk = (int) (mgmt->max_chunk * mgmt->factor);
	}

	return EMS_OK;
}

static mem_class *mem_pool_get_class(mem_mgmt *mgmt, int sz)
{
	ems_queue *p;
	mem_class *cls;

	ems_queue_foreach(&mgmt->list, p) {
		cls = ems_container_of(p, mem_class, entry);
		if (sz <= cls->sz_chunk)
			return cls;
	}

	return NULL;
}

static int mem_slab_get_block(mem_slab *slab)
{
	ems_uchar *map;

	int tail;
	int cur = slab->map.cur < 0 ? 0:slab->map.cur;

	map = slab->map.super;
	tail = cur;

	if (*(map + cur) == (ems_uchar)BLOCK_FREE)
		return cur;

	for ( cur = (cur + 1) % slab->cls->perslab;
	      tail != cur;
	      cur = (cur + 1) % slab->cls->perslab) {

		if (*(map + cur) == (ems_uchar)BLOCK_FREE)
			return cur;
	}

	return -1;
}

static void *mem_slab_malloc(mem_slab *slab)
{
	ems_void  *ptr;
	int cur;

	ems_assert(slab->map.left > 0 && slab->map.super);

	if (slab->map.left <= 0)
		return NULL;

	cur = mem_slab_get_block(slab);
	if (cur < 0)
		return NULL;

	ems_assert(*(slab->map.super + cur) == (ems_uchar)BLOCK_FREE);

	ptr = slab->_buf + cur * slab->cls->sz_chunk;
	*(slab->map.super + cur) = (ems_uchar)BLOCK_USED;

	slab->map.cur = (cur + 1) % slab->cls->perslab;
	slab->map.left--;

	mem_fprintf(stderr, "[mem]slab (%p <= %p < %p) malloc[next: %4d, left: %4d, current: %4d]\n",
		slab->_head, ptr, slab->_tail, slab->map.cur, slab->map.left, cur);

	return ptr;
}

static ems_int mem_pool_slab_init(mem_class *cls, mem_slab *slab)
{
	ems_assert(slab != NULL);

	memset(slab, 0, sizeof(mem_slab));

	slab->_buf = (void *)malloc(cls->sz_slab * sizeof(ems_uchar));
	if (!slab->_buf)
		return EMS_ERR;

	slab->_head = slab->_buf;
	slab->_tail = slab->_head + cls->sz_slab;

	slab->map.super = (ems_uchar *)malloc((cls->perslab + 1) * sizeof(ems_uchar));
	if (!slab->map.super) {
		free(slab->_buf);
		return EMS_ERR;
	}

	memset(slab->map.super, BLOCK_FREE, cls->perslab + 1);
	slab->map.cur  = 0;
	slab->map.left = cls->perslab;

	slab->cls = cls;
	ems_queue_init(&slab->entry);

	return EMS_OK;
}

static mem_slab *mem_pool_slab_find(mem_class *cls)
{
	ems_queue *p;
	mem_slab  *slab;

	ems_queue_foreach(&cls->list, p) {
		slab = ems_container_of(p, mem_slab, entry);
		if (slab->map.left > 0)
			return slab;
	}

	return NULL;
}

static void *mem_pool_slab_malloc(mem_class *cls)
{
	mem_slab *slab;
	void     *ptr = NULL;

	if (!cls)
		return NULL;

	mem_class_lock(cls);

	slab = mem_pool_slab_find(cls);

	if (!slab) {
		mem_fprintf(stderr, "[mem]class:(%d, %d, %d) has no slab left, create one\n",
				cls->sz_slab, cls->sz_chunk, cls->perslab);
		slab = (mem_slab *)malloc(sizeof(mem_slab));
		if (!slab)
			goto err_out;

		if (mem_pool_slab_init(cls, slab) != EMS_OK) {
			free(slab);
			goto err_out;
		}

		ems_queue_insert_head(&cls->list, &slab->entry);
	}

	ptr = mem_slab_malloc(slab);

err_out:
	mem_class_unlock(cls);
	return ptr;
}

static void mem_pool_slab_free(mem_slab *slab, void *ptr)
{
	mem_class *cls = slab->cls;
	int cur;

	mem_class_lock(cls);

	ems_assert(slab != NULL && ptr != NULL);
	ems_assert(slab->_head <= ptr && slab->_tail > ptr);

	cur = (ptr - slab->_buf) / cls->sz_chunk;

	ems_assert(cur >= 0 && cur < cls->perslab);
	if (cur >= 0 && cur < cls->perslab) {
		*(slab->map.super + cur) = (ems_uchar)BLOCK_FREE;
		slab->map.left++; 

		if (*(slab->map.super + slab->map.cur) == (ems_uchar)BLOCK_USED)
			slab->map.cur = cur;
	}
	mem_fprintf(stderr, "[mem]slab (%p <= %p < %p) free  [next: %4d, left: %4d, current: %4d]\n",
		slab->_head, ptr, slab->_tail, slab->map.cur, slab->map.left, cur);

	if (slab->map.left == cls->perslab) {
		ems_queue *p = &slab->entry;

		/* if it is the only one we got, do not free it */
		if (ems_queue_head(&cls->list) != p || 
		    ems_queue_last(&cls->list) != p) {

			mem_fprintf(stderr, "[mem]slab (%p <= %p < %p) delete one\n",
				slab->_head, ptr, slab->_tail);
			ems_queue_remove(&slab->entry);
			mem_pool_slab_uninit(slab);
			free(slab);
		}
	}

	mem_class_unlock(cls);
}

static mem_slab *mem_pool_find_ptr(mem_mgmt *mgmt, void *ptr)
{
	ems_queue *pcls, *p;
	mem_class *cls;
	mem_slab  *slab;

	ems_queue_foreach(&mgmt->list, pcls) {

		cls = ems_container_of(pcls, mem_class, entry);

		ems_queue_foreach(&cls->list, p) {
			slab = ems_container_of(p, mem_slab, entry);

			if (slab->_head <= ptr && slab->_tail > ptr)
				return slab;
		}
	}

	return NULL;
}

static mem_mgmt *_gmem = NULL;

int mem_pool_init(ems_int thread_safe, float factor)
{
	_gmem = (mem_mgmt *)malloc(sizeof(mem_mgmt));
	if (!_gmem)
		return EMS_ERR;

	ems_queue_init(&_gmem->list);

	_gmem->factor = factor;
	_gmem->min_chunk = 16;
	_gmem->max_chunk = _gmem->min_chunk;
	_gmem->maxcls = 10;
	_gmem->thread_safe = thread_safe;

	if (mem_pool_init_class(_gmem) != EMS_OK) {
		mem_pool_uninit();
		return EMS_ERR;
	}

	return EMS_OK;
}

int mem_pool_uninit()
{
	mem_class *cls;
	ems_queue *p, *q;

	if (!_gmem)
		return EMS_ERR;

	ems_queue_foreach_safe(&_gmem->list, p, q) {
		cls = ems_container_of(p, mem_class, entry);
		ems_queue_remove(p);

		mem_pool_uninit_class(cls);
		free(cls);
	}

	free(_gmem);
	_gmem = NULL;

	return EMS_OK;
}


void *mem_malloc(int sz)
{
	ems_void *ptr = NULL;

	if (!_gmem || sz > _gmem->max_chunk)
		return malloc(sz);

	ptr = mem_pool_slab_malloc(mem_pool_get_class(_gmem, sz));

	if (ptr == NULL)
		return malloc(sz);

	return ptr;
}

void mem_free(void *ptr)
{
	mem_slab *slab;

	if (!_gmem)
		return free(ptr);

	slab = mem_pool_find_ptr(_gmem, ptr);
	if (slab) {
		mem_pool_slab_free(slab, ptr);
	} else
		free(ptr);
}

void *mem_realloc(void *ptr, int sz)
{
	mem_slab *slab;

	if (!_gmem)
		return realloc(ptr, sz);

	slab = mem_pool_find_ptr(_gmem, ptr);
	if (slab) {
		if (slab->cls->sz_chunk >= sz)
			return ptr;
		else {
			void *ptr_new = mem_malloc(sz);
			if (ptr_new) {
				memcpy(ptr_new, ptr, slab->cls->sz_chunk);
				mem_pool_slab_free(slab, ptr);

				return ptr_new;
			}
		}
	} else
		return realloc(ptr, sz);

	return NULL;
}

#endif
