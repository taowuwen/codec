

#ifndef EMS_NET_HEADER___
#define EMS_NET_HEADER___

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef WIN32
	#include <windows.h>
	#include <sys/types.h>
#else
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <sys/un.h>
	#include <unistd.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <arpa/nameser.h>
	#include <resolv.h>
	#include <errno.h>
	#include <fcntl.h>
#endif


#include "ems_types.h"
#include "ems_str.h"
#include "ems_queue.h"
#include "ems_utils.h"
#include "ems_block.h"

#ifdef WIN32
#include "ems_getopt.h"
#define getopt	ems_getopt
#endif

#define ems_malloc	Malloc
#define ems_free		Free
#define ems_strdup	STRDup
#define ems_realloc	Realloc
#define ems_memdup	MEMDup
#define ems_assert	Assert


#ifdef WIN32
	#define ems_sleep(x) Sleep((x)*1000)
#else
	#define ems_sleep sleep
#endif

#endif
