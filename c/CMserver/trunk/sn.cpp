/* Supernode for n2n-2.x */

/* (c) 2009 Richard Andrews <andrews@ntop.org> 
 *
 * Contributions by:
 *    Lukasz Taczuk
 *    Struan Bartlett
 */


#include "n2n.h"
#include <sys/time.h>
#include <sys/resource.h>

//p2p port
#define N2N_SN_LPORT_DEFAULT 6161

//tcp forward port 
#define N2N_SN_FORWARD_PORT_DEFAULT 6165

#define N2N_SN_PKTBUF_SIZE   2048

#define N2N_SN_MGMT_PORT                1234

static int count =0;

fd_set socket_mask;


struct sn_stats
{
    size_t errors;              /* Number of errors encountered. */
    size_t reg_super;           /* Number of REGISTER_SUPER requests received. */
    size_t reg_super_nak;       /* Number of REGISTER_SUPER requests declined. */
    size_t fwd;                 /* Number of messages forwarded. */
    size_t broadcast;           /* Number of messages broadcast to a community. */
    time_t last_fwd;            /* Time when last message was forwarded. */
    time_t last_reg_super;      /* Time when last REGISTER_SUPER was received. */
};

typedef struct sn_stats sn_stats_t;

struct n2n_sn
{
    time_t              start_time;     /* Used to measure uptime. */
    sn_stats_t          stats;
    int                 daemon;         /* If non-zero then daemonise. */
    uint16_t            lport;          /* Local UDP port to bind to. */
   
    int                 sock;           /* Main socket for UDP traffic with edges. */
    int                 mgmt_sock;      /* management socket. */
    struct peer_info *  edges;          /* Link list of registered edges. */
};

typedef struct n2n_sn n2n_sn_t;


static ssize_t Udpsendto_sock( int fd, const void * buf, size_t len, const n2n_sock_t * dest )
{
    struct sockaddr_in peer_addr;
    ssize_t sent;

    fill_sockaddr( (struct sockaddr *) &peer_addr,
                   sizeof(peer_addr),
                   dest );

    sent = sendto( fd, buf, len, 0/*flags*/,
                   (struct sockaddr *)&peer_addr, sizeof(struct sockaddr_in) );
    if ( sent < 0 )
    {
        char * c = strerror(errno);
        traceEvent( TRACE_ERROR, "sendto failed (%d) %s", errno, c );
    }
    else
    {
        traceEvent( TRACE_DEBUG, "sendto sent=%d to ", (signed int)sent );
    }

    return sent;
}

static void  SendFwdInfoToPeer(ST_PEER_INFO * fwd_peer_info)
{
	char sendbuf[2048]={0};
	int data_sent_len;
	int send_size;
	p2p_head * Pp2pHead = (p2p_head *)sendbuf;
	send_size = sizeof(p2p_head) + sizeof(fwd_peer_info->fwd_info);
	Pp2pHead->cmd = htons(REQUEST_FORWARD_INFO);
	Pp2pHead->datasize = htons(send_size);
	Pp2pHead->version = ntohs(SERVER_VERSION);
	traceEvent( TRACE_DEBUG, "SendFwdInfoToPeer ticket= %s,fd =%d",fwd_peer_info->szTicket,fwd_peer_info->fd);
	
	fwd_printhex((char *)&(fwd_peer_info->fwd_info),sizeof(fwd_peer_info->fwd_info));
	memcpy(sendbuf + sizeof(p2p_head), &(fwd_peer_info->fwd_info), sizeof(ST_FORWARD_SERVER_INFO));
	Udpsendto_sock(fwd_peer_info->fd, sendbuf, send_size, &fwd_peer_info->pcSock);
	Udpsendto_sock(fwd_peer_info->fd, sendbuf, send_size, &fwd_peer_info->phoneSock);
	
}

static string OS_SockNumberToIp(const unsigned long &ulIp);

static int OS_SockSend(SOCKET &sock, char *pBuff, const size_t isize);
static int OS_SockRecv(SOCKET sock, char *pBuff, const size_t isize);
static int try_forward( n2n_sn_t * sss, 
                        const n2n_common_t * cmn,
                        const n2n_mac_t dstMac,
                        const uint8_t * pktbuf,
                        size_t pktsize );

static int try_broadcast( n2n_sn_t * sss, 
                          const n2n_common_t * cmn,
                          const n2n_mac_t srcMac,
                          const uint8_t * pktbuf,
                          size_t pktsize );


void ClearTicketInfo(SOCKET fd)
{
	map <SOCKET, list<string> >::iterator itMap ;
	map <SOCKET, ST_FORWARD_SERVER_INFO_INMAP>::iterator itMapInfo ;
	//pthread_mutex_lock(&g_MutexTicket);	//clear ticket info
	itMap= g_MapTicketInfo.find(fd);
	if(itMap != g_MapTicketInfo.end())
	{
		g_MapTicketInfo.erase(itMap);
	}
	//pthread_mutex_unlock(&g_MutexTicket);
	
	//pthread_mutex_lock(&g_MutexForwardInfo);	//clear forward server info
	itMapInfo= g_MapForwardInfo.find(fd);
	if(itMapInfo != g_MapForwardInfo.end())
	{
		g_MapForwardInfo.erase(itMapInfo);
		
	}
	//pthread_mutex_unlock(&g_MutexForwardInfo);
}



/** Initialise the supernode structure */

static int init_sn( n2n_sn_t * sss )
{
#ifdef WIN32
    initWin32();
#endif
    memset( sss, 0, sizeof(n2n_sn_t) );

    sss->daemon = 1; /* By defult run as a daemon. */
    sss->lport = N2N_SN_LPORT_DEFAULT;
    sss->sock = -1;
    sss->mgmt_sock = -1;
    sss->edges = NULL;

    return 0; /* OK */
}

/** Deinitialise the supernode structure and deallocate any memory owned by
 *  it. */
static void deinit_sn( n2n_sn_t * sss )
{
    if (sss->sock >= 0)
    {
        closesocket(sss->sock);
    }
    sss->sock=-1;

    if ( sss->mgmt_sock >= 0 )
    {
        closesocket(sss->mgmt_sock);
    }
    sss->mgmt_sock=-1;

    purge_peer_list( &(sss->edges), 0xffffffff );
}


/** Determine the appropriate lifetime for new registrations.
 *
 *  If the supernode has been put into a pre-shutdown phase then this lifetime
 *  should not allow registrations to continue beyond the shutdown point.
 */
static uint16_t reg_lifetime( n2n_sn_t * sss )
{
    return 120;
}

void dump(unsigned char *p,int len)
{
	int i;
	printf("\n");
	for(i=0;i<len;i++)
	{
		printf("%.2x ",p[i]);
		if(i==15) printf("\n");
		if(i%16==0 && i>20)
			printf("\n");
	}
	printf("\n\n");
	
}


/** Update the edge table with the details of the edge which contacted the
 *  supernode. */
static int update_edge_uuid(struct xcloud_p2p_register_uuid *p2p_register,
			   n2n_sn_t * sss, 
                        const n2n_mac_t edgeMac,
                        const n2n_community_t community,
                        const n2n_sock_t * sender_sock,
                        time_t now)
{
    macstr_t            mac_buf;
    n2n_sock_str_t      sockbuf;
    struct peer_info *  scan;

   // traceEvent( TRACE_DEBUG, "update edge for %s [%s],uuid=%s",
               // macaddr_str( mac_buf, edgeMac ),
                //sock_to_cstr( sockbuf, sender_sock ),p2p_register->uuid );

    scan = find_peer_by_uuid( sss->edges, p2p_register->uuid );

    if ( NULL == scan )
    {
        /* Not known */

        scan = (struct peer_info*)calloc(1, sizeof(struct peer_info)); /* deallocated in purge_expired_registrations */
         if ( NULL == scan )
         	return 0;

        memcpy(scan->community_name, community, sizeof(n2n_community_t) );
        memcpy(&(scan->mac_addr), edgeMac, sizeof(n2n_mac_t));
        memcpy(&(scan->sock), sender_sock, sizeof(n2n_sock_t));
		memcpy(&(scan->uuid), p2p_register->uuid, sizeof(p2p_register->uuid));

        memcpy(&(scan->device),&(p2p_register->device),sizeof(struct device_info));
        memcpy(&(scan->device.public_ip),sender_sock->addr.v4,IPV4_SIZE);
        scan->device.public_udp_port=ntohs(sender_sock->port);

        /* insert this guy at the head of the edges list */
        scan->next = sss->edges;     /* first in list */
        sss->edges = scan;           /* head of list points to new scan */
        traceEvent( TRACE_INFO, "update_edge created   %s ==> %s",
                    macaddr_str( mac_buf, edgeMac ),
                    sock_to_cstr( sockbuf, sender_sock ) );
    }
    else
    {
        /* Known 
        if ( (0 != memcmp(community, scan->community_name, sizeof(n2n_community_t))) ||
             (0 != sock_equal(sender_sock, &(scan->sock) )) )
        {*/
            memcpy(scan->community_name, community, sizeof(n2n_community_t) );
            memcpy(&(scan->sock), sender_sock, sizeof(n2n_sock_t));
			memcpy(&(scan->uuid), p2p_register->uuid, sizeof(p2p_register->uuid));
	     //by wangmk	
            memcpy(&(scan->device),&(p2p_register->device),sizeof(struct device_info));
            memcpy(&(scan->device.public_ip),sender_sock->addr.v4,IPV4_SIZE);
             scan->device.public_udp_port=ntohs(sender_sock->port);
            traceEvent( TRACE_INFO, "update_edge updated   %s ==> %s port = %d",
                       macaddr_str( mac_buf, edgeMac ),
                       sock_to_cstr( sockbuf, sender_sock ) ,ntohs(scan->device.lan_udp_port));

    }
    dump((unsigned char *)&(scan->device),32);
	//traceEvent( TRACE_DEBUG, "update_edge uuid =  %s",scan->uuid);
    scan->last_seen = now;
    return 0;
}


