
#include "ems_core.h"
#include "ems_client.h"
#include "app.h"
#include "ems_fw.h"
#include "ems_dns.h"
#include "ems_http.h"

/*
   for cna handles
 */
#define EMS_CNA_TIMEOUT_PHASE_ONE	3000

#define cna_st_init		0
#define cna_st_phase_one	1
#define cna_st_phase_two	2
#define cna_st_phase_three	3
#define cna_st_phase_exit	4

ems_cna *http_cna_new()
{
	ems_cna *cna = NULL;

	cna = (ems_cna *)ems_malloc(sizeof(ems_cna));
	if (cna) {
		memset(cna, 0, sizeof(ems_cna));

		ems_queue_init(&cna->entry);
		str_init(&cna->ip);
		str_init(&cna->host);
		str_init(&cna->param);
		cna->st = cna_st_init;
		ems_timeout_init(&cna->to);
	}

	return cna;
}

ems_void http_cna_destroy(ems_cna *cna)
{
	if (cna) {
		ems_timeout_cancel(&cna->to);

		str_uninit(&cna->ip);
		str_uninit(&cna->host);
		str_uninit(&cna->param);
		cna->st = 0;

		ems_free(cna);
	}
}

ems_cna *http_cna_user_find(ems_http *http, ems_cchar *ip)
{
	ems_queue *p;
	ems_cna   *cna;

	ems_queue_foreach(&http->cna_list, p) {
		cna = ems_container_of(p, ems_cna, entry);

		ems_assert(cna && str_len(&cna->ip) > 0);

		if (!strcmp(ip, str_text(&cna->ip)))
			return cna;
	}

	return NULL;
}

static ems_int 
update_cna_status(ems_cna *cna, ems_cchar *host, ems_cchar *param, ems_cchar *ver)
{
	ems_assert(cna && host && param && ver);

	if (str_len(&cna->host) <= 0 || strcmp(str_text(&cna->host), host)) {
		str_set(&cna->host, host);
		cna->st = cna_st_init;
	}

	if (str_len(&cna->param) <= 0 || strcmp(str_text(&cna->param), param)) {
		str_set(&cna->param, param);
		cna->st = cna_st_init;
	}

#define HTTP_1_0	"HTTP/1.0"
#define HTTP_1_1	"HTTP/1.1"

	if (ver) {
		switch(cna->st) {
		case cna_st_init:      /* we accept HTTP 1.0*/
		case cna_st_phase_two:
		case cna_st_phase_three:
			if (strcmp(ver, HTTP_1_0))
				cna->st = cna_st_phase_one;
			break;

		case cna_st_phase_one: /* we accept HTTP 1.1 */
			if (strcmp(ver, HTTP_1_1))
				cna->st = cna_st_init;
			break;
		default:
			break;
		}
	}

	return EMS_OK;
}

ems_void http_cna_timeout_cb(ems_timeout *timeout)
{
	ems_cna *cna = ems_container_of(timeout, ems_cna, to);

	ems_l_trace("[http] cna user (%s, status: %d: %s%s) timeout",
			str_text(&cna->ip), cna->st,
			str_text(&cna->host),
			str_text(&cna->param));

	ems_queue_remove(&cna->entry);
	http_cna_destroy(cna);
}

ems_int http_cna_user_run(ems_http *http,
	ems_cna     *cna, 
	ems_session *sess, 
	ems_cchar   *host, 
	ems_cchar   *param)
{
	ems_int rtn;

	if (!cna)  {
		cna = http_cna_new();
		if (!cna)
			return EMS_ERR;

		str_set(&cna->ip, ems_sock_addr(&sess->sock));
		ems_queue_insert_tail(&http->cna_list, &cna->entry);
	}

	ems_l_trace("[http] cna current st: %d (%s: %s) [cna: %s:%s]", 
			cna->st, host, param, str_text(&cna->host), str_text(&cna->param));
	update_cna_status(cna, host, param, http_header_get(http->hdr, HTTP_Version));
	ems_l_trace("[http] cna after updated st: %d", cna->st);

	switch (cna->st) {
	case cna_st_init:
		cna->st = cna_st_phase_one;
		ems_timeout_insert_sorted(timeouter(), &cna->to, 10000, http_cna_timeout_cb);
		rtn = HTTP_RETURN_FAILED;
		break;

	case cna_st_phase_one:
		cna->st = cna_st_phase_two;
		rtn = HTTP_RETURN_PORTAL;
		ems_timeout_insert_sorted(timeouter(), &cna->to, 15000, http_cna_timeout_cb);
		break;

	case cna_st_phase_two:
		cna->st = cna_st_phase_three;
		rtn = HTTP_RETURN_SUCCESS;
		ems_timeout_insert_sorted(timeouter(), &cna->to, 30000, http_cna_timeout_cb);
		break;

	case cna_st_phase_three:
		rtn = HTTP_RETURN_SUCCESS;
		ems_timeout_insert_sorted(timeouter(), &cna->to, 30000, http_cna_timeout_cb);
		break;

	default:
		rtn = HTTP_RETURN_PORTAL;
		break;
	}

	return rtn;
}
