
#include "ems_core.h"
#include "ems_client.h"
#include "app.h"
#include "ems_fw.h"
#include "ems_portal.h"
#include "ems_radius.h"

#define PORTAL_RETRY_TIMES	3
#define PORTAL_RETRY_TIMEOUT	9
#define PORTAL_ERROR_TIMEOUT	30000

#define ERR_PORTAL_NETWORK	0x3000
#define ERR_PORTAL_RESPONSE	0x3001


static ems_int portal_start(ems_portal *ptl, ems_session *sess, ems_uint flg);
static ems_int portal_reg (ems_portal *ptl, ems_session *sess, ems_uint flg);
static ems_int portal_normal(ems_portal *ptl, ems_session *sess, ems_uint flg);
static ems_int portal_hb (ems_portal *ptl, ems_session *sess, ems_uint flg);
static ems_int portal_stopped(ems_portal *ptl, ems_session *sess, ems_uint flg);
static ems_int portal_err  (ems_portal *ptl, ems_session *sess, ems_uint flg);

typedef ems_int (*portal_evt_func)(ems_portal *ptl, ems_session *sess, ems_uint flg);
static portal_evt_func portal_evt_handler[] = 
{
	[st_start]   = portal_start,
	[st_reg]     = portal_reg,
	[st_normal]  = portal_normal,
	[st_hb]      = portal_hb, 
	[st_err]     = portal_err,
	[st_stopped] = portal_stopped
};

static ems_int portal_to_reg(ems_portal *ptl, ems_session *sess, ems_timeout *to);
static ems_int portal_to_normal(ems_portal *ptl, ems_session *sess, ems_timeout *to);
static ems_int portal_to_hb(ems_portal *ptl, ems_session *sess, ems_timeout *to);
static ems_int portal_to_err(ems_portal *ptl, ems_session *sess, ems_timeout *to);

typedef ems_int (*portal_timeout_func)(ems_portal *ptl, ems_session *sess, ems_timeout *to);
static portal_timeout_func portal_timeout_handler[] = 
{
	[st_start]   = NULL,
	[st_reg]     = portal_to_reg,
	[st_normal]  = portal_to_normal,
	[st_hb]      = portal_to_hb, 
	[st_err]     = portal_to_err,
	[st_stopped] = NULL
};

typedef ems_int (*ptl_recv_cb)(ems_portal *ptl, ems_session *sess, ems_queue *list);


typedef struct{
	ems_int    attr; 
	ems_int    id;
	ems_int    ty; /* int string */
} portal_dict;

typedef struct {
	ems_uchar  ty;
	ems_uchar  len;
	ems_char   val[2];
} portal_attr;

enum {
	portal_ty_req_challenge = 0x01,
	portal_ty_ack_challenge = 0x02,
	portal_ty_req_auth      = 0x03,
	portal_ty_ack_auth      = 0x04,
	portal_ty_req_logout    = 0x05,
	portal_ty_ack_logout    = 0x06,
	portal_ty_aff_ack_auth  = 0x07,
	portal_ty_ntf_logout    = 0x08,
	portal_ty_req_info      = 0x09,
	portal_ty_ack_info      = 0x0a
};

enum {
	portal_user_st_normal        = 0x00,
	portal_user_st_req_challenge = 0x01,
	portal_user_st_ack_challenge = 0x02,
	portal_user_st_req_auth      = 0x03,
	portal_user_st_ack_auth      = 0x04,
	portal_user_st_req_logout    = 0x05,
	portal_user_st_ack_logout    = 0x06,
	portal_user_st_aff_ack_auth  = 0x07,
	portal_user_st_ntf_logout    = 0x08
};

#define PORTAL_AUTH_TYPE	"PAP,CHAP"

#define TYPE_INT	0
#define TYPE_STRING	1

#define ATTR_PKGTYPE	0x01
#define ATTR_AUTHTYPE	0x02
#define ATTR_NASNAME	0x03
#define ATTR_RESPONSE	0x04
#define ATTR_USERNAME	0x05
#define ATTR_PASSWORD	0x06
#define ATTR_CHALLENGE	0x07
#define ATTR_CHAPPASS	0x08
#define ATTR_USERMAC	0x09
#define ATTR_ONLINE	0x0a


static portal_dict g_dict[] = {
	{ATTR_PKGTYPE,     0x0b, TYPE_INT},
	{ATTR_AUTHTYPE,    0x02, TYPE_STRING},
	{ATTR_NASNAME,     0x03, TYPE_STRING},
	{ATTR_RESPONSE,    0x12, TYPE_STRING},
	{ATTR_USERNAME,    0x01, TYPE_STRING},
	{ATTR_PASSWORD,    0x02, TYPE_STRING},
	{ATTR_CHALLENGE,   0x03, TYPE_STRING},
	{ATTR_CHAPPASS,    0x04, TYPE_STRING},
	{ATTR_USERMAC,     0xc9, TYPE_STRING},
	{ATTR_ONLINE,      0x09, TYPE_STRING}
};

portal_dict *ptl_dict_load(ems_int attr)
{
	ems_int i, len;
	portal_dict *dict = NULL;

	len = sizeof(g_dict) / sizeof(portal_dict);

	for (i = 0; i < len; i++) {
		dict = &g_dict[i];

		if (dict->attr == attr)
			return dict;
	}

	return NULL;
}


static ems_void ptl_vp_destroy(portal_value_pair *vp)
{
	if (vp) {
		if (vp->val) {
			ems_free(vp->val);
			vp->val = NULL;
		}

		ems_free(vp);
	}
}

static portal_value_pair *ptl_vp_new()
{
	portal_value_pair *vp = NULL;

	vp = (portal_value_pair *)ems_malloc(sizeof(portal_value_pair));
	if (vp) {
		memset(vp, 0, sizeof(portal_value_pair));
		vp->val = NULL;
		ems_queue_init(&vp->entry);
	}

	return vp;
}