static int update_edge(struct xcloud_p2p_register *p2p_register,
			   n2n_sn_t * sss, 
                        const n2n_mac_t edgeMac,
                        const n2n_community_t community,
                        const n2n_sock_t * sender_sock,
                        time_t now)
{
    macstr_t            mac_buf;
    n2n_sock_str_t      sockbuf;
    struct peer_info *  scan;

    //traceEvent( TRACE_DEBUG, "update edge for %s [%s]",
    //            macaddr_str( mac_buf, edgeMac ),
    //            sock_to_cstr( sockbuf, sender_sock ) );

    scan = find_peer_by_mac( sss->edges, edgeMac );

    if ( NULL == scan )
    {
        /* Not known */

        scan = (struct peer_info*)calloc(1, sizeof(struct peer_info)); /* deallocated in purge_expired_registrations */
         if ( NULL == scan )
         	return 0;

        memcpy(scan->community_name, community, sizeof(n2n_community_t) );
        memcpy(&(scan->mac_addr), edgeMac, sizeof(n2n_mac_t));
        memcpy(&(scan->sock), sender_sock, sizeof(n2n_sock_t));
		//memcpy(&(scan->uuid), p2p_register->uuid, sizeof(p2p_register->uuid));

        memcpy(&(scan->device),&(p2p_register->device),sizeof(struct device_info));
        memcpy(&(scan->device.public_ip),sender_sock->addr.v4,IPV4_SIZE);
        scan->device.public_udp_port=ntohs(sender_sock->port);

        /* insert this guy at the head of the edges list */
        scan->next = sss->edges;     /* first in list */
        sss->edges = scan;           /* head of list points to new scan */

       // traceEvent( TRACE_INFO, "update_edge created   %s ==> %s",
       //             macaddr_str( mac_buf, edgeMac ),
       //             sock_to_cstr( sockbuf, sender_sock ) );
    }
    else
    {
        /* Known */
        if ( (0 != memcmp(community, scan->community_name, sizeof(n2n_community_t))) ||
             (0 != sock_equal(sender_sock, &(scan->sock) )) )
        {
            memcpy(scan->community_name, community, sizeof(n2n_community_t) );
            memcpy(&(scan->sock), sender_sock, sizeof(n2n_sock_t));
//			memcpy(&(scan->uuid), p2p_register->uuid, sizeof(p2p_register->uuid));
	     //by wangmk	
            memcpy(&(scan->device),&(p2p_register->device),sizeof(struct device_info));
            memcpy(&(scan->device.public_ip),sender_sock->addr.v4,IPV4_SIZE);
             scan->device.public_udp_port=ntohs(sender_sock->port);

            /*traceEvent( TRACE_INFO, "update_edge updated   %s ==> %s",
                        macaddr_str( mac_buf, edgeMac ),
                        sock_to_cstr( sockbuf, sender_sock ) );*/
        }
        else
        {
           /* traceEvent( TRACE_DEBUG, "update_edge unchanged %s ==> %s",
                        macaddr_str( mac_buf, edgeMac ),
                        sock_to_cstr( sockbuf, sender_sock ) );*/
        }

    }
    //dump((unsigned char *)&(scan->device),32);	
    scan->last_seen = now;
    return 0;
}

/** Send a datagram to the destination embodied in a n2n_sock_t.
 *
 *  @return -1 on error otherwise number of bytes sent
 */
static ssize_t sendto_sock(n2n_sn_t * sss, 
                           const n2n_sock_t * sock, 
                           const uint8_t * pktbuf, 
                           size_t pktsize)
{
    n2n_sock_str_t      sockbuf;

    if ( AF_INET == sock->family )
    {
        struct sockaddr_in udpsock;

        udpsock.sin_family = AF_INET;
        udpsock.sin_port = htons( sock->port );
        memcpy( &(udpsock.sin_addr.s_addr), &(sock->addr.v4), IPV4_SIZE );

        traceEvent( TRACE_DEBUG, "sendto_sock %lu to [%s]",
                    pktsize,
                    sock_to_cstr( sockbuf, sock ) );

        return sendto( sss->sock, pktbuf, pktsize, 0, 
                       (const struct sockaddr *)&udpsock, sizeof(struct sockaddr_in) );
    }
    else
    {
        /* AF_INET6 not implemented */
        errno = EAFNOSUPPORT;
        return -1;
    }
}



/** Try to forward a message to a unicast MAC. If the MAC is unknown then
 *  broadcast to all edges in the destination community.
 */
static int try_forward( n2n_sn_t * sss, 
                        const n2n_common_t * cmn,
                        const n2n_mac_t dstMac,
                        const uint8_t * pktbuf,
                        size_t pktsize )
{
    struct peer_info *  scan;
    macstr_t            mac_buf;
    n2n_sock_str_t      sockbuf;

    scan = find_peer_by_mac( sss->edges, dstMac );

    if ( NULL != scan )
    {
        int data_sent_len;
        data_sent_len = sendto_sock( sss, &(scan->sock), pktbuf, pktsize );

        if ( data_sent_len == pktsize )
        {
            ++(sss->stats.fwd);
            traceEvent(TRACE_DEBUG, "unicast %lu to [%s] %s",
                       pktsize,
                       sock_to_cstr( sockbuf, &(scan->sock) ),
                       macaddr_str(mac_buf, scan->mac_addr));
        }
        else
        {
            ++(sss->stats.errors);
            traceEvent(TRACE_ERROR, "unicast %lu to [%s] %s FAILED (%d: %s)",
                       pktsize,
                       sock_to_cstr( sockbuf, &(scan->sock) ),
                       macaddr_str(mac_buf, scan->mac_addr),
                       errno, strerror(errno) );
        }
    }
    else
    {
        traceEvent( TRACE_DEBUG, "try_forward unknown MAC" );

        /* Not a known MAC so drop. */
    }
    
    return 0;
}


/** Try and broadcast a message to all edges in the community.
 *
 *  This will send the exact same datagram to zero or more edges registered to
 *  the supernode.
 */
static int try_broadcast( n2n_sn_t * sss, 
                          const n2n_common_t * cmn,
                          const n2n_mac_t srcMac,
                          const uint8_t * pktbuf,
                          size_t pktsize )
{
    struct peer_info *  scan;
    macstr_t            mac_buf;
    n2n_sock_str_t      sockbuf;

    traceEvent( TRACE_DEBUG, "try_broadcast" );

    scan = sss->edges;
    while(scan != NULL) 
    {
        if( 0 == (memcmp(scan->community_name, cmn->community, sizeof(n2n_community_t)) )
            && (0 != memcmp(srcMac, scan->mac_addr, sizeof(n2n_mac_t)) ) )
            /* REVISIT: exclude if the destination socket is where the packet came from. */
        {
            int data_sent_len;
          
            data_sent_len = sendto_sock(sss, &(scan->sock), pktbuf, pktsize);

            if(data_sent_len != pktsize)
            {
                ++(sss->stats.errors);
                traceEvent(TRACE_WARNING, "multicast %lu to [%s] %s failed %s",
                           pktsize,
                           sock_to_cstr( sockbuf, &(scan->sock) ),
                           macaddr_str(mac_buf, scan->mac_addr),
                           strerror(errno));
            }
            else 
            {
                ++(sss->stats.broadcast);
                traceEvent(TRACE_DEBUG, "multicast %lu to [%s] %s",
                           pktsize,
                           sock_to_cstr( sockbuf, &(scan->sock) ),
                           macaddr_str(mac_buf, scan->mac_addr));
            }
        }

        scan = scan->next;
    } /* while */
    
    return 0;
}


