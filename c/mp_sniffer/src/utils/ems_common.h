
#ifndef EMS_HEADER___ALL_____
#define EMS_HEADER___ALL_____

#include "ems.h"

# include <endian.h>
# if __BYTE_ORDER == __BIG_ENDIAN
#  define WORDS_BIGENDIAN 1
# endif


/*
 * MSG FROM AC
 * */
#define MSG_FROM_AC	0x1001
#define MSG_FROM_APCFG	0x1002

/*
 * MSG FROM SNIFFER
 * */
#define MSG_EVT_SEND_TO_WTP	0x1101
#define MSG_EVT_SEND_TO_APCFG	0x1102


/*
   error msgs goes from here
 */
#define ERR_BASE		  0x1200
#define ERR_EVT_TYPE_INVALID      (ERR_BASE + 1)
#define ERR_EVT_INTERFACE_UNKNOWN (ERR_BASE + 2)
#define ERR_EVT_OVER_MAX_CAPTURE  (ERR_BASE + 3)
#define ERR_EVT_TYPE_CONFILICT	  (ERR_BASE + 4)
#define ERR_EVT_NO_SPACE_LEFT	  (ERR_BASE + 5)
#define ERR_EVT_CAPTURE_BUSY	  (ERR_BASE + 6)
#define ERR_EVT_MEMORY_ERROR	  (ERR_BASE + 7)
#define ERR_EVT_SERVER_MISSING	  (ERR_BASE + 8)
#define ERR_EVT_SERVER_FTP_HOST	  (ERR_BASE + 9)
#define ERR_EVT_LOAD_PCAP_FILED	  (ERR_BASE + 10)
#define ERR_EVT_PCAP_FILTER	  (ERR_BASE + 11)
#define ERR_EVT_PCAP_ERROR	  (ERR_BASE + 12)
#define ERR_EVT_UPLOAD_FAILED	  (ERR_BASE + 13)


#define CAPTURE_TYPE_RADIO		1
#define CAPTURE_TYPE_WLAN		2
#define CAPTURE_TYPE_USER_SPECIFIED	3

#define WTP_EVT_START		1
#define WTP_EVT_STOP		2
#define WTP_EVT_UPLOADING	3
#define WTP_EVT_PAUSED		4
#define WTP_EVT_CONTINUE	5


#define APCFG_MSG_START		1
#define APCFG_MSG_STOP		2
#define APCFG_MSG_TERM		3
#define APCFG_MSG_ENV_RESTORE	4
#define APCFG_MSG_UPDATE_CHANNEL	5

#define SERVER_TYPE_FTP		1


#pragma pack(push, 1)
/*
 * for request
 * +--------------+------------------+----
 *     TAG           LENGTH            VALUES ...
 * +--------------+------------------+----
 * */
typedef union {
	char val[4];
	struct {
		unsigned short tag;
		unsigned short len;
	};
} ems_request;

#pragma pack(pop)

#define EMS_CONTINUE	0xff11aabb
#define EMS_BUFFER_FULL	0xff11aabc
#define EMS_BUFFER_INSUFFICIENT	 0xff11aabd

#define SIZE_REQUEST	sizeof(ems_request)

#define ems_start_memory_trace	startMemoryTrace
#define ems_stop_memory_trace	stopMemoryTrace

#define ems_strlen(a)	(a?strlen(a):0)

#ifndef EMS_YES
#define EMS_YES		1
#endif

#ifndef EMS_NO
#define EMS_NO		0
#endif

#define ems_flag_like(fld, flg)		((fld) & (flg))
#define ems_flag_unlike(fld, flg)	!ems_flag_like(fld, flg)
#define ems_flag_set(fld, flg)		((fld) |= (flg))
#define ems_flag_unset(fld, flg)	if(ems_flag_like(fld, flg)) ((fld) ^= (flg))
#define ems_flag_test			ems_flg_like

#endif