static portal_value_pair * 
ptl_vp_append(ems_queue *head, ems_int attr, ems_cchar *val, ems_int lval)
{
	portal_dict       *dict = NULL;
	portal_value_pair *vp = NULL;

	dict = ptl_dict_load(attr);
	if (!dict) {
		ems_assert(0 && "should never be here");
		ems_l_trace("[portal] did not find any attribute : %d", attr);
		return NULL;
	}

	vp = ptl_vp_new();
	if (!vp)
		return NULL;

	vp->attr = dict->attr;
	vp->id   = dict->id;
	vp->ty   = dict->ty;

	switch(dict->ty) {
	case TYPE_STRING:
	{
		if (val) {
			if (lval < 0)
				lval = strlen((ems_char *)val);

			vp->lval = lval;
			if (vp->lval > 253) {
				ems_l_trace("[portal] value length too long: %d", vp->lval);
				ptl_vp_destroy(vp);
				return NULL;
			}

			vp->val  = ems_malloc(lval + 1);
			if (!vp->val) {
				ptl_vp_destroy(vp);
				return NULL;
			}

			memcpy(vp->val, val, lval);
			vp->val[vp->lval] = '\0';
		} else {
			vp->val  = NULL;
			vp->lval = 0;
		}
	}
	break;

	case TYPE_INT:
	{
		vp->lval = *(ems_int *)val;
	}
	break;

	default:
		ems_assert(0 && "never be here");
		ptl_vp_destroy(vp);
		return NULL;
	}

	ems_queue_insert_tail(head, &vp->entry);

	return vp;
}

static ems_int ptl_pack_list(ems_queue *list, ems_buffer *buff)
{
	portal_value_pair *vp = NULL;
	portal_attr  *attr;
	ems_queue    *q;
	ems_char     *buf  = buf_wr(buff);
	ems_int       lbuf = buf_left(buff);

	ems_queue_foreach(list, q) {
		vp = ems_container_of(q, portal_value_pair, entry);
		attr =  (portal_attr *) buf;

		if (lbuf <= 0) 
			return EMS_BUFFER_INSUFFICIENT;

		switch(vp->ty) {
		case TYPE_STRING:
		{
			attr->ty  = vp->id;
			attr->len = vp->lval + 2; 

			if (lbuf <= attr->len)
				return EMS_BUFFER_INSUFFICIENT;

			if (vp->val) {
				ems_assert(vp->lval > 0);
				memcpy(attr->val, vp->val, vp->lval);
			}
		}
		break;

		case TYPE_INT:
		{
			attr->ty  = vp->id;
			attr->len = 6; 

			if (lbuf <= attr->len)
				return EMS_BUFFER_INSUFFICIENT;

			vp->lval = htonl(vp->lval);

			memcpy(attr->val, &vp->lval, 4);
		}
		break;

		default:
			ems_assert(0 && "never be here");
			return EMS_ERR;
		}

		buf  += attr->len;
		lbuf -= attr->len;
	}

	ems_buffer_seek_wr(buff, abs(buf_wr(buff) - buf), EMS_BUFFER_SEEK_CUR);

	return EMS_OK;
}

static ems_int 
ptl_parse_ctx(ems_queue *list, ems_cchar *buf, ems_int lbuf, ems_int n, ems_int evt)
{
	portal_value_pair *vp = NULL;
	portal_attr       *attr = NULL;
	ems_cchar         *p;
	ems_int            attribute;

	p = buf;
	while (n != 0 && lbuf > 0) {
		attr = (portal_attr *)buf;
		if (lbuf < attr->len)
			return EMS_CONTINUE;

		buf  += attr->len;
		lbuf -= attr->len;
		n--;

		switch(attr->ty) {
		case 0x01:
			attribute = ATTR_USERNAME;
			break;
		case 0x0b:
			attribute = ATTR_PKGTYPE;
			break;
		case 0x02:
			attribute = ATTR_AUTHTYPE;
			if (evt)
				attribute = ATTR_PASSWORD;
			break;
		case 0x03:
			attribute = ATTR_NASNAME;
			if (evt)
				attribute = ATTR_CHALLENGE;
			break;
		case 0x04:
			attribute = ATTR_CHAPPASS;
			break;
		case 0xc9:
			attribute = ATTR_USERMAC;
			break;
		case 0x12:
			attribute = ATTR_RESPONSE;
			break;
		case 0x09:
			attribute = ATTR_ONLINE;
			break;

		default:
			attribute = 0;
			ems_l_trace("[portal] unkown ty: %d, length: %d", attr->ty, attr->len);
			if (attr->len <= 0) {
				ems_l_trace("[portal]parse stopped", attr->ty, attr->len);
				return abs(buf -p);
			}
			break;
		}

		if (attribute == 0)
			continue;

		vp = ptl_vp_append(list, attribute, attr->val, attr->len - 2);

		if (vp && (vp->ty == TYPE_INT)) {
			vp->lval = ntohl(vp->lval);
		}

#ifdef DEBUG
		if (vp) {
			if (vp->ty == TYPE_INT)
				ems_l_trace("[portal] got attr: %d, id: %d, value: %x",  
						vp->attr, vp->id, vp->lval);
			else {

				if (attribute == ATTR_USERMAC) {
					ems_char usermac[12];
					ems_bin2str(vp->val, vp->lval, usermac, 12);
					ems_l_trace("[portal] got attr: %d, id: %d value: %s",  
							vp->attr, vp->id, usermac);
				}
				else
					ems_l_trace("[portal] got attr: %d, id: %d, value: %s",  
						vp->attr, vp->id, vp->val?vp->val:"no ctx");
			}
		}
#endif
	}

	return abs(buf - p);
}

static portal_value_pair *ptl_vp_find(ems_queue *list, ems_int attr)
{
	ems_queue *p;
	portal_value_pair *vp = NULL;

	ems_queue_foreach(list, p) {
		vp = ems_container_of(p, portal_value_pair, entry);

		if (vp->attr == attr)
			return vp;
	}

	return NULL;
}


/* user area */
static portal_user *ptl_user_new()
{
	portal_user *user = NULL;

	user = (portal_user *)ems_malloc(sizeof(portal_user));
	if (user) {
		memset(user, 0, sizeof(portal_user));
		str_init(&user->name);
		str_init(&user->pass);
		str_init(&user->mac);
		str_init(&user->ip);

		user->serial = 0;
		user->reqid  = 0;
		user->flg    = 0;

		ems_timeout_init(&user->to);
		ems_buffer_init(&user->buf_out, EMS_BUFFER_1K);
		user->st    = portal_user_st_normal;
		user->retry_times   = 0;
		user->retry_timeout = 0;

		ems_queue_init(&user->entry);
	}

	return user;
}