static int process_mgmt( n2n_sn_t * sss, 
                         const struct sockaddr_in * sender_sock,
                         const uint8_t * mgmt_buf, 
                         size_t mgmt_size,
                         time_t now)
{
    char resbuf[N2N_SN_PKTBUF_SIZE];
    size_t ressize=0;
    ssize_t r;

    traceEvent( TRACE_DEBUG, "process_mgmt" );

    ressize += snprintf( resbuf+ressize, N2N_SN_PKTBUF_SIZE-ressize, 
                         "----------------\n" );

    ressize += snprintf( resbuf+ressize, N2N_SN_PKTBUF_SIZE-ressize, 
                         "uptime    %lu\n", (now - sss->start_time) );

    ressize += snprintf( resbuf+ressize, N2N_SN_PKTBUF_SIZE-ressize, 
                         "edges     %u\n", 
			 (unsigned int)peer_list_size( sss->edges ) );

    ressize += snprintf( resbuf+ressize, N2N_SN_PKTBUF_SIZE-ressize, 
                         "errors    %u\n", 
			 (unsigned int)sss->stats.errors );

    ressize += snprintf( resbuf+ressize, N2N_SN_PKTBUF_SIZE-ressize, 
                         "reg_sup   %u\n", 
			 (unsigned int)sss->stats.reg_super );

    ressize += snprintf( resbuf+ressize, N2N_SN_PKTBUF_SIZE-ressize, 
                         "reg_nak   %u\n", 
			 (unsigned int)sss->stats.reg_super_nak );

    ressize += snprintf( resbuf+ressize, N2N_SN_PKTBUF_SIZE-ressize, 
                         "fwd       %u\n",
			 (unsigned int) sss->stats.fwd );

    ressize += snprintf( resbuf+ressize, N2N_SN_PKTBUF_SIZE-ressize, 
                         "broadcast %u\n",
			 (unsigned int) sss->stats.broadcast );

    ressize += snprintf( resbuf+ressize, N2N_SN_PKTBUF_SIZE-ressize, 
                         "last fwd  %lu sec ago\n", 
			 (long unsigned int)(now - sss->stats.last_fwd) );

    ressize += snprintf( resbuf+ressize, N2N_SN_PKTBUF_SIZE-ressize, 
                         "last reg  %lu sec ago\n",
			 (long unsigned int) (now - sss->stats.last_reg_super) );


    r = sendto( sss->mgmt_sock, resbuf, ressize, 0/*flags*/, 
                (struct sockaddr *)sender_sock, sizeof(struct sockaddr_in) );

    if ( r <= 0 )
    {
        ++(sss->stats.errors);
        traceEvent( TRACE_ERROR, "process_mgmt : sendto failed. %s", strerror(errno) );
    }

    return 0;
}


/** Examine a datagram and determine what to do with it.
 *
 */

int process_p2p_ack(n2n_sn_t * sss, uint8_t *username,uint8_t *outbuf)
{
	
 struct peer_info *list;
 int offset=0,i=0;
 

list= sss->edges;
while(list != NULL)
   {
        traceEvent( TRACE_DEBUG, "get uname=%s ,regist name =%s", username, list->community_name);
      if( 0 == memcmp(username ,list->community_name, sizeof(n2n_community_t)))
       {
       	memcpy(outbuf+offset,&(list->device),sizeof(struct device_info));
       	offset=offset+sizeof(struct device_info);
       	if(offset>1024)
       		break;
       }
      i++;
      list = list->next;
   }
traceEvent( TRACE_DEBUG, "Server has (%d) peers ,size=%d", i,offset );

return offset;
 

}


int process_share_p2p_ack(n2n_sn_t * sss, char *usruuid,uint8_t *outbuf)
{
	
 struct peer_info *list;
 int offset=0,i=0;
 

list= sss->edges;
while(list != NULL)
   {
   		traceEvent( TRACE_DEBUG, "list->uuid = ", list->uuid);
      if( 0 == memcmp(usruuid ,list->uuid, sizeof(n2n_uuid)))
       {
       	memcpy(outbuf+offset,&(list->device),sizeof(struct device_info));
       	offset=offset+sizeof(struct device_info);
       	//if(offset>1024)
       	break;
       }
      i++;
      list = list->next;
   }
traceEvent( TRACE_DEBUG, "Server has (%d) peers ,size=%d", i,offset );

return offset;
 

}

int generateRand(int n)
{
	return (rand()%n);
}

char *GenerateRandStr(char *str, int len)
{
		int j;
		int base;
		for(j = 0; j < len;)
		{
			base = (rand())%3;
			if(0 == base)
			{
				str[j] = (char)(generateRand(10)+'0');
				j++;
			}
			else if(1 == base)
			{
				str[j] = (char)(generateRand(26)+'a');
				if(('i' != str[j]) && ('o' != str[j]))
				{
					j++;
				}
			}
			else
			{
				str[j] = (char)(generateRand(26)+'A');
				if(('I' != str[j]) && ('O' != str[j]))
				{				
					j++;
				}
			}		
		}
		return str;
		
}

char * GenerateTicket(char *ticket)
{
	map <SOCKET, list<string> >::iterator itMap ;
	list<string>::iterator itStr;
	while(1)
	{
		ticket= GenerateRandStr(ticket, 16);
		traceEvent( TRACE_ERROR,"ticket = %s" ,ticket);
		pthread_mutex_lock(&g_MutexTicket);
		for(itMap = g_MapTicketInfo.begin(); itMap != g_MapTicketInfo.end(); itMap++)
		{
			for(itStr= itMap->second.begin(); itStr != itMap->second.end(); itStr++)
			{
				if(*itStr == string(ticket))
				{
					pthread_mutex_unlock(&g_MutexTicket);
					continue;
				}
			}
		}
		pthread_mutex_unlock(&g_MutexTicket);
		break;
		
	}
}

int ChoseForwardServer(ST_FORWARD_SERVER_INFO *serverInfo, char *ticket)
{
	SOCKET forwardsock = 0;
	map <SOCKET, list<string> >::iterator itMap ;
	map <SOCKET, ST_FORWARD_SERVER_INFO_INMAP>::iterator itMapInfo ;
	list<string>::iterator itStr;
	int size = 0;
	int flag = 1;
	//pthread_mutex_lock(&g_MutexTicket);	
	for(itMap = g_MapTicketInfo.begin(); itMap != g_MapTicketInfo.end(); itMap++)
	{
		if(flag==1)
		{
			size = itMap->second.size();
			forwardsock = itMap->first;
		}
		else
		{
			if(size > itMap->second.size())
			{
				size = itMap->second.size();
				forwardsock = itMap->first;
			}
		}
		traceEvent( TRACE_ERROR,"size =  %d, socket = %d" ,size, forwardsock);
		flag++;
	}
	//pthread_mutex_unlock(&g_MutexTicket);
	traceEvent( TRACE_ERROR,"after size =  %d, socket = %d" ,size, forwardsock);
	//pthread_mutex_lock(&g_MutexForwardInfo);
	if(forwardsock != 0)
	{
		itMapInfo = g_MapForwardInfo.find(forwardsock);
		if(itMapInfo == g_MapForwardInfo.end())
		{
			traceEvent( TRACE_ERROR,"REQUEST_FORWARD_INFO not find sock %d" ,forwardsock);
			//pthread_mutex_unlock(&g_MutexForwardInfo);
			return -1;
		
		}
	}
	else
	{
		if(g_MapForwardInfo.size()!=0)
		{
			itMapInfo = g_MapForwardInfo.begin();
		}
		else
		{
			traceEvent( TRACE_ERROR,"REQUEST_FORWARD_INFO g_MapForwardInfo.size=%d" ,g_MapForwardInfo.size());
			//pthread_mutex_unlock(&g_MutexForwardInfo);
			return -1;
		}
	}
	memcpy(serverInfo, &(itMapInfo->second.serverInfo), sizeof(ST_FORWARD_SERVER_INFO));
	memcpy(&(serverInfo->szTicket), ticket, sizeof(serverInfo->szTicket));
	serverInfo->cerror = 1;
	//pthread_mutex_unlock(&g_MutexForwardInfo);

	Json::Value root;
	root["ticket"] = ticket;
	unsigned int jsonlen;
	string strTicket = root.toStyledString();
	char sendBuff[512]={0};
	char recvBuff[512]={0};
	itMap = g_MapTicketInfo.begin();
	traceEvent( TRACE_DEBUG,"REQUEST_FORWARD_INFO g_MapForwardInfo.size()=%d,g_MapTicketInfo.size()=%d" ,g_MapForwardInfo.size(),itMap->second.size());
	ST_RECV_PACKAGE_HEAD head = {0};
	head.usOpCode = htons(NOTIFY_TICKET_INFO);
	head.usPackageLens = htons(sizeof(head) +  strTicket.size() + sizeof(jsonlen));
	head.usVersion  = htons(SERVER_VERSION);
	jsonlen = strTicket.size();
	jsonlen = htonl(jsonlen);
	memcpy(sendBuff, &head, sizeof(head));
	memcpy(sendBuff + sizeof(head), &jsonlen, sizeof(unsigned int));
	memcpy(sendBuff + sizeof(head) + sizeof(unsigned int), strTicket.c_str(), strTicket.size());
	size = OS_SockSend(forwardsock, sendBuff, ntohs(head.usPackageLens));
	if(0 != size )
	{
		traceEvent( TRACE_DEBUG,"REQUEST_FORWARD_INFO OS_SockSend eoorr =  %s" ,strerror(errno));
		return -1;
	}
	/*traceEvent( TRACE_DEBUG,"OS_SockSend to fwd server,size=%d ",size);
	ST_RTN_PACKAGE_HEAD * pRtnHead = (ST_RTN_PACKAGE_HEAD *)recvBuff;
	size = OS_SockRecv(forwardsock,recvBuff,sizeof(ST_RTN_PACKAGE_HEAD));
	traceEvent( TRACE_DEBUG,"recv error size=%d",size);
	if(-1 == size)
	{		
		FD_CLR(forwardsock, &socket_mask);
		ClearTicketInfo(forwardsock);
		close(forwardsock);
		return -1;
	}
	else if(0 == size)
	{
		fwd_printhex(recvBuff, 8);
		if(ntohs(pRtnHead->usOpCode) == NOTIFY_TICKET_INFO && ntohs(pRtnHead->usRtnCode)==0)
		{
			map <SOCKET, list<string> >::iterator itTicketMap ;
			//pthread_mutex_lock(&g_MutexTicket);
			itTicketMap = g_MapTicketInfo.find(forwardsock);
			if(itTicketMap != g_MapTicketInfo.end())
			{
				(itTicketMap->second).push_back(string(ticket));
			}
			//pthread_mutex_unlock(&g_MutexTicket);
		}
		else
		{
			traceEvent( TRACE_DEBUG,"CMD error cmd=%d,rtncode=%d ",ntohs(pRtnHead->usOpCode),ntohs(pRtnHead->usRtnCode));
			size = OS_SockRecv(forwardsock,recvBuff,ntohs(pRtnHead->usPackageLens)-sizeof(ST_RTN_PACKAGE_HEAD));
			if(-1 == size)
			{		
				FD_CLR(forwardsock, &socket_mask);
				ClearTicketInfo(forwardsock);
				close(forwardsock);
				return -1;
			}
			return -1;
		}
	}
	else
	{
		traceEvent( TRACE_DEBUG,"recv error size=%d",size);
		return -1;
	}
	size = OS_SockRecv(forwardsock,recvBuff,ntohs(pRtnHead->usPackageLens)-sizeof(ST_RTN_PACKAGE_HEAD));
	if(-1 == size)
	{		
		FD_CLR(forwardsock, &socket_mask);
		ClearTicketInfo(forwardsock);
		close(forwardsock);
		return -1;
	}
	fwd_printhex(recvBuff, 8);*/
	return 0;
}

