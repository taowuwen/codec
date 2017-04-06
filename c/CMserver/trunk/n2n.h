/*
 * (C) 2007-09 - Luca Deri <deri@ntop.org>
 *               Richard Andrews <andrews@ntop.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>
 *
 * Code contributions courtesy of:
 *    Babak Farrokhi <babak@farrokhi.net> [FreeBSD port]
 *    Lukasz Taczuk
 *
 */

#ifndef _N2N_H_
#define _N2N_H_

/*
   tunctl -t tun0
   tunctl -t tun1
   ifconfig tun0 1.2.3.4 up
   ifconfig tun1 1.2.3.5 up
   ./edge -d tun0 -l 2000 -r 127.0.0.1:3000 -c hello
   ./edge -d tun1 -l 3000 -r 127.0.0.1:2000 -c hello


   tunctl -u UID -t tunX
*/

#if defined(__APPLE__) && defined(__MACH__)
#define _DARWIN_
#endif


/* Some capability defaults which can be reset for particular platforms. */
#define N2N_HAVE_DAEMON 1
#define N2N_HAVE_SETUID 1
/* #define N2N_CAN_NAME_IFACE */

/* Moved here to define _CRT_SECURE_NO_WARNINGS before all the including takes place */
#ifdef WIN32
#include "win32/n2n_win32.h"
#undef N2N_HAVE_DAEMON
#undef N2N_HAVE_SETUID
#endif

#include <time.h>
#include <ctype.h>
#include <stdlib.h>

#ifndef WIN32
#include <netdb.h>
#endif

#ifndef _MSC_VER
#include <getopt.h>
#endif /* #ifndef _MSC_VER */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

#ifndef WIN32
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <pthread.h>

#ifdef __linux__
#include <linux/if.h>
#include <linux/if_tun.h>
#define N2N_CAN_NAME_IFACE 1
#endif /* #ifdef __linux__ */

#ifdef __FreeBSD__
#include <netinet/in_systm.h>
#endif /* #ifdef __FreeBSD__ */

#include <syslog.h>
#include <sys/wait.h>

#define ETH_ADDR_LEN 6
struct ether_hdr
{
    uint8_t  dhost[ETH_ADDR_LEN];
    uint8_t  shost[ETH_ADDR_LEN];
    uint16_t type;                /* higher layer protocol encapsulated */
} __attribute__ ((__packed__));

typedef struct ether_hdr ether_hdr_t;

#ifdef __sun__
#include <sys/sysmacros.h> /* MIN() and MAX() declared here */
#undef N2N_HAVE_DAEMON
#endif /* #ifdef __sun__ */

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>

#define closesocket(a) close(a)
#endif /* #ifndef WIN32 */

#include <string.h>

#include <stdarg.h>

#ifdef WIN32
#include "win32/wintap.h"
#endif /* #ifdef WIN32 */

#include "n2n_wire.h"
#include <list>
#include <string>
#include <map>
#include <iostream>

using namespace std;

#include "json/json.h"


#if defined(DEBUG)
#define PURGE_REGISTRATION_FREQUENCY   60
#define REGISTRATION_TIMEOUT          120
#define REGISTRATION_TIMEOUT          120
#else /* #if defined(DEBUG) */
#define PURGE_REGISTRATION_FREQUENCY   30
#define REGISTRATION_TIMEOUT           (180)
#define FWD_TIMEOUT							(180)
#endif /* #if defined(DEBUG) */



#define SERVER_VERSION                 	(0x0001)

#define MSG_P2P_GET             			 (0x0001)
#define MSG_P2P_ACK             		 	(0x0002)
#define MSG_P2P_HOLE_PUNCH             	(0x0003)
#define MSG_P2P_HOLE_PUNCH_ACK	(0x0004)
#define MSG_P2P_REGISTER             	 (0x0005)
#define SHARE_P2P_DEVINFO	             (0x4005)

//协助中转连接命令
#define REGIST_FORWARD_INFO	             (0x1501)
#define REQUEST_FORWARD_INFO	             (0x1502)
#define NOTIFY_TICKET_INFO	             (0x1503)
#define REQUEST_RELEASE_TICKET	             (0x1505)
#define FWD_TO_CM_HEARTBEAT  				(0x1506)


//指令头，6字节
typedef struct _p2p_head
{
	unsigned short datasize; //数据大小
	unsigned short version; //版本
	unsigned short cmd;//指令

}p2p_head;

//设备信息
struct device_info
{
	
	char lan_ip[4]; //内网IP地址
	unsigned short lan_udp_port; //内网绑定的UDP端口
	unsigned short lan_tcp_port; //内网绑定的TCP端口
	
	
	char public_ip[4];	//xcloud外部公网IP地址	
	unsigned short public_udp_port; //xcloud外部公网的UDP端口			
	unsigned short upnp_tcp_port; //获取到的UPNP映射的UDP端口	
	