ems_void ptl_user_destroy(portal_user *user)
{
	if (user) {
		ems_l_trace("destroy user(%s, %s, %s, %s), exit st: 0x%x", 
				str_text(&user->name),
				str_text(&user->pass),
				str_text(&user->mac),
				str_text(&user->ip),
				user->st
				);
		str_clear(&user->name);
		str_clear(&user->pass);
		str_clear(&user->mac);
		str_clear(&user->ip);
		

		ems_buffer_uninit(&user->buf_out);
		ems_free(user);
	}
}


portal_user *ptl_user_find(ems_portal *ptl, ems_cchar *ip)
{
	ems_queue   *p;
	portal_user *user;

	ems_assert(ptl && ip);

	ems_queue_foreach(&ptl->users, p) {

		user = ems_container_of(p, portal_user, entry);

		ems_assert(str_len(&user->ip) > 0);

		if (!strcmp(str_text(&user->ip), ip))
			return user;
	}

	return NULL;
}

/* end of user area */

static ems_int ptl_evt_run(ems_portal *ptl, ems_session *sess, ems_uint flg)
{
	ems_assert(portal_evt_handler[ptl->st] != NULL);

	return portal_evt_handler[ptl->st](ptl, sess, flg);
}

static ems_int 
ptl_timeout_run(ems_portal *ptl, ems_session *sess, ems_timeout *to)
{
	ems_assert(portal_timeout_handler[ptl->st] != NULL);

	if (portal_timeout_handler[ptl->st])
		return portal_timeout_handler[ptl->st](ptl, sess, to);

	return EMS_OK;
}

static ems_void ptl_evt_cb(ems_session *sess, ems_int err, ems_int flg)
{
	ems_portal *ptl = (ems_portal *)sess_cbarg(sess);

	ems_assert(ptl->st > st_min && ptl->st < st_max);

	if (err) {
		ems_l_trace("[portal] evt err, sess: %d %s",
				ems_sock_fd(&sess->sock),
				ems_sock_addr(&sess->sock));
		ptl->lasterr = ERR_PORTAL_NETWORK;
		portal_change_status(ptl, st_err);
		return;
	}

	ptl_evt_run(ptl, sess, flg);
}

static ems_void ptl_timeout_cb(ems_session *sess, ems_timeout *to)
{
	ems_portal *ptl = (ems_portal *)sess_cbarg(sess);

	ems_assert(ptl->st > st_min && ptl->st < st_max);

	ptl_timeout_run(ptl, sess, to);
}

static ems_int portal_connect(ems_session *sess)
{
	ems_int    fd;
	socklen_t  len;
	struct sockaddr_in addr;
	ems_sock   *sock = &sess->sock;
	ems_assert(sess);
	
	memset(&addr, 0, sizeof(addr));
	if (ems_gethostbyname(ems_sock_addr(sock), &addr) != OK) {
		ems_l_trace("[portal]gethostbyename failed %s : %s", 
				ems_sock_addr(sock), ems_lasterrmsg());
		return EMS_ERR;
	}

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd <= 0)
		return EMS_ERR;

	ems_l_trace("[portal] sess(%d) connect to: %s(%s): %d...",
			fd, 
			ems_sock_addr(sock), 
			inet_ntoa(addr.sin_addr), 
			ems_sock_port(sock));

	addr.sin_family = AF_INET;
	addr.sin_port   = htons(ems_sock_port(sock));

	ems_setnonblocking(fd, YES);
	len = sizeof(struct sockaddr_in);
	if (connect(fd, (struct sockaddr *)&addr, len)) {
		ems_l_trace("[portal] connect to: %s:%d: failed: %s",
				ems_sock_addr(sock), 
				ems_sock_port(sock), 
				ems_lasterrmsg());
		close(fd);
		return EMS_ERR;
	}

	ems_sock_setfd(sock, fd);
	return EMS_OK;
}

static ems_int portal_start(ems_portal *ptl, ems_session *sess, ems_uint flg)
{
	if ( !ptl->sess) {
		ptl->sess = ems_session_new();
		if (!ptl->sess) {
			ems_assert(0 && "never be here");
			portal_change_status(ptl, st_err);
			return EMS_ERR;
		}

		ems_buffer_increase(&ptl->sess->buf_in,  EMS_BUFFER_1K);
		ems_buffer_increase(&ptl->sess->buf_out, EMS_BUFFER_1K);
	}

	sess = ptl->sess;
	ptl->lasterr = 0;
	sess_cbarg_set(sess, ptl);

	ems_sock_setaddr(&sess->sock, str_text(&ptl->addr));
	ems_sock_setport(&sess->sock, ptl->port);
	if (portal_connect(sess) != EMS_OK) {
		ptl->lasterr = ERR_PORTAL_NETWORK;
		portal_change_status(ptl, st_err);
		return EMS_ERR;
	}

	ems_app_process(ty_portal, ty_radius, EMS_APP_RADIUS_START, NULL);

	return portal_change_status(ptl, st_reg);
}

static ems_cchar *ptl_mac_to_radius(ems_str *mac)
{
	static ems_char buff[32], *p;
	ems_cchar  *buf;
	ems_int    i;

	i   = 0;
	buf = str_text(mac);
	p   = buff;

	while (*buf) {
		*p++ = *buf++;

		if (i++ % 2)
			*p++ = ':';
	}

	p--;
	*p = '\0';

	return buff;
}

static ems_cchar *radius_mac_to_ptl(ems_cchar *mac)
{
	static ems_char buf[32] = {0};
	ems_char ch, *p;

	p = buf;

	memset(buf, 0, sizeof(buf));
	while (*mac) {
		ch = *mac++;
		if (ch == ':')
			continue;
		*p++ = ch;
	}

	return buf;
}