static int process_udp( n2n_sn_t * sss, 
                        const struct sockaddr_in * sender_sock,
                        const uint8_t * udp_buf, 
                        size_t udp_size,
                        time_t now)
{
     size_t              msg_type; 
    macstr_t            mac_buf; 
    n2n_sock_str_t      sockbuf;
    n2n_sock_t          sock; 
    p2p_head *head;
    int data_sent_len;


    //traceEvent( TRACE_DEBUG, "process_udp(%lu)", udp_size );

   head=( p2p_head *)udp_buf;

    // ��ݴ�С��һ�� 
   if(ntohs(head->datasize)!=udp_size)
   {
	traceEvent( TRACE_ERROR,"datasize errorhead->datasize=%d,udp_size=%d",ntohs(head->datasize),udp_size );
        return -1;
   }
   msg_type=ntohs(head->cmd);

   //������Ϣ
   if(msg_type==MSG_P2P_REGISTER)
   {
   		//traceEvent( TRACE_DEBUG, "sizeof(struct xcloud_p2p_register)=(%d)", sizeof(struct xcloud_p2p_register));
   		if((udp_size - (sizeof(p2p_head))) == sizeof(struct xcloud_p2p_register) )
   		{
   			//traceEvent( TRACE_DEBUG, "%s", "no uuid");
			struct xcloud_p2p_register *p2p_register;
			p2p_register=(struct xcloud_p2p_register *)(udp_buf+sizeof(p2p_head));   
			traceEvent( TRACE_DEBUG, "lantcp=%d,lanudp=%d,pubudp=%d,pubtcp=%d", ntohs(p2p_register->device.lan_tcp_port),ntohs(p2p_register->device.lan_udp_port),ntohs(p2p_register->device.public_udp_port),ntohs(p2p_register->device.upnp_tcp_port));

	        /* Edge requesting registration with us.  */
	        
	        sss->stats.last_reg_super=now;
	        ++(sss->stats.reg_super);

	        sock.family = AF_INET;
	        sock.port = ntohs(sender_sock->sin_port);
	        memcpy( sock.addr.v4, &(sender_sock->sin_addr.s_addr), IPV4_SIZE );       
	  
	        //traceEvent( TRACE_DEBUG, "Rx P2P_REGISTER for %s [%s]",
	        //            macaddr_str( mac_buf, p2p_register->id ),
	        //            sock_to_cstr( sockbuf, &(sock) ) );
	        
	        update_edge( p2p_register,sss, (uint8_t *)p2p_register->id, (uint8_t *)p2p_register->username, &sock, now );      
   		}
		else
		{
			//traceEvent( TRACE_DEBUG, "%s", "have uuid");
			struct xcloud_p2p_register_uuid *p2p_register;
			p2p_register=(struct xcloud_p2p_register_uuid *)(udp_buf+sizeof(p2p_head));   
			traceEvent( TRACE_DEBUG, "lantcp=%d,lanudp=%d,pubudp=%d,pubtcp=%d", ntohs(p2p_register->device.lan_tcp_port),ntohs(p2p_register->device.lan_udp_port),ntohs(p2p_register->device.public_udp_port),ntohs(p2p_register->device.upnp_tcp_port));
                        //traceEvent( TRACE_DEBUG, "Rx P2P_REGISTER for %s [%s]",macaddr_str( mac_buf, p2p_register->id ),sock_to_cstr( sockbuf, &(sock) ) );
	        /* Edge requesting registration with us.  */
	        traceEvent( TRACE_DEBUG, "uuid = %s", p2p_register->uuid);
                //traceEvent( TRACE_DEBUG, "mac = %s", p2p_register->id);
	        sss->stats.last_reg_super=now;
	        ++(sss->stats.reg_super);

	        sock.family = AF_INET;
	        sock.port = ntohs(sender_sock->sin_port);
	        memcpy( sock.addr.v4, &(sender_sock->sin_addr.s_addr), IPV4_SIZE );       
	  
	       // traceEvent( TRACE_DEBUG, "Rx P2P_REGISTER for %s [%s]",
	         //           macaddr_str( mac_buf, p2p_register->id ),
	           //         sock_to_cstr( sockbuf, &(sock) ) );
	        
	        update_edge_uuid( p2p_register,sss,(uint8_t *)p2p_register->id, (uint8_t *)p2p_register->username, &sock, now );
		}

   }
   
   else if (msg_type==MSG_P2P_GET)
   {
   	size_t send_size;
   	 uint8_t pktbuf[N2N_SN_PKTBUF_SIZE];
   	struct mobile_p2p_get *p2p_get;
   	struct p2p_server_respond *server_respond;
   	p2p_get=(struct  mobile_p2p_get *)(udp_buf+sizeof(p2p_head));   
   	traceEvent(TRACE_DEBUG, "p2p_get->usename=%s", p2p_get->username);
	send_size=process_p2p_ack(sss,(uint8_t *)p2p_get->username,pktbuf+sizeof(p2p_head)+6);
	traceEvent(TRACE_DEBUG, "send_size =%d ", send_size);
	if(send_size==0)
	{
		traceEvent(TRACE_DEBUG, "%s","no device");
		head->datasize=ntohs(sizeof(p2p_head));
		head->cmd=ntohs(MSG_P2P_ACK);
		head->version=ntohs(SERVER_VERSION);
		sock.family = AF_INET;
        sock.port = ntohs(sender_sock->sin_port);
        memcpy( sock.addr.v4, &(sender_sock->sin_addr.s_addr), IPV4_SIZE );
		memcpy(pktbuf, head, sizeof(p2p_head));
		memcpy(pktbuf + sizeof(p2p_head), sock.addr.v4 , 4);
		memcpy(pktbuf + sizeof(p2p_head) +4 , &(sender_sock->sin_port) , 2);
		sendto_sock( sss, &sock, pktbuf, sizeof(p2p_head)+6);
		return 0;
		
	}
	
	//���¹���ACK��ͷ
	head=( p2p_head *)pktbuf;
	send_size=send_size+sizeof(p2p_head)+6;
	head->datasize=ntohs(send_size);
	head->cmd=ntohs(MSG_P2P_ACK);
	head->version=ntohs(SERVER_VERSION);

	server_respond=(struct  p2p_server_respond*)(pktbuf+sizeof(p2p_head)); 
	memcpy(server_respond->my_public_ip,&(sender_sock->sin_addr.s_addr), IPV4_SIZE ); 
	server_respond->my_public_udp_port=sender_sock->sin_port;	
	
	
	
	 sock.family = AF_INET;
        sock.port = ntohs(sender_sock->sin_port);
        memcpy( sock.addr.v4, &(sender_sock->sin_addr.s_addr), IPV4_SIZE ); 
        
        data_sent_len = sendto_sock( sss, &sock, pktbuf, send_size);
         if ( data_sent_len == send_size )
        {
            ++(sss->stats.fwd);
            traceEvent(TRACE_DEBUG, "MSG_P2P_GET %lu to [%s] ",
                       send_size,
                       sock_to_cstr( sockbuf, &sock ));
        }
        else
        {
            ++(sss->stats.errors);
            traceEvent(TRACE_ERROR, "MSG_P2P_GET %lu to [%s]  FAILED (%d: %s)",
                       send_size,
                       sock_to_cstr( sockbuf,  &sock ),                      
                       errno, strerror(errno) );
        }

   }
   //�����
    else if (msg_type==MSG_P2P_HOLE_PUNCH)
   {
   	
   	struct p2p_hole_punch *hole_punch;   	
   	hole_punch=(struct p2p_hole_punch *)(udp_buf+sizeof(p2p_head));  
   	hole_punch->from_server=1; //�����ת��

   	sock.family = AF_INET;
       sock.port = ntohs(hole_punch->dest_public_udp_port);
       memcpy( sock.addr.v4, hole_punch->dest_public_ip, IPV4_SIZE );   
   
        data_sent_len = sendto_sock( sss, &sock, udp_buf, udp_size);

        if ( data_sent_len == udp_size )
        {
            ++(sss->stats.fwd);
            traceEvent(TRACE_DEBUG, "MSG_P2P_HOLE_PUNCH %lu to [%s] ",
                       udp_size,
                       sock_to_cstr( sockbuf, &sock ));
        }
        else
        {
            ++(sss->stats.errors);
            traceEvent(TRACE_ERROR, "MSG_P2P_HOLE_PUNCH %lu to [%s]  FAILED (%d: %s)",
                       udp_size,
                       sock_to_cstr( sockbuf,  &sock ),                      
                       errno, strerror(errno) );
        }
   	
   	

   }
     //��Ӧ����
    else if (msg_type==MSG_P2P_HOLE_PUNCH_ACK)
   {
   	
   	struct p2p_hole_ack *hole_ack;   	
   	hole_ack=(struct p2p_hole_ack *)(udp_buf+sizeof(p2p_head));  
   	hole_ack->from_server=1; //�����ת��

   	sock.family = AF_INET;
       sock.port =ntohs( hole_ack->dest_public_udp_port);
       memcpy( sock.addr.v4, hole_ack->dest_public_ip, IPV4_SIZE );   
   
        data_sent_len = sendto_sock( sss, &sock, udp_buf, udp_size);

        if ( data_sent_len == udp_size )
        {
            ++(sss->stats.fwd);
            traceEvent(TRACE_DEBUG, "MSG_P2P_HOLE_PUNCH_ACK %lu to [%s] ",
                       udp_size,
                       sock_to_cstr( sockbuf, &sock ));
        }
        else
        {
            ++(sss->stats.errors);
            traceEvent(TRACE_ERROR, "unicast %lu to [%s]  FAILED (%d: %s)",
                       udp_size,
                       sock_to_cstr( sockbuf,  &sock ),                      
                       errno, strerror(errno) );
        }

   }

	//С�Ʒ����ȡp2p��Ϣ
	else if (msg_type==SHARE_P2P_DEVINFO)
   {
   	size_t send_size;
   	uint8_t pktbuf[N2N_SN_PKTBUF_SIZE];
   	SHARE_P2P_GET_DEV_INFO *p2p_get;
   	struct p2p_server_respond *server_respond;
   	p2p_get=(SHARE_P2P_GET_DEV_INFO *)(udp_buf+sizeof(p2p_head));  
	traceEvent(TRACE_DEBUG, "SHare get size= %d", udp_size);
   	traceEvent(TRACE_DEBUG, "%s", "SHare get p2p info");
	traceEvent(TRACE_DEBUG, "SHare get uuid= %s", p2p_get->szUserPcUuid);
	show_uuid(sss->edges);
	send_size=process_share_p2p_ack(sss,p2p_get->szUserPcUuid, pktbuf+sizeof(p2p_head)+6);
	traceEvent( TRACE_DEBUG, "Send_size= %d", send_size);
	if(send_size==0)
		return -1;

	
	//���¹���ACK��ͷ
	head=( p2p_head *)pktbuf;
	send_size=send_size+sizeof(p2p_head)+6;
	head->datasize=ntohs(send_size);
	head->cmd=ntohs(SHARE_P2P_DEVINFO);
	head->version=ntohs(SERVER_VERSION);

	server_respond=(struct  p2p_server_respond*)(pktbuf+sizeof(p2p_head)); 
	memcpy(server_respond->my_public_ip,&(sender_sock->sin_addr.s_addr), IPV4_SIZE ); 
	server_respond->my_public_udp_port=sender_sock->sin_port;	
	
	
	
	 sock.family = AF_INET;
        sock.port = ntohs(sender_sock->sin_port);
        memcpy( sock.addr.v4, &(sender_sock->sin_addr.s_addr), IPV4_SIZE ); 
        
        data_sent_len = sendto_sock( sss, &sock, pktbuf, send_size);
         if ( data_sent_len == send_size )
        {
            ++(sss->stats.fwd);
            traceEvent(TRACE_DEBUG, "MSG_P2P_GET %lu to [%s] ",
                       send_size,
                       sock_to_cstr( sockbuf, &sock ));
        }
        else
        {
            ++(sss->stats.errors);
            traceEvent(TRACE_ERROR, "MSG_P2P_GET %lu to [%s]  FAILED (%d: %s)",
                       send_size,
                       sock_to_cstr( sockbuf,  &sock ),                      
                       errno, strerror(errno) );
        }

   }
	 else if (msg_type==REQUEST_FORWARD_INFO)
   {
   		
   		traceEvent( TRACE_NORMAL, "%s,count=%d", "cmd REQUEST_FORWARD_INFO",count++);
		//return 0;
	   	struct peer_info *pPeer;
		int rtn= 0;
		unsigned short send_size;
		n2n_sock_t pcSock = {0};
		p2p_head *rtnHead = NULL;
		char pktbuf[N2N_PKT_BUF_SIZE] = {0};
		char ticket[17]= {0};
		char * uuid = NULL;
		ST_FORWARD_SERVER_INFO serverInfo = {0};
		//traceEvent( TRACE_NORMAL, "size111=%d,udp_size=%d", ntohs(head->datasize),udp_size);
		uuid = (char *)udp_buf + sizeof(p2p_head); 
		//uuid = uuid + sizeof(p2p_head);
		traceEvent(TRACE_DEBUG, "REQUEST_FORWARD_INFO uuid = %s", uuid);
		rtnHead=( p2p_head *)pktbuf;
		send_size=sizeof(p2p_head) + sizeof(ST_FORWARD_SERVER_INFO);
		rtnHead->datasize=ntohs(send_size);
		rtnHead->cmd=ntohs(REQUEST_FORWARD_INFO);
		rtnHead->version=ntohs(SERVER_VERSION);
		//serverInfo.cerror = 1;
		//memcpy(pktbuf + sizeof(p2p_head), &serverInfo, sizeof(serverInfo));
		//goto voer;
	   	pPeer = find_peer_by_uuid(sss->edges, uuid);
		if(pPeer == NULL)
		{
			traceEvent(TRACE_DEBUG, "REQUEST_FORWARD_INFO uuid=%s not find", uuid);
			serverInfo.cerror = '%';
			//memcpy(pktbuf + sizeof(p2p_head), &serverInfo, sizeof(serverInfo));
			memcpy(pktbuf + sizeof(p2p_head), &serverInfo, sizeof(serverInfo));
			sock.family = AF_INET;
			sock.port = ntohs(sender_sock->sin_port);
			memcpy( sock.addr.v4, &(sender_sock->sin_addr.s_addr), IPV4_SIZE );
			/*traceEvent(TRACE_DEBUG, "sock  info %lu to [%s] ",
		                       udp_size,
		                       sock_to_cstr( sockbuf, &sock ));*/
			data_sent_len = sendto_sock( sss, &sock, (uint8_t *)pktbuf, send_size);

			if ( data_sent_len == send_size)
			{
			  ++(sss->stats.fwd);
			  /*traceEvent(TRACE_DEBUG, "send to phone %lu to [%s] ",
			             udp_size,
			             sock_to_cstr( sockbuf, &sock ));*/
			}
			else
			{
			  ++(sss->stats.errors);
			  /*traceEvent(TRACE_ERROR, "MSG_P2P_HOLE_PUNCH %lu to [%s]  FAILED (%d: %s)",
			             udp_size,
			             sock_to_cstr( sockbuf,  &sock ),                      
			             errno, strerror(errno) );*/
			}
		}
		else
		{
			GenerateTicket(ticket);
			traceEvent(TRACE_DEBUG, "REQUEST_FORWARD_INFO ticket = %s", ticket);
			rtn = ChoseForwardServer(&serverInfo, ticket);
			if(rtn == 0)
			{

				ST_PEER_INFO fwd_peer_info={0};
				memcpy(fwd_peer_info.szTicket,ticket,16);
				memcpy(&(fwd_peer_info.fwd_info), &serverInfo, sizeof(serverInfo));
				
				pcSock.family = AF_INET;
				pcSock.port = ntohs(pPeer->device.public_udp_port);

				memcpy( pcSock.addr.v4, &(pPeer->device.public_ip), IPV4_SIZE ); 
		        memcpy(&(fwd_peer_info.pcSock), &pcSock, sizeof(pcSock));

				sock.family = AF_INET;
		  		sock.port = ntohs(sender_sock->sin_port);
		  		memcpy( sock.addr.v4, &(sender_sock->sin_addr.s_addr), IPV4_SIZE );
				memcpy(&(fwd_peer_info.phoneSock), &sock, sizeof(sock));
				fwd_peer_info.fd = sss->sock;
				fwd_peer_info.fwd_info.cerror=0;
				g_MapPeerInfo.insert(make_pair(ticket, fwd_peer_info));
				
				 /*data_sent_len = sendto_sock( sss, &pcSock, (uint8_t *)pktbuf, send_size);		
				if ( data_sent_len == send_size )
				{
				    ++(sss->stats.fwd);
				    traceEvent(TRACE_DEBUG, "REQUEST_FORWARD_INFO send to pc  %lu to [%s] ",
					       udp_size,
					       sock_to_cstr( sockbuf, &sock ));
				}
				else
				{
				    ++(sss->stats.errors);
				    traceEvent(TRACE_ERROR, "send to pc %lu to [%s]  FAILED (%d: %s)",
					       udp_size,
					       sock_to_cstr( sockbuf,  &sock ),                      
					       errno, strerror(errno) );
				}*/
			}
			else
			{
				serverInfo.cerror = 1;
				memcpy(pktbuf + sizeof(p2p_head), &serverInfo, sizeof(serverInfo));
				sock.family = AF_INET;
			  sock.port = ntohs(sender_sock->sin_port);
			  memcpy( sock.addr.v4, &(sender_sock->sin_addr.s_addr), IPV4_SIZE );
			/*traceEvent(TRACE_DEBUG, "sock  info %lu to [%s] ",
		                       udp_size,
		                       sock_to_cstr( sockbuf, &sock ));*/
			  data_sent_len = sendto_sock( sss, &sock, (uint8_t *)pktbuf, send_size);

			  if ( data_sent_len == send_size)
			  {
			      ++(sss->stats.fwd);
			      /*traceEvent(TRACE_DEBUG, "send to phone %lu to [%s] ",
			                 udp_size,
			                 sock_to_cstr( sockbuf, &sock ));*/
			  }
			  else
			  {
			      ++(sss->stats.errors);
			      /*traceEvent(TRACE_ERROR, "MSG_P2P_HOLE_PUNCH %lu to [%s]  FAILED (%d: %s)",
			                 udp_size,
			                 sock_to_cstr( sockbuf,  &sock ),                      
			                 errno, strerror(errno) );*/
			  }
			}
			
		}

	//voer:
		  
 	}
 
  else 
 {
 traceEvent( TRACE_DEBUG, "error cmd %x", head->cmd);
 }
   	



    return 0;
}


