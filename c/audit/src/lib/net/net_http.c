#include "audit.h"
#include "ems_core.h"
#include "ems_client.h"
#include "net.h"

#include <sys/types.h>
#include <sys/stat.h>

typedef struct {
	ems_str citycode;
} http_core;

static ems_int http_stop();

static ems_void http_getcfg(http_core *e6)
{
	str_set(&e6->citycode, cfg_get(emscfg(), CFG_net_filter_e6wifi_citycode));
}

static ems_int http_fillinfo(http_core *e6, net_flow *pkgs, ems_cchar *url)
{

/* usermac, apmac, time, url, userip, username,gwip,3gip,citycode*/
	ems_cchar *usermac,*apmac,*username,*gwip,*threeGIP,*citycode;
        const struct flow_ethernet *ether;
	const struct flow_ip       *ip;
	ems_char tmbuf[64]  = {0};
	struct tm   *tm;
	ems_buffer  *buf = core_buffer();
	

	ip    	= (struct flow_ip*)(pkgs->packet + SIZE_ETHERNET);
	ether 	= (struct flow_ethernet*)(pkgs->packet);
	
        usermac = net_mac2str(ether->ether_shost);
	
        tm	= localtime(&(pkgs->hdr)->ts.tv_sec);	
	snprintf(tmbuf, 64, "%04d-%02d-%02d %02d:%02d:%02d", 
		1900 + tm->tm_year,
                1 + tm->tm_mon,
                tm->tm_mday,
		tm->tm_hour,
                tm->tm_min, 
                tm->tm_sec);
        
        if (str_len(&pkgs->apmac) <= 0){
                apmac = "*";
        }else {
                apmac = str_text(&pkgs->apmac);
        } 
        
        if (str_len(&pkgs->gwip) <= 0){
                gwip = "*";
        }else {
                gwip = str_text(&pkgs->gwip);
        }

        if (str_len(&e6->citycode) <= 0){
                citycode = "*";
        }else {
                citycode = str_text(&e6->citycode);
        }
        
        if (!pkgs->user){
                username = "*";
        }else {
                username = pkgs->user;
        }
        threeGIP = "*";
	
        if (!url){
                url = "*";
        }

        ems_buffer_refresh(buf);
	snprintf(buf_wr(buf), buf_left(buf), 
				"%s,%s,%s,%s,%s,%s,%s,%s,%s", 
				usermac,apmac,tmbuf,url,inet_ntoa(ip->ip_src),username,gwip,threeGIP,citycode);

	output_log(id_net, id_http, buf_rd(buf));
        return EMS_OK;

}

#define HTTP_HOST       "Host: "
#define HTTP_REFERER    "Referer: "
#define HTTP_NEWLINE    "\r\n"          
#define HTTP_GET        "GET"
#define HTTP_POST       "POST"

static ems_cchar *http_get_params(ems_cchar *pkg, ems_char *buf, ems_int l)
{
	ems_int     len;
	ems_cchar  *p, *q, *ender;

	p = pkg;

	if (!strncmp(p, HTTP_GET, 3)) {
		p += 4;
	} else if (!strncmp(p, HTTP_POST, 4)) {
		p += 5;
	} else {
                return NULL;
	}

	ender = strstr(p, HTTP_NEWLINE);
	if (!ender) {
		ems_l_trace("header: %s", p);
                return NULL;
	}

	q = strchr(p, ' ');
	if (!q)
                return NULL;

        if (abs(q - ender) != 9)
                return NULL;

	len = abs(p - q);
        if (len >= l)
                return NULL;

        memcpy(buf, p, len);
        buf[len] = '\0';

        return buf;
}

static ems_cchar *get_http_header(ems_cchar *pkg,ems_cchar *key, ems_char *buf, ems_int l_buf)
{
        ems_int   l = 0;
        ems_cchar *p, *q;

        ems_assert(pkg && key && buf)

        p = strstr(pkg, key);
        if (p) {
                p += strlen(key);
                q = strstr(p, HTTP_NEWLINE);

                if (q) {
                        l = abs(q-p);
                        if (l_buf <= l)
                                return NULL;

                        memcpy(buf, p, l);
                        buf[l] = '\0';
                        return buf;
                }
        }

        return NULL;
}