	char device[32];//设备信息
	
};


//心跳注册
struct xcloud_p2p_register
{
	
	struct device_info device;
	
	char id[32];//唯一序列号,目前取mac地址
	char username[32];
	//char uuid[64];
	char reserve[32];
} ;
struct xcloud_p2p_register_uuid
{
	
	struct device_info device;
	
	char id[32];//唯一序列号,目前取mac地址
	char username[32];
	char uuid[64];
	char reserve[32];
} ;


//手机获取信息
struct mobile_p2p_get
{	
	char username[32]; //用户名
	char reserve[32]; //保留
};


//服务端返回数据给手机

 struct p2p_server_respond
{
	

	char my_public_ip[4]; //我的公网ip
	unsigned short  my_public_udp_port; //我的公网udp端口
	
	
	struct device_info device;

};



//打洞
struct p2p_hole_punch
{
	

	unsigned short from_server;//非0为从服务器接收，0为客户端	
	
	char my_public_ip[4];//自己的公网IP
	unsigned short my_public_udp_port;//自己的公网UDP端口
	
	char dest_public_ip[4];//对方的公网IP
	unsigned short dest_public_udp_port;//对方的公网UDP端口

	char lan_ip[4];//我的内网IP
	unsigned short lan_tcp_port;	//TCP端口
	unsigned short lan_udp_port;//UDP端口
	unsigned short upnp_tcp_port;//upnp端口
	unsigned short upnp_udp_port;//UPNP  udp端口

};



//打洞响应
struct p2p_hole_ack
{
	
	unsigned short from_server;//非0为从服务器接收，0为客户端	
	
	char my_public_ip[4];//自己的公网IP
	unsigned short  my_public_udp_port;//自己的公网UDP端口

	char dest_public_ip[4];//对方的公网IP
	unsigned short  dest_public_udp_port;//对方的公网udp端口
};	

//分享获取p2p信息
typedef struct  _share_get_p2p_dev_respond
 {
    char szUserPcUuid[64];//分享电脑UUID
    char szReserve[32];//保留字段
}SHARE_P2P_GET_DEV_INFO;

//中转服务器信息
typedef struct st_forward_server_info
{
	char szIpv6[16]; //ipv6地址，为扩展成ipv6地址是使用；
	unsigned int uiServerIp; //中转服务器ip；
	unsigned short usServerPort; //中转服务器端口；
	char szServerUuid[32]; //中转服务器唯一标识；
	char szTicket[16]; //中转连接的Ticket	
	char cerror;	//0：成功， 其他：失败
	char szreserve[29]; //保留字段
}ST_FORWARD_SERVER_INFO;


typedef struct st_peer_info
{
	int fd;
	char szTicket[16];
	n2n_sock_t pcSock;
	n2n_sock_t phoneSock;
	ST_FORWARD_SERVER_INFO fwd_info;
}ST_PEER_INFO;

typedef struct st_forward_server_info_inmap
{
	time_t regTM;
	ST_FORWARD_SERVER_INFO serverInfo;
}ST_FORWARD_SERVER_INFO_INMAP;


typedef struct RecvPackageHead
{
    unsigned short usPackageLens; //数据长度
    unsigned short usVersion;    //客户端版本号
    unsigned short usOpCode;     //请求码
    unsigned short usResver;	//保留字段
}ST_RECV_PACKAGE_HEAD;

typedef struct RtnPackageHead
{
    unsigned short usPackageLens;//数据长度
    unsigned short usVersion;    //服务器版本号
    unsigned short usOpCode;     //请求码
    unsigned short usRtnCode;    //请求是否成功标志，0：成功， 其他：失败
 }ST_RTN_PACKAGE_HEAD;




/* N2N_IFNAMSIZ is needed on win32 even if dev_name is not used after declaration */
#define N2N_IFNAMSIZ            16 /* 15 chars * NULL */
#ifndef WIN32
typedef struct tuntap_dev {
  int           fd;
  uint8_t       mac_addr[6];
  uint32_t      ip_addr, device_mask;
  uint16_t      mtu;
  char          dev_name[N2N_IFNAMSIZ];
} tuntap_dev;

#define SOCKET int
#endif /* #ifndef WIN32 */



#define QUICKLZ               1

/* N2N packet header indicators. */
#define MSG_TYPE_REGISTER               1
#define MSG_TYPE_DEREGISTER             2
#define MSG_TYPE_PACKET                 3
#define MSG_TYPE_REGISTER_ACK           4
#define MSG_TYPE_REGISTER_SUPER         5
#define MSG_TYPE_REGISTER_SUPER_ACK     6
#define MSG_TYPE_REGISTER_SUPER_NAK     7
#define MSG_TYPE_FEDERATION             8