/** Help message to print if the command line arguments are not valid. */
static void exit_help(int argc, char * const argv[])
{
    fprintf( stderr, "%s usage\n", argv[0] );
    fprintf( stderr, "-l <lport>\tSet UDP main listen port to <lport>\n" );

#if defined(N2N_HAVE_DAEMON)
    fprintf( stderr, "-f        \tRun in foreground.\n" );
#endif /* #if defined(N2N_HAVE_DAEMON) */
    fprintf( stderr, "-v        \tIncrease verbosity. Can be used multiple times.\n" );
    fprintf( stderr, "-h        \tThis help message.\n" );
    fprintf( stderr, "\n" );
    exit(1);
}

static int run_loop( n2n_sn_t * sss );

int OS_SockRecv(SOCKET sock, char *pBuff, const size_t isize)
{
	int Rtn = 0;
    if (-1 == sock)
    {
        return 1;
    }

    if (0 == isize)
    {
        return 0;
    }
    
    int iRecvSize = 0;
    int iCount = 0;

    do 
    {
        iRecvSize = recv(sock, pBuff + iCount, isize - iCount, 0);
        if (0 > iRecvSize)
        {
            int iRtn = 0;
            Rtn = 1;
            break;
        }

        if (0 == iRecvSize)
        {
            int iRtn = 0;
            Rtn = -1;

            break;
        }

        iCount += iRecvSize;

    } while (iCount < isize);

    if (isize != iCount)
    {
        return Rtn;
    }

    return 0;
}