static ems_int http_handle_pkgs(net_plugin *plg, ems_uint evt, net_flow *pkgs)
{
        ems_int len = 0;
        static ems_char params[2048];
        static ems_char buf[4096];
        ems_char *p;
        ems_char *ptr;

	ems_assert(pkgs != NULL && pkgs->l5 != NULL);
//	ems_l_trace("%s", pkgs->l5);

        if (!http_get_params((ems_cchar *)pkgs->l5,params, 2048))
                return EMS_OK;

        snprintf(buf,4096, "http://");

        if (get_http_header((ems_cchar *)pkgs->l5, HTTP_HOST, buf + 7, 4096 - 7)) {
                len = strlen(buf);

                snprintf(buf + len, 4096 - len, "%s", params);
        } else {
                if (!get_http_header((ems_cchar *)pkgs->l5, HTTP_REFERER, buf, 4096)) {
                        return EMS_OK;
                }
        }
        
        ptr = strtok_r(buf, ",", &p);  

        ems_l_trace("url: %s", ptr);
	http_fillinfo((http_core *)plg->ctx, pkgs, ptr);
	return EMS_OK;
}


static ems_int http_evt_citycode(http_core *e6,json_object *req)
{
	ems_cfg   *cfg = emscfg();
	ems_str   citycode;
	ems_int   rtn;

	ems_assert(req != NULL);
	str_init(&citycode);
	do {
		rtn = EMS_ERR;

		ems_json_get_string_def(req, "citycode", &citycode, NULL); 

		if (str_len(&citycode) <= 0) break;

		str_cpy(&e6->citycode, &citycode);

		cfg_set(cfg, CFG_net_filter_e6wifi_citycode, str_text(&citycode));

		cfg_write(cfg);

		rtn = EMS_OK;
	} while (0);
	str_uninit(&citycode);
	return rtn;
}


static ems_int http_start()
{
	ems_l_trace("http, plug start");

	return EMS_OK;
}

static ems_int http_init(net_plugin *plg)
{
        http_core *e6 = NULL;
	ems_l_trace("http, plug init");
        e6 = (http_core *)ems_malloc(sizeof(http_core));
        

        memset(e6,0,sizeof(http_core));
	str_init(&e6->citycode);
        http_getcfg(e6);
        plg->ctx = (ems_void *)e6;
	return EMS_OK;
}



static ems_int http_process(net_plugin *plg, ems_uint evt, ems_void *arg)
{
        http_core *e6 = (http_core *)plg->ctx;
        ems_l_trace("http process, evt %d(0x%x)", evt, evt);

	switch(evt) {
	case A_AUDIT_NET_PKGS:
		return http_handle_pkgs(plg, evt, (net_flow *)arg);

	case A_AUDIT_START:
		ems_l_trace("http start");
		return http_start();

	case A_AUDIT_STOP:
		ems_l_trace("http stop");
		return http_stop();
        
        case MSG_E6WIFI_CITYCODE:
                return http_evt_citycode(e6,(json_object *)arg);

	default:
		break;
	}
	return EMS_OK;
}

static ems_int http_uninit(net_plugin *plg)
{
	http_core *e6 = (http_core *)plg->ctx;
        ems_l_trace("http, plug uninit");
	str_uninit(&e6->citycode);
        ems_free(e6);
        plg->ctx = NULL;
        return EMS_OK;
}

static ems_int http_stop()
{
	ems_l_trace("http, plug stop");
	return EMS_OK;
}

net_plugin net_http = {
	.id    = id_http,
	.mount = id_net,
	.desc  = ems_string("http"),
	.ctx   = NULL,
	.init  = http_init,
	.process = http_process,
	.uninit  = http_uninit,
};

