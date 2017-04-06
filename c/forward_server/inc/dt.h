

#ifndef DT_NET_HEADER___
#define DT_NET_HEADER___

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


#include "dt_types.h"
#include "dt_str.h"
#include "dt_queue.h"
#include "dt_log.h"
#include "dt_utils.h"
#include "dt_block.h"

#ifdef WIN32
#include "dt_getopt.h"
#define getopt	dt_getopt
#endif

#define dt_malloc	Malloc
#define dt_free		Free
#define dt_strdup	STRDup
#define dt_realloc	Realloc
#define dt_memdup	MEMDup
#define dt_assert	Assert
#define dt_validblock	validBlock


#ifdef WIN32
	#define dt_sleep(x) Sleep((x)*1000)
#else
	#define dt_sleep sleep
#endif

#endif