int OS_SockSend(SOCKET &sock, char *pBuff, const size_t isize)
{
    if(0 == sock)
    {
        return 1;
    }

    if (NULL == pBuff)
    {
        return 1;
    }

    if (0 == isize)
    {
        return 0;
    }

    int start = 0;
    int sended = 0;

    do
    {
        int len =send(sock, pBuff + sended, isize - sended, 0);
        if(0 > len)
        {
        	if(errno==EAGAIN) /* EAGAIN : Resource temporarily unavailable*/   
            { 
                sleep(1);
                continue;  
                 
            }
            break;
        }

        sended += len;

    }while(sended < isize);

    if (sended != isize)
    {
        return 1;
    }

    return 0;
}


unsigned long OS_SockIpToNumber(const string &strIp)
{
    return inet_addr(strIp.c_str());
}

string OS_SockNumberToIp(const unsigned long &ulIp)
{
    char acIP[64] = {0};

    unsigned int ip = (unsigned int)ulIp;

#ifdef WIN32
    snprintf(acIP, sizeof(acIP) - 1, "%u.%u.%u.%u",
        ip&0xFF, (ip>>8)&0xFF, (ip>>16)&0xFF, (ip>>24)&0xFF);
#else
    snprintf(acIP, sizeof(acIP) - 1, "%u.%u.%u.%u",
        ip&0xFF, (ip>>8)&0xFF, (ip>>16)&0xFF, (ip>>24)&0xFF);
#endif

    return string(acIP);
}


static int Init_tcpSock(int tcpSock)
{
	SOCKET sock_fd;
	struct sockaddr_in local_address;
	int sockopt = 1;

	sock_fd = socket(PF_INET, SOCK_STREAM, 0);
	if(sock_fd  < 0) {
	  traceEvent(TRACE_ERROR, "Unable to create socket [%s][%d]\n",
	       strerror(errno), sock_fd);
	  return(-1);
	}

#ifndef WIN32
	 fcntl(sock_fd, F_SETFL, O_NONBLOCK); 
#endif

	setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&sockopt, sizeof(sockopt));

	memset(&local_address, 0, sizeof(local_address));
	local_address.sin_family = AF_INET;
	local_address.sin_port = htons(N2N_SN_FORWARD_PORT_DEFAULT);
	local_address.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(sock_fd, (struct sockaddr*) &local_address, sizeof(local_address)) == -1) {
	  traceEvent(TRACE_ERROR, "Bind error [%s]\n", strerror(errno));
	  return(-1);
	}
	if((listen(sock_fd, 32)) == -1) {
	  traceEvent(TRACE_ERROR, "listen [%s]\n", strerror(errno));
	  return(-1);
	}
	 traceEvent(TRACE_ERROR, "socket = %d", sock_fd);
	return(sock_fd);	
}


void  ClearTimeoutFWDServer(  )
{
  static time_t last_fwd_tm = 0;
  time_t now = time(NULL);

  if((now - last_fwd_tm) < PURGE_REGISTRATION_FREQUENCY) 
	{
		return;
	}
	map <SOCKET, ST_FORWARD_SERVER_INFO_INMAP>::iterator itMap ;
	map <SOCKET, list<string> >::iterator itTicketMap ;
	list <string> listTicket;
	pthread_mutex_lock(&g_MutexForwardInfo);
	for(itMap = g_MapForwardInfo.begin(); itMap != g_MapForwardInfo.end();)
	{
		if(now - itMap->second.regTM  > FWD_TIMEOUT)
		{
			//pthread_mutex_lock(&g_MutexTicket);
	
			itTicketMap = g_MapTicketInfo.find(itMap->first);
			if(itTicketMap != g_MapTicketInfo.end())
			{
				g_MapTicketInfo.erase(itTicketMap);
				
			}
			//pthread_mutex_unlock(&g_MutexTicket);
			
			FD_CLR(itMap->first, &socket_mask);
			//ClearTicketInfo(forwardsock);
			close(itMap->first);
		
			g_MapForwardInfo.erase(itMap++);
						
		}
		else
		{
			itMap++;
		}
	}
	return;
}

void fwd_printhex(char *s, int len)
{	int   n, ret;	
	char buf[64], *p;	
	n = 16;	p = buf;	
	while (len > 0) 
	{		
		if ( n == 0 ) 
		{			
			traceEvent( TRACE_INFO, "%s", buf);			
			n = 16;			
			p = buf;			
			continue;		
		}		
		if (n == 8)			
			ret = snprintf(p, 64, "  %02x", *s & 0xff);		
		else			
			ret = snprintf(p, 64, " %02x",  *s & 0xff);		
		n--;		
		p += ret;		
		s++;		
		len --;	
	}	
	traceEvent( TRACE_INFO, "%s", buf);

}