/* Set N2N_COMPRESSION_ENABLED to 0 to disable lzo1x compression of ethernet
 * frames. Doing this will break compatibility with the standard n2n packet
 * format so do it only for experimentation. All edges must be built with the
 * same value if they are to understand each other. */
#define N2N_COMPRESSION_ENABLED 1

#define DEFAULT_MTU   1400

/** Common type used to hold stringified IP addresses. */
typedef char ipstr_t[32];

/** Common type used to hold stringified MAC addresses. */
#define N2N_MACSTR_SIZE 32
typedef char macstr_t[N2N_MACSTR_SIZE];

typedef char n2n_uuid[64];

struct peer_info {
    struct peer_info *  next;
    n2n_community_t     community_name;
    n2n_mac_t           mac_addr;
    n2n_sock_t          sock;
	n2n_uuid			uuid;
    time_t              last_seen;
    struct device_info device;
};

struct n2n_edge; /* defined in edge.c */
typedef struct n2n_edge         n2n_edge_t;


/* ************************************** */

#define TRACE_ERROR     0, __FILE__, __LINE__
#define TRACE_WARNING   1, __FILE__, __LINE__
#define TRACE_NORMAL    2, __FILE__, __LINE__
#define TRACE_INFO      3, __FILE__, __LINE__
#define TRACE_DEBUG     4, __FILE__, __LINE__

/* ************************************** */

#define SUPERNODE_IP    "127.0.0.1"
#define SUPERNODE_PORT  1234

/* ************************************** */

#ifndef max
#define max(a, b) ((a < b) ? b : a)
#endif

#ifndef min
#define min(a, b) ((a > b) ? b : a)
#endif

/* ************************************** */

/* Variables */
/* extern TWOFISH *tf; */
extern int traceLevel;
extern int useSyslog;
extern const uint8_t broadcast_addr[6];
extern const uint8_t multicast_addr[6];

extern pthread_mutex_t g_MutexForwardInfo;
extern map<SOCKET, ST_FORWARD_SERVER_INFO_INMAP> g_MapForwardInfo;

extern pthread_mutex_t g_MutexTicket;
extern map<SOCKET, list<string> > g_MapTicketInfo;

extern map<string, ST_PEER_INFO> g_MapPeerInfo;


/* Functions */

void fwd_printhex(char *s, int len);


extern void traceEvent(int eventTraceLevel, const char* file, int line, const char * format, ...);

extern int  tuntap_open(tuntap_dev *device, char *dev, const char *address_mode, char *device_ip, 
			char *device_mask, const char * device_mac, int mtu);
extern int  tuntap_read(struct tuntap_dev *tuntap, unsigned char *buf, int len);
extern int  tuntap_write(struct tuntap_dev *tuntap, unsigned char *buf, int len);
extern void tuntap_close(struct tuntap_dev *tuntap);
extern void tuntap_get_address(struct tuntap_dev *tuntap);

extern SOCKET open_socket(int local_port, int bind_any);

extern char* intoa(uint32_t addr, char* buf, uint16_t buf_len);
extern char* macaddr_str(macstr_t buf, const n2n_mac_t mac);
extern int   str2mac( uint8_t * outmac /* 6 bytes */, const char * s );
extern char * sock_to_cstr( n2n_sock_str_t out,
                            const n2n_sock_t * sock );

extern int sock_equal( const n2n_sock_t * a, 
                       const n2n_sock_t * b );

extern uint8_t is_multi_broadcast(const uint8_t * dest_mac);
extern const char* msg_type2str(uint16_t msg_type);
extern void hexdump(const uint8_t * buf, size_t len);

void print_n2n_version();

void show_uuid( struct peer_info * list);


/* Operations on peer_info lists. */
struct peer_info * find_peer_by_mac( struct peer_info * list,
                                     const n2n_mac_t mac );
struct peer_info * find_peer_by_uuid( struct peer_info * list,
                                     const n2n_uuid uuid );

void   peer_list_add( struct peer_info * * list,
                      struct peer_info * newpeer );
size_t peer_list_size( const struct peer_info * list );
size_t purge_peer_list( struct peer_info ** peer_list, 
                        time_t purge_before );
size_t clear_peer_list( struct peer_info ** peer_list );
size_t purge_expired_registrations( struct peer_info ** peer_list );

//void  ClearTimeoutFWDServer();

/* version.c */
extern char *n2n_sw_version, *n2n_sw_osName, *n2n_sw_buildDate;

#endif /* _N2N_H_ */
