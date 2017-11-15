#ifndef _json_h_memory_111_
#define _json_h_memory_111_


#ifdef USE_EMS_MEMORY_MGMT
#include "ems_block.h"
#define json_malloc	Malloc
#define json_free	Free
#define json_strdup	STRDup
#define json_realloc	Realloc
#define json_memdup	MEMDup
#define json_assert	Assert
#define json_calloc	Calloc

#else
#define json_malloc	malloc
#define json_free	free
#define json_strdup	strdup
#define json_realloc	realloc
#define json_memdup	memdup
#define json_assert	assert
#define json_calloc	calloc
#endif

#endif