void process_tcp_mesg(int fd)
{
	int iRtn = 0;
	ST_RECV_PACKAGE_HEAD *recvHead = NULL;
	ST_RTN_PACKAGE_HEAD *rHead = NULL;
	unsigned short  rtnLen = 0;
	char *pRtnData = NULL;
	char * pData = NULL;
	char * pTmpData = NULL;
	int dataLen;
	Json::Reader reader;
	Json::Value  value;
	map <SOCKET, ST_FORWARD_SERVER_INFO_INMAP>::iterator itMap ;
	map <SOCKET, list<string> >::iterator itTicketMap ;
	map <string, ST_PEER_INFO>::iterator itPeerInfo;
	list<string>::iterator itStr;
	
	char headBuff[10]= {0};
	iRtn = OS_SockRecv(fd, headBuff, sizeof(ST_RECV_PACKAGE_HEAD));
	fwd_printhex(headBuff, 8);
	if(0 != iRtn)
	{
		traceEvent( TRACE_ERROR, "error  rtn=%d", iRtn);
		if(-1 == iRtn)
		{		
			FD_CLR(fd, &socket_mask);
			ClearTicketInfo(fd);
			close(fd);
			return;
		}
	}
	recvHead = (ST_RECV_PACKAGE_HEAD *)headBuff;
	recvHead->usOpCode = ntohs(recvHead->usOpCode);
	recvHead->usPackageLens= ntohs(recvHead->usPackageLens);
	recvHead->usVersion= ntohs(recvHead->usVersion);
	traceEvent( TRACE_NORMAL, "recv TCP code=%u, len=%u,", recvHead->usOpCode,  recvHead->usPackageLens);
	dataLen = recvHead->usPackageLens - sizeof(ST_RECV_PACKAGE_HEAD);
	if(dataLen > 0)
	{
		pTmpData = (char *)malloc(dataLen);
		memset(pTmpData,0, dataLen);
		if(pTmpData == NULL)
		{
			traceEvent( TRACE_ERROR, "%s", "error malloc");
		}
		iRtn = OS_SockRecv(fd, pTmpData, dataLen);
		
		if(0 != iRtn)
		{
			traceEvent( TRACE_ERROR, "error  rtn=%d", iRtn);
			if(-1 == iRtn)
			{				
				FD_CLR(fd, &socket_mask);
				ClearTicketInfo(fd);
				close(fd);
				return;
			}
		}
		pData = pTmpData + 4;
		fwd_printhex(pTmpData, dataLen);
		int len = ntohl(*((int *)pTmpData));
		traceEvent( TRACE_NORMAL, "strlen = %d", len);
	}
	switch(recvHead->usOpCode)
	{
		case REGIST_FORWARD_INFO:
			if(pData == NULL)
			{
				traceEvent( TRACE_NORMAL, "%s", "data error!");
				return;
			}
			traceEvent( TRACE_NORMAL, "%s", "cmd REGIST_FORWARD_INFO");
			traceEvent( TRACE_NORMAL, "pdata = %s", pData);
			
			if(reader.parse(pData,value))
			{
				ST_RTN_PACKAGE_HEAD rtnHead = {0};
				string fwd_ip = value["fwd_addr"].asString();
				traceEvent( TRACE_NORMAL, "ip= %s", fwd_ip.c_str());
				unsigned short fwd_port = (unsigned short)value["fwd_port"].asUInt();
				ST_FORWARD_SERVER_INFO_INMAP forward_info;
				list <string> listTicket;
				forward_info.serverInfo.uiServerIp = (unsigned int)OS_SockIpToNumber(fwd_ip);
				traceEvent( TRACE_NORMAL, "ipint = %d,ip=%s,", forward_info.serverInfo.uiServerIp, (OS_SockNumberToIp(htonl(forward_info.serverInfo.uiServerIp))).c_str());
				forward_info.serverInfo.usServerPort = fwd_port;
				traceEvent( TRACE_NORMAL, "port= %d", forward_info.serverInfo.usServerPort );
				//forward_info.serverInfo.uiServerIp = htonl(forward_info.serverInfo.uiServerIp);
				forward_info.serverInfo.usServerPort = htons(forward_info.serverInfo.usServerPort);
				forward_info.regTM = time(0);
				//pthread_mutex_lock(&g_MutexForwardInfo);

				itMap = g_MapForwardInfo.find(fd);
				if(itMap == g_MapForwardInfo.end())
				{
					traceEvent( TRACE_NORMAL, "%s", "not find ");
					g_MapForwardInfo.insert(make_pair(fd, forward_info));
				}
				//pthread_mutex_unlock(&g_MutexForwardInfo);

				//pthread_mutex_lock(&g_MutexTicket);
				itTicketMap = g_MapTicketInfo.find(fd);
				if(itTicketMap == g_MapTicketInfo.end())
				{
					traceEvent( TRACE_NORMAL, "%s", "not find ");
					g_MapTicketInfo.insert(make_pair(fd, listTicket));
				}
				//pthread_mutex_unlock(&g_MutexTicket);
				
				rtnLen=sizeof(rtnHead);
				traceEvent( TRACE_NORMAL, "rtnLen11= %d", rtnLen);
				pRtnData = (char *)malloc(rtnLen);
				{
					if(pRtnData == NULL)
					{
						traceEvent( TRACE_ERROR, "%s", "error malloc");
						return;
					}
				}
				rtnHead.usOpCode = htons(REGIST_FORWARD_INFO);
				rtnHead.usPackageLens = htons(rtnLen);
				rtnHead.usRtnCode = htons(0);
				memcpy(pRtnData, &rtnHead, rtnLen);
				traceEvent( TRACE_NORMAL, "rtnLen222= %d", rtnLen);
			}
		break;

		case REQUEST_RELEASE_TICKET:
			if(pData == NULL)
			{
				traceEvent( TRACE_NORMAL, "%s", "data error!");
				return;
			}
			traceEvent( TRACE_NORMAL, "%s", "cmd REQUEST_RELEASE_TICKET");
			traceEvent( TRACE_NORMAL, "pData= %s", pData);	
			if(reader.parse(pData,value))
			{
				ST_RTN_PACKAGE_HEAD rtnHead = {0};
				string strTicket = value["ticket"].asString();
				traceEvent( TRACE_NORMAL, "ticket= %s", strTicket.c_str());
				ST_FORWARD_SERVER_INFO_INMAP forward_info;
				//pthread_mutex_lock(&g_MutexTicket);

				itTicketMap = g_MapTicketInfo.find(fd);
				if(itTicketMap != g_MapTicketInfo.end())
				{
					for(itStr = itTicketMap->second.begin(); itStr != itTicketMap->second.end();)
					{
						if(*itStr == strTicket)
						{
							itTicketMap->second.erase(itStr++);
						}
						else
						{
							itStr++;
						}
					}
				}
				//pthread_mutex_unlock(&g_MutexTicket);
				rtnLen=sizeof(rtnHead);

				pRtnData = (char *)malloc(rtnLen);
				{
					if(pRtnData == NULL)
					{
						traceEvent( TRACE_ERROR, "%s", "error malloc");
						return;
					}
				}
				rtnHead.usOpCode = htons(REQUEST_RELEASE_TICKET);
				rtnHead.usPackageLens = htons(rtnLen);
				rtnHead.usRtnCode = htons(0);
				memcpy(pRtnData, &rtnHead, rtnLen);
			}
			break;
		case NOTIFY_TICKET_INFO:

			if(pData == NULL)
			{
				traceEvent( TRACE_NORMAL, "%s", "data error!");
				return;
			}
			traceEvent( TRACE_NORMAL, "%s", "cmd NOTIFY_TICKET_INFO");
			traceEvent( TRACE_NORMAL, "pData= %s", pData);
			
			if(reader.parse(pData,value))
			{
				ST_RTN_PACKAGE_HEAD rtnHead = {0};
				string strTicket = value["ticket"].asString();
				traceEvent( TRACE_NORMAL, "ticket= %s", strTicket.c_str());

				rHead = (ST_RTN_PACKAGE_HEAD *)headBuff;
				itPeerInfo = g_MapPeerInfo.find(strTicket);
				if(0 == ntohs(rHead->usRtnCode))
				{
					
					if(itPeerInfo != g_MapPeerInfo.end())
					{
						SendFwdInfoToPeer(&(itPeerInfo->second));
						traceEvent( TRACE_NORMAL, "%s", "SendFwdInfoToPeer ok");
						itTicketMap = g_MapTicketInfo.find(fd);
						if(itTicketMap != g_MapTicketInfo.end())
						{
							(itTicketMap->second).push_back(strTicket);
						}
					}
					else
					{
						traceEvent( TRACE_NORMAL, "%s", "not find peer info");
						
					}

					//pthread_mutex_lock(&g_MutexTicket);
					
				//pthread_mutex_unlock(&g_MutexTicket);
				}
				if(itPeerInfo != g_MapPeerInfo.end())
				{
					g_MapPeerInfo.erase(itPeerInfo);
				}
			}
			
						
			break;

		case FWD_TO_CM_HEARTBEAT:
			//pthread_mutex_lock(&g_MutexForwardInfo);
			itMap = g_MapForwardInfo.find(fd);
			if(itMap != g_MapForwardInfo.end())
			{
				//traceEvent( TRACE_NORMAL, "%s", "not find ");
				itMap->second.regTM = time(0);
			}
			//pthread_mutex_unlock(&g_MutexForwardInfo);
			break;
		default:
			traceEvent( TRACE_NORMAL, "%s", "cmd UNKNOW");
		break;		
	}
	//traceEvent( TRACE_NORMAL, "rtnLen = %d",rtnLen);
	if(pRtnData != NULL && rtnLen > 0)
	{
		
		OS_SockSend(fd, pRtnData, rtnLen);
		traceEvent( TRACE_NORMAL, "%s", "free111111..");
		free(pRtnData);
	}
	if(pTmpData != NULL)
	{
		traceEvent( TRACE_NORMAL, "%s", "free222222222..");
		free(pTmpData);
	}
	return;
}

