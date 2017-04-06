#ifndef __PORTAL_H__
#define __PORTAL_H__

#pragma pack(push, 1)

typedef union _portal_auth_header_s portal_auth_hdr;
typedef struct _ems_portal_   ems_portal;

union _portal_auth_header_s {
	ems_uchar val[16];
	struct {
		ems_uchar	ver;
		ems_uchar	ty;
		ems_uchar       auth_ty; /* pap == 1 /chap == 0 */
		ems_uchar       rsrv;
		ems_ushort      serial;
		ems_ushort      reqid;
		ems_uint        ip;
		ems_ushort      port;
		ems_uchar       err;
		ems_uchar       n_attr;
	};
};

#pragma pack(pop)

typedef struct _portal_value_pair_s {
	ems_int    attr; /* msg id */
	ems_int    id;
	ems_int    ty;   /* int, string */
	ems_uint   lval;
	ems_char  *val;
	ems_queue  entry;
} portal_value_pair;

typedef struct _portal_user_s {
	ems_str    name;
	ems_str    pass;
	ems_str    mac;
	ems_str    ip;

	ems_ushort  serial;
	ems_ushort  reqid;
	ems_uchar   auth_ty;
	ems_uint    flg;

	ems_int     st;
	ems_timeout to;
	ems_buffer  buf_out;
	ems_int     retry_times;
	ems_int     retry_timeout;
	ems_portal  *ptl;

	ems_queue   entry;
} portal_user;

struct _ems_portal_ 
{
	ems_queue     users;
	ems_session  *sess;

	ems_str       addr;
	ems_int       port;
	ems_int       reg_period;
	ems_int       hb_period;
	ems_int       reg;

	ems_int       retry_times;
	ems_int       retry_timeout;
	ems_status    st;
	ems_int       lasterr;
};


ems_int portal_change_status(ems_portal *ptl, ems_status st);
portal_user *ptl_user_find(ems_portal *ptl, ems_cchar *ip);
ems_void     ptl_user_destroy(portal_user *user);

ems_int  ptl_user_ntf_logout(ems_portal *ptl, portal_user *user);
ems_int  ptl_user_auth_rsp(ems_portal *ptl, portal_user *user, ems_int err);
ems_int  ptl_current_address(ems_portal *ptl);


#endif  /* __PORTAL_H__ */