static ems_void user_timeout_cb(ems_timeout *timeout)
{
	portal_user *user = ems_container_of(timeout, portal_user, to);
	ems_portal  *ptl  = user->ptl;

	ems_l_trace("timer user(st: 0x%x, %s, %s), retry times: %d", user->st,
			str_text(&user->ip), str_text(&user->name), user->retry_times);

	user->retry_times--;
	if (user->retry_times > 0) {
		ems_session *sess = ptl->sess;
		
		ems_buffer_refresh(&sess->buf);
		ems_buffer_write(&sess->buf, buf_rd(&user->buf_out), buf_len(&user->buf_out));
		sess_event_set(sess, EMS_EVT_WRITE, ptl_evt_cb);
		ems_timeout_insert_sorted(timeouter(), &user->to, user->retry_timeout * 1000, user_timeout_cb);
		return;
	}

	switch(user->st) {
	case portal_user_st_ack_auth:
	{
		if (ems_flag_like(user->flg, EMS_FLG_ONLINE))
		{
			/* we did not recved aff_ack_auth, do logout */
			json_object  *jobj;
			jobj = json_object_new_object();
			json_object_object_add(jobj, "username", json_object_new_string(str_text(&user->name)));
			json_object_object_add(jobj, "userip",   json_object_new_string(str_text(&user->ip)));
			json_object_object_add(jobj, "usermac",  json_object_new_string(ptl_mac_to_radius(&user->mac)));
			ems_send_message(ty_portal, ty_radius,   EMS_APP_CMD_RADIUS_LOGOUT, jobj);

			json_object_put(jobj);

			ptl_user_ntf_logout(ptl, user);
		} else {
			ems_queue_remove(&user->entry);
			ptl_user_destroy(user);
		}
	}
	break;

	case portal_user_st_ntf_logout:
	case portal_user_st_ack_logout:
	case portal_user_st_ack_challenge:
	{
		ems_queue_remove(&user->entry);
		ptl_user_destroy(user);
	}
	break;

	default:
		break;
	}
}


static ems_int ptl_evt_ack(
		ems_portal        *ptl,
		portal_user       *user,
		portal_auth_hdr   *auth, 
		ems_queue         *attr)
{
	ems_char     *wr;
	ems_session  *sess = ptl->sess;

	ems_assert(ptl && auth && "never show up this line");

	if (attr) {
		ems_queue_len(attr, auth->n_attr);
	} else
		auth->n_attr = 0;

	wr = buf_wr(&sess->buf);
	ems_buffer_write(&sess->buf, (ems_char *)auth->val, sizeof(portal_auth_hdr));
	if (attr)
		ptl_pack_list(attr, &sess->buf);

	ems_l_trace("[portal]ptl, ack 0x%x, err: %d", auth->ty, auth->err);
	if (user) {
		ems_l_trace("[portal]ptl (%s: %s), ack 0x%x, err: %d",
				str_text(&user->ip), str_text(&user->name), auth->ty, auth->err);

		ems_buffer_clear(&user->buf_out);
		ems_buffer_write(&user->buf_out, wr, abs(buf_wr(&sess->buf) - wr));

		user->retry_times   = PORTAL_RETRY_TIMES;
		user->retry_timeout = PORTAL_RETRY_TIMEOUT;

		ems_timeout_insert_sorted(timeouter(), &user->to, user->retry_timeout * 1000, user_timeout_cb);
	}

	sess_event_set(sess, EMS_EVT_WRITE, ptl_evt_cb);

	return EMS_OK;
}

static ems_int
ptl_evt_req_auth(ems_portal *ptl, ems_session *sess, portal_auth_hdr *auth, ems_queue *attr)
{
	portal_user       *user;
	portal_value_pair *vp;
	static ems_char    ip[32];
	static ems_char    mac[32];
	struct in_addr     addr;

	memset(&addr, 0, sizeof(addr));
	memcpy(&addr.s_addr, &auth->ip, 4);
	//addr.s_addr = htonl(auth->ip);
	snprintf(ip, sizeof(ip), "%s", inet_ntoa(addr));

	user = ptl_user_find(ptl, ip);

	if (!user) {
		vp = ptl_vp_find(attr, ATTR_USERNAME);
		if (!(vp && vp->val)) goto err_out;

		user = ptl_user_new();
		if (!user) goto err_out;

		user->ptl = ptl;
		str_set(&user->name, vp->val);
		str_set(&user->ip,   ip);
		ems_queue_insert_tail(&ptl->users, &user->entry);

	} else {
		ems_l_trace("(curent(0x%04x, 0x%04x), user(0x%04x, 0x%04x)", 
					auth->serial, auth->reqid, user->serial, user->reqid);

		if (ems_flag_like(user->flg, EMS_FLG_ONLINE)) {
			user->serial = auth->serial;
			auth->err = 2;
			goto err_out_1;
		}

		if (user->st == portal_user_st_req_auth) {
			auth->err = 3;
			goto err_out_1;
		}

		if (user->st == portal_user_st_ack_challenge) {
			if (user->reqid != auth->reqid) {
				auth->err = 4;
				goto err_out_1;
			}
		}
	}

	ems_flag_unset(user->flg, EMS_FLG_ONLINE);
	user->st = portal_user_st_req_auth;
	user->serial = auth->serial;
	user->reqid  = auth->reqid;
	user->auth_ty = auth->auth_ty;

	if (auth->auth_ty == 0) { /* chap */

		vp = ptl_vp_find(attr, ATTR_CHAPPASS);
		if (!(vp && vp->val)) goto err_out;
		str_set(&user->pass, vp->val);

	} else if (auth->auth_ty == 1) { /* pap */

		vp = ptl_vp_find(attr, ATTR_PASSWORD);
		if (!(vp && vp->val)) goto err_out;
		str_set(&user->pass, vp->val);

	} else {
		ems_assert(0 && "server's error");
		goto err_out;
	}

	vp = ptl_vp_find(attr, ATTR_USERMAC);
	if (vp && vp->val)
		ems_bin2str(vp->val, vp->lval, mac, 32);
	else {
		snprintf(mac, sizeof(mac), "%s", radius_mac_to_ptl(ems_usermac(ip)));
		if (strlen(mac) <= 0)
			goto err_out;
	}
	
	str_set(&user->mac, mac);
	{
		json_object  *jobj;
		jobj = json_object_new_object();
		json_object_object_add(jobj, "username", json_object_new_string(str_text(&user->name)));
		json_object_object_add(jobj, "userpass", json_object_new_string(str_text(&user->pass)));
		json_object_object_add(jobj, "userip",   json_object_new_string(str_text(&user->ip)));
		json_object_object_add(jobj, "usermac",  json_object_new_string(ptl_mac_to_radius(&user->mac)));
		ems_send_message(ty_portal, ty_radius,   EMS_APP_CMD_RADIUS_AUTH, jobj);

		json_object_put(jobj);
	}

	return EMS_OK;

err_out_1:
	auth->serial = htons(auth->serial);
	auth->reqid  = htons(auth->reqid);
	auth->ty = portal_ty_ack_auth;
	return ptl_evt_ack(ptl, NULL, auth, NULL);

err_out:
	auth->serial = htons(auth->serial);
	auth->reqid  = htons(auth->reqid);
	if (user)
		user->st = portal_user_st_ack_auth;
	auth->ty = portal_ty_ack_auth;
	auth->err = 1;
	return ptl_evt_ack(ptl, user, auth, NULL);
}