/* *********************************************** 

static const struct option long_options[] = {
  { "foreground",      no_argument,       NULL, 'f' },
  { "local-port",      required_argument, NULL, 'l' },
  { "help"   ,         no_argument,       NULL, 'h' },
  { "verbose",         no_argument,       NULL, 'v' },
  { NULL,              0,                 NULL,  0  }
};*/

static int t_reset_rlimit()
{
	struct rlimit rl, old; 

	if (getrlimit(RLIMIT_CORE, &rl) == 0) {
		printf("getrlimit RLIMIT_CORE (cur: %ld, max: %ld)", rl.rlim_cur, rl.rlim_max);
		rl.rlim_cur = rl.rlim_max;
		if (setrlimit(RLIMIT_CORE, &rl))
			printf("set RLIMIT_CORE failed, try run this by root: %s", strerror(errno));
	} else
		printf("get RLIMIT_CORE failed, try run this by root! error: %s", strerror(errno));

	return 0;
}

/** Main program entry point from kernel. */
int main( int argc, char * const argv[] )
{
    n2n_sn_t sss;
	srand(time(0));

	t_reset_rlimit();

	pthread_mutex_init(&g_MutexForwardInfo, NULL);
	pthread_mutex_init(&g_MutexTicket, NULL);
    init_sn( &sss );
    sss.daemon = 0; /* foreground */
  // printf("sizeof head=%d\n",sizeof(struct p2p_hole_ack));

   /* {
        int opt;

        while((opt = getopt_long(argc, argv, "fl:vh", long_options, NULL)) != -1) 
        {
            switch (opt) 
            {
            case 'l': // local-port 
                sss.lport = atoi(optarg);
                break;
            case 'f': // foreground 
                sss.daemon = 0;
                break;
            case 'h': // help 
                exit_help(argc, argv);
                break;
            case 'v': // verbose 
                ++traceLevel;
                break;
            }
        }
        
    }*/

#if defined(N2N_HAVE_DAEMON)
    if (sss.daemon)
    {
        useSyslog=1; /* traceEvent output now goes to syslog. */
        if ( -1 == daemon( 0, 0 ) )
        {
            traceEvent( TRACE_ERROR, "Failed to become daemon." );
            exit(-5);
        }
    }
#endif /* #if defined(N2N_HAVE_DAEMON) */

    traceEvent( TRACE_DEBUG, "traceLevel is %d", traceLevel);

    sss.sock = open_socket(sss.lport, 1 /*bind ANY*/ );
    if ( -1 == sss.sock )
    {
        traceEvent( TRACE_ERROR, "Failed to open main socket. %s", strerror(errno) );
        exit(-2);
    }
    else
    {
        traceEvent( TRACE_NORMAL, "p2pserver is listening on UDP %u (main)", sss.lport );
    }

    sss.mgmt_sock = open_socket(N2N_SN_MGMT_PORT, 0 /* bind LOOPBACK */ );
    if ( -1 == sss.mgmt_sock )
    {
        traceEvent( TRACE_ERROR, "Failed to open management socket. %s", strerror(errno) );
        exit(-2);
    }
    else
    {
        traceEvent( TRACE_NORMAL, "p2pserver is listening on UDP %u (management)", N2N_SN_MGMT_PORT );
    }

    traceEvent(TRACE_NORMAL, "p2pserver started");

    return run_loop(&sss);
}


/** Long lived processing entry point. Split out from main to simply
 *  daemonisation on some platforms. */
static int run_loop( n2n_sn_t * sss )
{
    uint8_t pktbuf[N2N_SN_PKTBUF_SIZE];
    int keep_running=1;
	int tcpSock;
	int new_sock;
	//for Forward server
	tcpSock = Init_tcpSock(tcpSock);

    sss->start_time = time(NULL);
	int rc;
	int udpcount = 0;
	size_t bread;
  int max_sock, tmp_max_sock;
  fd_set work_set;
  struct timeval wait_time;
  time_t now=0;

  FD_ZERO(&socket_mask);
  tmp_max_sock = MAX(sss->sock, sss->mgmt_sock);
	tmp_max_sock = MAX(tmp_max_sock, tcpSock);
  FD_SET(sss->sock, &socket_mask);
  FD_SET(sss->mgmt_sock, &socket_mask);
	FD_SET(tcpSock, &socket_mask);
    traceEvent( TRACE_NORMAL, "sss->sock=%d,sss->m_sock=%d,tcpock=%d", sss->sock,sss->mgmt_sock,tcpSock);
    while(keep_running) 
    {
        
		max_sock=tmp_max_sock;
		memset(&work_set, 0, sizeof(work_set));
		memcpy(&work_set, &socket_mask, sizeof(work_set));
        wait_time.tv_sec = 1; wait_time.tv_usec = 0;
        rc = select(max_sock+1, &work_set, NULL, NULL, &wait_time);
		//traceEvent( TRACE_NORMAL, "select rc = %d,max sock=%d", rc, max_sock);
        now = time(NULL);

        if(rc > 0) 
        {
        		for(int j = 0; j<=max_sock;j++)
        		{
        			if(FD_ISSET(j, &work_set))
        			{
		            if (j == sss->sock) 
		            {
		            		traceEvent( TRACE_NORMAL, "udp ......udpcount=%d",udpcount++);
		                struct sockaddr_in  sender_sock;
		                socklen_t           i;

		                i = sizeof(sender_sock);
		                bread = recvfrom( sss->sock, pktbuf, N2N_SN_PKTBUF_SIZE, 0/*flags*/,
						  (struct sockaddr *)&sender_sock, (socklen_t*)&i);
						fwd_printhex((char *)pktbuf,bread);

		                if ( bread < 0 ) /* For UDP bread of zero just means no data (unlike TCP). */
		                {
		                    /* The fd is no good now. Maybe we lost our interface. */
		                    traceEvent( TRACE_ERROR, "sss->sock recvfrom() failed %d errno %d (%s)", bread, errno, strerror(errno) );
		                    //keep_running=0;
		                   // break;
		                }

		                /* We have a datagram to process */
		                                traceEvent( TRACE_NORMAL, "1udp sock port=%d", sender_sock.sin_port);
						 traceEvent( TRACE_NORMAL, "ntohs udp sock port=%d", ntohs(sender_sock.sin_port));
		                if ( bread > 0 )
		                {
		                    /* And the datagram has data (not just a header) */
		                    process_udp( sss, &sender_sock, pktbuf, bread, now );
		                }
		            }
		            else if (j == sss->mgmt_sock) 
		            {
		            		traceEvent( TRACE_NORMAL, "mgmt_sock ......");
		                struct sockaddr_in  sender_sock;
		                size_t              i;

		                i = sizeof(sender_sock);
		                bread = recvfrom( sss->mgmt_sock, pktbuf, N2N_SN_PKTBUF_SIZE, 0/*flags*/,
						  (struct sockaddr *)&sender_sock, (socklen_t*)&i);

		                if ( bread <= 0 )
		                {
		                    traceEvent( TRACE_ERROR, "mgmt_sock recvfrom() failed %d errno %d (%s)", bread, errno, strerror(errno) );
		                   // keep_running=0;
		                    //break;
		                }

		                /* We have a datagram to process */
		                process_mgmt( sss, &sender_sock, pktbuf, bread, now );
		            }
						else if(j == tcpSock)//forward server mesg
						{
							
							new_sock= accept(tcpSock, NULL, NULL);
							//traceEvent( TRACE_NORMAL, "accept.........new_sock=%d",new_sock);
							 // Nothing to be accepted
							 if (new_sock < 0)
							 {
							 	//traceEvent( TRACE_NORMAL, "continue........");
								continue;
							 }
							 else
							 {
							 	//traceEvent( TRACE_NORMAL, "else........");
							 	tmp_max_sock = MAX(tmp_max_sock, new_sock);
								FD_SET(new_sock, &socket_mask);
							 }
									
						}
						else
						{
							//traceEvent( TRACE_NORMAL, "recv.........");
							process_tcp_mesg(j);
						}
					}
        		}
        }
        else
        {
           // traceEvent( TRACE_DEBUG, "timeout" );
        }

        purge_expired_registrations( &(sss->edges) );
			ClearTimeoutFWDServer();

    } /* while */

    deinit_sn( sss );

    return 0;
}

