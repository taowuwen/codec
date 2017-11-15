
#ifndef EMS_HEADER_FOR_HTTP___
#define EMS_HEADER_FOR_HTTP___

#define EMS_FLG_HTTP_ENABLE_CNA		0x00004000
#define EMS_FLG_HTTP_ENABLE_URL		0x00002000
#define EMS_FLG_HTTP_ENABLE_NASGETINFO	0x00001000
#define EMS_FLG_HTTP_ENABLE_GREYLIST	0x00000800

typedef struct _ems_http_s          ems_http;
typedef struct _ems_cna_s           ems_cna;
typedef struct _ems_cna_rule_s      ems_cna_rule;
typedef struct _ems_url_whitelist_s ems_url_greylist;
typedef struct _ems_url_whitelist_s ems_url_whitelist;
typedef struct _http_header_s       http_header;

struct _ems_url_whitelist_s {
	ems_queue  entry;
	ems_str    key;
};

struct _ems_cna_rule_s {
	ems_queue  entry;
	ems_str    host;
	ems_str    param;
};

struct _ems_http_s {
	ems_queue    entry;
	ems_uint     flg;
	ems_session *sess;
	ems_queue    cmd; /* for requests */
	ems_status   st;

	struct {
		ems_int      src_port;
		ems_int      dst_port;
		ems_str      nas;
		ems_queue    url; /* for url whitelists*/
		ems_queue    cna; /* for cna lists */
		ems_queue    cna_list; /* for cna requests */
		ems_queue    grey; /* for grey lists*/
	};

	http_header      *hdr; // for save memory, and speed up access
	ems_nic_wireless *ssid;
};

struct _ems_cna_s {
	ems_queue   entry;

	ems_str     ip;
	ems_str     host;
	ems_str     param;
	ems_int     st;
	ems_timeout to;
};

ems_void ems_url_whitelist_destroy(ems_url_whitelist *white);
ems_cna_rule *ems_cna_rule_new();
ems_void ems_cna_rule_destroy(ems_cna_rule *cna);
ems_int ems_http_change_status(ems_http *http, ems_status st);
ems_url_whitelist *ems_url_whitelist_new();

#define ems_url_greylist_new      ems_url_whitelist_new
#define ems_url_greylist_destroy  ems_url_whitelist_destroy

ems_void http_cna_destroy(ems_cna *cna);
#define HTTP_RETURN_FAILED	0x100
#define HTTP_RETURN_SUCCESS	0x101
#define HTTP_RETURN_PORTAL	0x102

ems_int http_cna_user_run(ems_http*, ems_cna*, ems_session*, ems_cchar*, ems_cchar*);
ems_cna *http_cna_user_find(ems_http*, ems_cchar*);


typedef enum {
	HTTP_MIN     = 0,
#define HTTP_Method	HTTP_MIN
	HTTP_Param,
	HTTP_Version,
	HTTP_Host,
	HTTP_UserAgent,
//	HTTP_ContentLength,
	HTTP_MAX
} http_request_key;


http_header *http_header_new();
ems_void http_header_destroy(http_header *);

ems_void http_header_init(http_header *);
ems_void http_header_uninit(http_header *);
#define http_header_clear	http_header_uninit

ems_int    http_header_parse(http_header *, ems_cchar *);
ems_cchar *http_header_get(http_header *, http_request_key);
ems_cchar *http_header_set(http_header *, http_request_key, ems_cchar *str);
ems_int    http_header_full(ems_cchar *);
ems_int    http_msg_is_web(ems_cchar *);

#define HTTP_NEWLINE	"\r\n"
#define HTTP_GET	"GET"
#define HTTP_POST	"POST"


#endif