static ems_int
ptl_evt_req_info(ems_portal *ptl, ems_session *sess, portal_auth_hdr *auth, ems_queue *attr)
{
	auth->ty  = portal_ty_ack_info;
	auth->err = 0;
	return ptl_evt_ack(ptl, NULL, auth, NULL);
}

static ems_int
ptl_evt_req_logout(ems_portal *ptl, ems_session *sess, portal_auth_hdr *auth, ems_queue *attr)
{
	portal_user       *user;
	static ems_char    ip[32];
	struct in_addr     addr;

	memset(&addr, 0, sizeof(addr));
	memcpy(&addr.s_addr, &auth->ip, 4);
	//addr.s_addr = htonl(auth->ip);
	snprintf(ip, sizeof(ip), "%s", inet_ntoa(addr));

	auth->ty  = portal_ty_ack_logout;
	auth->err = 0;
	auth->serial = htons(auth->serial);
	auth->reqid  = htons(auth->reqid);

	user = ptl_user_find(ptl, ip);

	if (user) {
		json_object  *jobj;

		user->st     = portal_user_st_ack_logout;
		user->serial = auth->serial;
		user->reqid  = auth->reqid;
		ems_flag_unset(user->flg, EMS_FLG_ONLINE);

		jobj = json_object_new_object();
		json_object_object_add(jobj, "username", json_object_new_string(str_text(&user->name)));
		json_object_object_add(jobj, "userip",   json_object_new_string(str_text(&user->ip)));
		json_object_object_add(jobj, "usermac",  json_object_new_string(ptl_mac_to_radius(&user->mac)));
		ems_send_message(ty_portal, ty_radius,   EMS_APP_CMD_RADIUS_LOGOUT, jobj);

		json_object_put(jobj);
	}

	return ptl_evt_ack(ptl, user, auth, NULL);
}

static ems_int
ptl_evt_aff_ack_auth(ems_portal *ptl, ems_session *sess, portal_auth_hdr *auth, ems_queue *attr)
{
	portal_user       *user;
	static ems_char    ip[32];
	struct in_addr     addr;

	memset(&addr, 0, sizeof(addr));
	memcpy(&addr.s_addr, &auth->ip, 4);
	//addr.s_addr = htonl(auth->ip);
	snprintf(ip, sizeof(ip), "%s", inet_ntoa(addr));

	user = ptl_user_find(ptl, ip);

	if (user) {
		ems_flag_set(user->flg, EMS_FLG_ONLINE);
		user->st = portal_user_st_aff_ack_auth;
		ems_timeout_cancel(&user->to);
	}

	return EMS_OK;
}

static ems_int
ptl_evt_req_challenge(ems_portal *ptl, ems_session *sess, portal_auth_hdr *auth, ems_queue *attr)
{
	portal_user       *user;
	static ems_char    ip[32];
	struct in_addr     addr;

	memset(&addr, 0, sizeof(addr));
	memcpy(&addr.s_addr, &auth->ip, 4);
	//addr.s_addr = htonl(auth->ip);
	snprintf(ip, sizeof(ip), "%s", inet_ntoa(addr));

	user = ptl_user_find(ptl, ip);

	if (!user) {
		portal_value_pair *vp;
		vp = ptl_vp_find(attr, ATTR_USERNAME);
		if (!(vp && vp->val)) goto err_out;

		user = ptl_user_new();
		if (!user) goto err_out;

		user->ptl = ptl;
		str_set(&user->name, vp->val);
		str_set(&user->ip,   ip);
		user->serial = auth->serial;
		user->reqid  = random() & 0xffff;/* we need to sereqid*/
		ems_queue_insert_tail(&ptl->users, &user->entry);
	} else
		goto err_out;

	{
		ems_queue  vp;

		ems_queue_init(&vp);
		{
			ems_char  vector[32] = {0};
			rc_random_vector((ems_uchar *)vector);
			ptl_vp_append(&vp, ATTR_CHALLENGE,vector, 16);
		}

		user->st     = portal_user_st_ack_challenge;
		user->auth_ty = auth->auth_ty;
		auth->serial = htons(user->serial);
		auth->reqid  = htons(user->reqid);

		auth->ty  = portal_ty_ack_challenge;
		auth->err = 0;

		ptl_evt_ack(ptl, user, auth, &vp);
		ems_queue_clear(&vp, portal_value_pair, entry, ptl_vp_destroy);
	}

	return EMS_OK;

err_out:
	if (user)
		user->st = portal_user_st_ack_challenge;

	auth->serial = htons(auth->serial);
	auth->reqid  = htons(auth->reqid);
	auth->ty = portal_ty_ack_challenge;
	auth->err = 1;
	return ptl_evt_ack(ptl, user, auth, NULL);
}

