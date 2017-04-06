
#ifndef EMS_CLIENT_DNS_INTERCEPT_HEADER__
#define EMS_CLIENT_DNS_INTERCEPT_HEADER__

#pragma pack(push, 1)
struct _dns_header_s {
	ems_short   id;

	ems_short flg;

#define DNS_QR(hdr)	((ntohs((hdr)->flg) & 0x8000) >> 15)
#define DNS_OPCODE(hdr)	((ntohs((hdr)->flg) & 0x7800) >> 11)
#define DNS_AA(hdr)	((ntohs((hdr)->flg) & 0x0400) >> 10)
#define DNS_TC(hdr)	((ntohs((hdr)->flg) & 0x0200) >>  9)
#define DNS_RD(hdr)	((ntohs((hdr)->flg) & 0x0100) >>  8)
#define DNS_RA(hdr)	((ntohs((hdr)->flg) & 0x0080) >>  7)
#define DNS_Z(hdr)	((ntohs((hdr)->flg) & 0x0070) >>  4)
#define DNS_RCODE(hdr)	((ntohs((hdr)->flg) & 0x000f) >>  0)

	ems_short   qdcount;
	ems_short   ancount;
	ems_short   nscount;
	ems_short   arcount;
};

struct _dns_item_s {
	ems_short  ty;
	ems_short  cls;
	ems_uint   ttl;
	ems_short  len;
	ems_uchar  buf[0];
};

#pragma pack(pop)

struct _dns_question_s {
	ems_short  qtype;
	ems_short  qclass;
	ems_str    qname;
};

struct _dns_rr_s {
	ems_short           ptr; /*NAME ptr, maybe a name */
	ems_str             nick;
	struct _dns_item_s  item;
};

struct _dns_user_s {
	ems_hash_fd     h_msg;
	ems_timeout     to;
	ems_uint        flg;
	ems_int         port;
	struct in_addr  addr;
	ems_buffer      buf; /* for send and recv */

	ems_void       *ctx;
	ems_queue       entry;
};

struct _dns_url_s {
	ems_hash_fd     h_url;
	ems_uint        flg;
	ems_str         url;
	ems_int         n_addr;
	struct in_addr *addr;
	ems_int         mask;

	ems_queue       entry;
};

#define FLG_URL_IS_IPADDRESS	0x0001
#define FLG_DNS_QUERY_SELF	0x0002
#define FLG_DNS_QUERY_EXPIRED	0x0004
#define FLG_DNS_IS_SUBDOMAIN	0x0008

ems_int fw_dns_be_server(ems_fw *fw);
ems_int fw_dns_server_stop(ems_fw *fw);
ems_int fw_dns_client_stop(ems_fw *fw);
ems_int fw_dns_be_client(ems_fw *fw);

ems_int fw_dns_client_set_read(ems_fw *fw, ems_int rd);
ems_int fw_dns_client_set_write(ems_fw *fw, ems_int wr);
ems_int fw_dns_server_set_read(ems_fw *fw, ems_int rd);
ems_int fw_dns_server_set_write(ems_fw *fw, ems_int wr);

dns_user *dns_find_user(ems_fw *fw, ems_short key);
dns_user *dns_user_new();
ems_void  dns_user_free(dns_user *user);

dns_url  *dns_url_new();
ems_void  dns_url_free(dns_url *url);
dns_url  *dns_find_url(ems_fw *fw, ems_cchar *key);

ems_int fw_dns_query_triger(ems_fw *fw);
ems_int fw_url_in_whitelist(ems_fw *fw, ems_cchar *key);
ems_int fw_dns_is_subdomain(ems_fw *fw, ems_cchar *key);
ems_int fw_dns_subdomain_append(ems_fw *fw, dns_url *url);

#endif
