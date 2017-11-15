
#ifndef AUDIT_NET_MODULES_HEADER____
#define AUDIT_NET_MODULES_HEADER____

#include <pcap.h>

#define SNAP_LEN 	1518
#define SIZE_ETHERNET 	14
#define ETHER_ADDR_LEN	6

typedef struct _net_flow_	net_flow;
typedef struct _net_plugins_	net_plugin;
typedef struct __port_map__	net_port_map;
typedef struct _net_core_s	net_core;

enum _net_plugin_id {
	id_net  = 2,         /* root */        
	id_http	= 100,       /* for http handle  */
	id_http_url   = 101, /* a post process */
	id_max
};

struct _net_plugins_ {
	ems_int    id;
	ems_int    mount; /* for post process's mount */
	ems_str    desc;
	ems_void  *ctx;

	ems_int   (*init)(net_plugin *);
	ems_int   (*process)(net_plugin *, ems_uint msgid, ems_void *);
	ems_int   (*uninit)(net_plugin *);

	ems_queue  entry_port; /* for bind on port */
	ems_queue  entry_post; /* for postprocess goes from here */
	ems_queue  entry;
};

struct __port_map__ {
	ems_int	    port;
	ems_int	    proto;
	ems_hash_fd h_port;
	ems_queue   entry; /* net plugins */
};

struct flow_ethernet {
        u_char  ether_dhost[ETHER_ADDR_LEN];    /* destination host address */
        u_char  ether_shost[ETHER_ADDR_LEN];    /* source host address */
        u_short ether_type;                     /* IP? ARP? RARP? etc */
};

/* IP header */
struct flow_ip {
        u_char  ip_vhl;                 /* version << 4 | header length >> 2 */
        u_char  ip_tos;                 /* type of service */
        u_short ip_len;                 /* total length */
        u_short ip_id;                  /* identification */
        u_short ip_off;                 /* fragment offset field */
        #define IP_RF 0x8000            /* reserved fragment flag */
        #define IP_DF 0x4000            /* dont fragment flag */
        #define IP_MF 0x2000            /* more fragments flag */
        #define IP_OFFMASK 0x1fff       /* mask for fragmenting bits */
        u_char  ip_ttl;                 /* time to live */
        u_char  ip_p;                   /* protocol */
        u_short ip_sum;                 /* checksum */
        struct  in_addr ip_src,ip_dst;  /* source and dest address */
};
#define IP_HL(ip)               (((ip)->ip_vhl) & 0x0f)
#define IP_V(ip)                (((ip)->ip_vhl) >> 4)

/* TCP header */
typedef u_int tcp_seq;

struct flow_tcp {
        u_short th_sport;               /* source port */
        u_short th_dport;               /* destination port */
        tcp_seq th_seq;                 /* sequence number */
        tcp_seq th_ack;                 /* acknowledgement number */
        u_char  th_offx2;               /* data offset, rsvd */
#define TH_OFF(th)      (((th)->th_offx2 & 0xf0) >> 4)
        u_char  th_flags;
        #define TH_FIN  0x01
        #define TH_SYN  0x02
        #define TH_RST  0x04
        #define TH_PUSH 0x08
        #define TH_ACK  0x10
        #define TH_URG  0x20
        #define TH_ECE  0x40
        #define TH_CWR  0x80
        #define TH_FLAGS        (TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG|TH_ECE|TH_CWR)
        u_short th_win;                 /* window */
        u_short th_sum;                 /* checksum */
        u_short th_urp;                 /* urgent pointer */
};


/* UDP header */
struct flow_udp {
        u_short u_sport;               /* source port */
        u_short u_dport;               /* destination port */
        u_short u_len;                 /* length number */
        u_short u_sum;                 /* checksum*/
};

#ifdef USE_IPV6
/* do ipv6's filter */
#endif

struct _net_flow_ {
	ems_uchar  *l2; /* level 2 ethernet */
	ems_uchar  *l3; /* level 3 network */
	ems_uchar  *l4; /* level 4 transport level, tcp , udp, icmp...*/
	ems_uchar  *l5; /* application level */
	ems_uint    l5_len;

	const struct pcap_pkthdr *hdr;
	const u_char *packet;

	ems_str     gwip;  /* gw ip address goes from here */
	ems_str     apmac;
	ems_cchar  *user;  /* username */
};

struct _net_core_s {
	ems_queue     flt;     /* net_plugins */
	ems_hash     *h_port;  /* for port map hash table */
	ems_hash     *h_user;  /* hash for userinfo */

	net_flow      pkgs;
	ems_queue     pcap;    /* for all interfaces net_if */

	ems_timeout   to;
	ems_int       to_period;

	audit_status  st;
};

ems_int net_change_status(net_core *net, audit_status st);
ems_int net_plug_sendmsg(net_plugin *plg, ems_uint evt, ems_void *arg);
ems_int net_plug_broadcast(ems_queue *list, ems_uint evt, ems_void *arg);

ems_int net_unload_plugins(ems_queue *list);
ems_int net_load_plugins(ems_queue *list, ems_int mount);

ems_int net_sendmsg(ems_int s, ems_int d, ems_uint evt, ems_void *arg);
ems_queue *net_filters();
ems_cchar *net_mac2str(const u_char *s);

net_core  *netcorer();
ems_int net_load_all_netinf(net_core *net);

#endif