static ems_int 
ptl_process_evt(ems_portal *ptl, ems_session *sess, portal_auth_hdr *auth, ems_queue *attr)
{
	ems_assert(ptl && sess && auth);

	switch(auth->ty) {
	case portal_ty_req_challenge:
		return ptl_evt_req_challenge(ptl, sess, auth, attr);

	case portal_ty_req_auth:
		return ptl_evt_req_auth(ptl, sess, auth, attr);

	case portal_ty_req_logout:
		return ptl_evt_req_logout(ptl, sess, auth, attr);

	case portal_ty_aff_ack_auth:
		return ptl_evt_aff_ack_auth(ptl, sess, auth, attr);

	case portal_ty_req_info:
		return ptl_evt_req_info(ptl, sess, auth, attr);

	default:
		ems_assert(0 && "never show up this line");
		ems_l_trace("[portal]not handle(ty: 0x%x, err: 0x%x) for now", auth->ty, auth->err);
		break;
	}

	return EMS_OK;
}

static ems_int 
ptl_process_rsp(ems_portal *ptl, ems_session *sess, ptl_recv_cb h)
{
	portal_auth_hdr  auth;
	ems_cchar       *buf = NULL;
	ems_queue        vp;
	ems_int          total, rtn = EMS_OK;
	ems_int          len = sizeof(auth);

	ems_queue_init(&vp);

	if (buf_len(&sess->buf_in) <= 0)
		return EMS_CONTINUE;

	buf = buf_rd(&sess->buf_in);

	if ((*buf & 0x00ff) == 0x01) {
		if (buf_len(&sess->buf_in) < len)
			return EMS_CONTINUE;

		ems_buffer_prefetch(&sess->buf_in, (ems_char *)&auth, len);

		auth.serial = ntohs(auth.serial);
		auth.reqid  = ntohs(auth.reqid); 

#ifdef DEBUG
		{
			struct in_addr addr;
			memset(&addr, 0, sizeof(addr));
		//	addr.s_addr = htonl(auth.ip);
			memcpy(&addr.s_addr, &auth.ip, 4);

			ems_l_trace("[portal]evt ty: %d, serial: 0x%x, reqid:0x%x, ip: %s err: %d, nattr: %d",
				auth.ty, auth.serial, auth.reqid, inet_ntoa(addr), auth.err, auth.n_attr);
		}
#endif
		total = sizeof(auth);

		rtn = ptl_parse_ctx(&vp, buf + len,  buf_len(&sess->buf_in) - len, auth.n_attr, EMS_YES);
		if (rtn < 0) {
			ems_queue_clear(&vp, portal_value_pair, entry, ptl_vp_destroy);
			return rtn;
		}
		total += rtn;

		rtn = ptl_process_evt(ptl, sess, &auth, &vp);
	} else {
		rtn = ptl_parse_ctx(&vp, buf,  buf_len(&sess->buf_in), -1, EMS_NO);
		if (rtn <= 0) {
			ems_queue_clear(&vp, portal_value_pair, entry, ptl_vp_destroy);
			return rtn;
		}

		total = rtn;

		ems_assert(h);
		h(ptl, sess, &vp);
	}

	ems_buffer_seek_rd(&sess->buf_in, total, EMS_BUFFER_SEEK_CUR);
	ems_buffer_refresh(&sess->buf_in);

	ems_queue_clear(&vp, portal_value_pair, entry, ptl_vp_destroy);

	return rtn;
}

static ems_int
ptl_recv_msg(ems_portal *ptl, ems_session *sess, ems_uint flg, ptl_recv_cb h)
{
	ems_int ret, again;

	again = EMS_YES;
recv_again:
	ret = sess_recv(sess, &sess->buf_in);
	if (ret <= 0) {
		switch (ret) {
		case -EAGAIN:
			again = EMS_NO;
			break;
		default:
			if (buf_len(&sess->buf_in) > 0) 
				ptl_process_rsp(ptl, sess, h);
			/* shutdown current sesson*/
			return EMS_ERR;
		}
	}

	do {
		ret = ptl_process_rsp(ptl, sess, h);

		switch (ret) {
		case EMS_BUFFER_INSUFFICIENT:
		case EMS_ERR:
			return EMS_ERR;

		case EMS_OK:
		case EMS_CONTINUE:
		default:
			break;
		}
	} while (ret != EMS_CONTINUE);

	if (again)
		goto recv_again;

	return EMS_OK;
}

static ems_int 
ptl_send_msg(ems_portal *ptl, ems_session *sess, ems_uint flg)
{
	ems_int ret;

	ems_assert(ems_flag_like(flg, EMS_EVT_WRITE));
	ret = sess_send(sess, &sess->buf);
	if (ret <= 0) {
		switch(ret) {
		case -EAGAIN:
			break;

		default:
			return EMS_ERR;
		}
	}

	if (buf_len(&sess->buf) <= 0)
		sess_event_set(sess, EMS_EVT_READ, ptl_evt_cb);

	return EMS_OK;
}


static ems_int ptl_reg_rsp(ems_portal *ptl, ems_session *sess, ems_queue *list)
{
	portal_value_pair *vp = NULL;

	vp = ptl_vp_find(list, ATTR_RESPONSE);

	if (vp && vp->val && !strcmp(vp->val, "register")) {
		return portal_change_status(ptl, st_normal);
	}

	ptl->lasterr = ERR_PORTAL_RESPONSE;
	return portal_change_status(ptl, st_err);
}

static ems_int portal_reg (ems_portal *ptl, ems_session *sess, ems_uint flg)
{
	if (ems_flag_like(flg, EMS_EVT_READ)) {
		if (ptl_recv_msg(ptl, sess, flg, ptl_reg_rsp) != EMS_OK) {
			ptl->lasterr = ERR_PORTAL_NETWORK;
			return portal_change_status(ptl, st_err);
		}

		return EMS_OK;
	}

	if (ems_flag_like(flg, EMS_EVT_WRITE)) {
		if (ptl_send_msg(ptl, sess, flg) != EMS_OK) {
			ptl->lasterr = ERR_PORTAL_NETWORK;
			return portal_change_status(ptl, st_err);
		}

		return EMS_OK;
	}

	ems_assert(0 && "never be here");
	return EMS_OK;
}

static ems_int ptl_normal_rsp(ems_portal *ptl, ems_session *sess, ems_queue *list)
{
	return EMS_OK;
}

