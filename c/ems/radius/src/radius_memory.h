#ifndef _radius_h_memory_111_
#define _radius_h_memory_111_


#ifdef USE_EMS_MEMORY_MGMT
#include "ems_block.h"
#define radius_malloc	Malloc
#define radius_free	Free
#define radius_strdup	STRDup
#define radius_realloc	Realloc
#define radius_memdup	MEMDup
#define radius_assert	Assert
#define radius_calloc	Calloc

#else
#define radius_malloc	malloc
#define radius_free	free
#define radius_strdup	strdup
#define radius_realloc	realloc
#define radius_memdup	memdup
#define radius_assert	assert
#define radius_calloc	calloc
#endif

#endif