static ems_int portal_normal(ems_portal *ptl, ems_session *sess, ems_uint flg)
{
	if (ems_flag_like(flg, EMS_EVT_READ)) {
		if (ptl_recv_msg(ptl, sess, flg, ptl_normal_rsp) != EMS_OK) {
			ptl->lasterr = ERR_PORTAL_NETWORK;
			return portal_change_status(ptl, st_err);
		}

		return EMS_OK;
	}

	if (ems_flag_like(flg, EMS_EVT_WRITE)) {
		if (ptl_send_msg(ptl, sess, flg) != EMS_OK) {
			ptl->lasterr = ERR_PORTAL_NETWORK;
			return portal_change_status(ptl, st_err);
		}

		return EMS_OK;
	}

	ems_assert(0 && "never be here");
	return EMS_OK;
}

static ems_int ptl_hb_rsp(ems_portal *ptl, ems_session *sess, ems_queue *list)
{
	portal_value_pair *vp = NULL;

	vp = ptl_vp_find(list, ATTR_RESPONSE);

	if (vp && vp->val && !strcmp(vp->val, "active")) {
		return portal_change_status(ptl, st_normal);
	}

	ptl->lasterr = ERR_PORTAL_RESPONSE;
	return portal_change_status(ptl, st_err);
}

static ems_int portal_hb (ems_portal *ptl, ems_session *sess, ems_uint flg)
{
	if (ems_flag_like(flg, EMS_EVT_READ)) {
		if (ptl_recv_msg(ptl, sess, flg, ptl_hb_rsp) != EMS_OK) {
			ptl->lasterr = ERR_PORTAL_NETWORK;
			return portal_change_status(ptl, st_err);
		}

		return EMS_OK;
	}

	if (ems_flag_like(flg, EMS_EVT_WRITE)) {
		if (ptl_send_msg(ptl, sess, flg) != EMS_OK) {
			ptl->lasterr = ERR_PORTAL_NETWORK;
			return portal_change_status(ptl, st_err);
		}

		return EMS_OK;
	}

	ems_assert(0 && "never be here");
	return EMS_OK;
}

static ems_int ptl_clear_all_users(ems_portal *ptl)
{
	ems_queue   *p, *q;
	portal_user *user;

	ems_queue_foreach_safe(&ptl->users, p, q) {

		user = ems_container_of(p, portal_user, entry);
		if (ems_flag_like(user->flg, EMS_FLG_ONLINE))
		{
			json_object  *jobj;
			jobj = json_object_new_object();
			json_object_object_add(jobj, "username", json_object_new_string(str_text(&user->name)));
			json_object_object_add(jobj, "userip",   json_object_new_string(str_text(&user->ip)));
			json_object_object_add(jobj, "usermac",  json_object_new_string(ptl_mac_to_radius(&user->mac)));
			ems_send_message(ty_portal,  ty_radius,  EMS_APP_CMD_RADIUS_LOGOUT, jobj);

			json_object_put(jobj);
		}

		ems_queue_remove(&user->entry);
		ptl_user_destroy(user);
	}

	return EMS_OK;
}

static ems_int portal_stopped(ems_portal *ptl, ems_session *sess, ems_uint flg)
{
	sess = ptl->sess;
	if (sess) {
		ems_session_shutdown_and_destroy(sess);
		ptl->sess = NULL;
	}

	return ptl_clear_all_users(ptl);
}

static ems_int portal_err(ems_portal *ptl, ems_session *sess, ems_uint flg)
{
	sess = ptl->sess;

	if (sess) {
		sess_event_cancel(sess);
		sess_timeout_cancel(sess);
		ems_l_trace("[portal]shutdown session(%d) with [%s]", 
				ems_sock_fd(&sess->sock),
				ems_sock_addr(&sess->sock));
		ems_sock_close(&sess->sock);
		sess_timeout_set_sorted(sess, PORTAL_ERROR_TIMEOUT, ptl_timeout_cb);
	}

	ems_send_message(ty_portal, ty_radius, EMS_APP_RADIUS_STOP, NULL);

	return ptl_clear_all_users(ptl);
}

static ems_int portal_retry_send(ems_portal *ptl, ems_session *sess, ems_timeout *to)
{
	ptl->retry_times--;
	if (ptl->retry_times) {
		ems_buffer_seek_rd(&sess->buf_out, 0, EMS_BUFFER_SEEK_SET);
		sess_event_set(sess, EMS_EVT_WRITE, ptl_evt_cb);
		sess_timeout_set_sorted(sess, ptl->retry_timeout * 1000, ptl_timeout_cb);
		return EMS_OK;
	}

	ptl->lasterr = ERR_PORTAL_NETWORK;
	return portal_change_status(ptl, st_err);
}

static ems_int portal_to_reg(ems_portal *ptl, ems_session *sess, ems_timeout *to)
{
	return portal_retry_send(ptl, sess, to);
}

static ems_int portal_to_normal(ems_portal *ptl, ems_session *sess, ems_timeout *to)
{
	ptl->reg--;
	if (ptl->reg > 0)
		return portal_change_status(ptl, st_hb);

	return portal_change_status(ptl, st_reg);
}

static ems_int portal_to_hb(ems_portal *ptl, ems_session *sess, ems_timeout *to)
{
	return portal_retry_send(ptl, sess, to);
}

static ems_int portal_to_err(ems_portal *ptl, ems_session *sess, ems_timeout *to)
{
	return portal_change_status(ptl, st_start);
}

static ems_int portal_fill_reg_info(ems_portal *ptl, ems_session *sess)
{
	ems_queue vp;
	ems_uint  tmp;

	ems_queue_init(&vp);

	tmp = 2;
	ptl_vp_append(&vp, ATTR_PKGTYPE, (ems_cchar *)&tmp,  -1);
	ptl_vp_append(&vp, ATTR_AUTHTYPE, PORTAL_AUTH_TYPE,   -1);
	ptl_vp_append(&vp, ATTR_NASNAME,  core_sn(), -1);

	ptl_pack_list(&vp, &sess->buf);

	ems_queue_clear(&vp, portal_value_pair, entry, ptl_vp_destroy);

	return EMS_OK;
}

static ems_int ptl_status_into_reg(ems_portal *ptl)
{
	ems_session *sess = ptl->sess;

	ems_assert(ptl && ptl->sess && sess);

	ems_buffer_refresh(&sess->buf_out);
	portal_fill_reg_info(ptl, sess);

	ptl->retry_times   = PORTAL_RETRY_TIMES;
	ptl->retry_timeout = PORTAL_RETRY_TIMEOUT;
	ptl->lasterr       = 0;

	ptl->reg = ptl->reg_period / ptl->hb_period;

	sess_event_set(sess, EMS_EVT_WRITE, ptl_evt_cb);
	sess_timeout_set_sorted(sess, ptl->retry_timeout * 1000, ptl_timeout_cb);

	return EMS_OK;
}

static ems_int ptl_status_into_normal(ems_portal *ptl)
{
	ems_session *sess = ptl->sess;

	sess_event_cancel(sess);
	sess_event_set(sess, EMS_EVT_READ, ptl_evt_cb);

	if (ptl->reg > 0) {
		sess_timeout_set_sorted(sess, ptl->hb_period * 1000, ptl_timeout_cb);
	} else {
		ems_int     msecs;
		msecs = ptl->reg_period % ptl->hb_period;
		if (msecs <= 0)
			msecs = ptl->hb_period; 

		sess_timeout_set_sorted(sess, msecs * 1000, ptl_timeout_cb);
	}

	return EMS_OK;
}

static ems_int portal_fill_hb_info(ems_portal *ptl, ems_session *sess)
{
	ems_queue vp;
	ems_uint  tmp;

	ems_queue_init(&vp);

	tmp = 1;
	ptl_vp_append(&vp, ATTR_PKGTYPE, (ems_cchar *)&tmp,  -1);
	ptl_vp_append(&vp, ATTR_AUTHTYPE, PORTAL_AUTH_TYPE,   -1);
	ptl_vp_append(&vp, ATTR_NASNAME,  core_sn(), -1);

	ptl_pack_list(&vp, &sess->buf);
	ems_queue_clear(&vp, portal_value_pair, entry, ptl_vp_destroy);

	return EMS_OK;
}


static ems_int ptl_status_into_hb(ems_portal *ptl)
{
	ems_session *sess = ptl->sess;

	ems_assert(ptl && ptl->sess && sess);

	ems_buffer_refresh(&sess->buf_out);
	portal_fill_hb_info(ptl, sess);

	ptl->retry_times   = PORTAL_RETRY_TIMES;
	ptl->retry_timeout = PORTAL_RETRY_TIMEOUT;
	ptl->lasterr       = 0;

	sess_event_set(sess, EMS_EVT_WRITE, ptl_evt_cb);
	sess_timeout_set_sorted(sess, ptl->retry_timeout * 1000, ptl_timeout_cb);

	return EMS_OK;
}

ems_int portal_change_status(ems_portal *ptl, ems_status st)
{
	ems_l_trace("[portal] change  status: %s >> %s",
			ems_status_str(ptl->st), ems_status_str(st));

	ptl->st = st;

	switch(ptl->st) {
	case st_start:
	case st_stopped:
	case st_err:
		return ptl_evt_run(ptl, NULL, 0);

	case st_reg:
		return ptl_status_into_reg(ptl);

	case st_normal:
		return ptl_status_into_normal(ptl);

	case st_hb:
		return ptl_status_into_hb(ptl);

	default:
		break;
	}

	return EMS_OK;
}

ems_int  ptl_user_ntf_logout(ems_portal *ptl, portal_user *user)
{
	ems_queue  vp;
	portal_auth_hdr  auth;

	if (user) {
		ems_queue_init(&vp);

		ems_l_trace("ntf logout: current st: 0x%x", user->st);

		switch(user->st) {
		case portal_user_st_aff_ack_auth:
		case portal_user_st_ack_auth:
			break;

		default:
			return EMS_OK;

		}

		memset(&auth, 0, sizeof(auth));

		ems_flag_unset(user->flg, EMS_FLG_ONLINE);
		user->st = portal_user_st_ntf_logout;
		auth.ver    = 1;
		auth.ty     = portal_ty_ntf_logout;
		auth.serial = htons(user->serial);
		auth.reqid  = htons(user->reqid);
		auth.auth_ty= user->auth_ty;
		auth.ip     = (ems_uint) inet_addr(str_text(&user->ip));
		auth.port   = htons(0);
		auth.n_attr = 0;
		auth.err    = 0;
		{
			ems_char   mac[8];
			ems_str2bin(str_text(&user->mac), mac, 8); 
			ptl_vp_append(&vp, ATTR_USERMAC, mac, 6);
		}

		ptl_evt_ack(ptl, user, &auth, &vp);
		ems_queue_clear(&vp, portal_value_pair, entry, ptl_vp_destroy);
	}

	return EMS_OK;
}

ems_int  ptl_user_auth_rsp(ems_portal *ptl, portal_user *user, ems_int err)
{
	portal_auth_hdr  auth;

	if (user) {
		memset(&auth, 0, sizeof(auth));

		ems_l_trace("auth rsp: current st: 0x%x", user->st);

		if (user->st != portal_user_st_req_auth)
			return EMS_OK;

		auth.ver    = 1;
		auth.ty     = portal_ty_ack_auth;
		auth.serial = htons(user->serial);
		auth.reqid  = htons(user->reqid);
		auth.auth_ty= user->auth_ty;
		auth.ip     = (ems_uint) inet_addr(str_text(&user->ip));
		auth.port   = htons(0);
		auth.n_attr = 0;
		auth.err    = err;
		if (err) auth.err = 4;

		user->st = portal_user_st_ack_auth;
		if (!err)
			ems_flag_set(user->flg, EMS_FLG_ONLINE);

		return ptl_evt_ack(ptl, user, &auth, NULL);
	}

	return EMS_OK;
}

ems_int  ptl_current_address(ems_portal *ptl)
{
	socklen_t  len;
	struct sockaddr_in addr;

	if (!ptl->sess)
		return 0;

	len = sizeof(addr);
	memset(&addr, 0, sizeof(addr));

	if (ems_sock_fd(&ptl->sess->sock) <= 0)
		return 0;

	if (getpeername(ems_sock_fd(&ptl->sess->sock), (struct sockaddr *)&addr, &len))
		return 0;

	return (ems_int) addr.sin_addr.s_addr;
}
