
#ifndef EMS_MSG_HEADER___
#define EMS_MSG_HEADER___


#define ERR_EVT_BASE			0x0100
#define ERR_EVT_NOT_FOUND_DRIVER	(ERR_EVT_BASE + 0x01)


#define EMS_WEB_TICKET_TIMEOUT		120000
#define EMS_AC_TICKET_TIMEOUT		120000

#define EMS_SYNC_TIMEOUT_AAA_HB		60000
#define EMS_SYNC_TIMEOUT_LOGIC_HB	5000


#define MSG_ST_BASE_ERR			-0x0000
#define MSG_ST_ERR			-0x0001
#define MSG_ST_UPDATING			-0x0002
#define MSG_ST_MEM_ERR			-0x0003
#define MSG_ST_DB_ERR			-0x0004
#define MSG_ST_CANNOT_CONNECT_TO_AAA	-0x0005
#define MSG_ST_INVALID_ARG		-0x0006
#define MSG_ST_TICKET_INVALID		-0x0007
#define MSG_ST_MSG_UNKNOWN		-0x0008
#define MSG_ST_USER_OR_PASSWORD_ERROR	-0x0009
#define MSG_ST_PERMIT_DENIED		-0x000a
#define MSG_ST_VERIFY_TIMEOUT		-0x000b
#define MSG_ST_VERIFY_ERROR		-0x000c


#define MSG_ST_GRP_ERR			-0x0100
#define MSG_ST_GRP_NOT_FOUND		-0x0101
#define MSG_ST_GRP_DUP			-0x0102
#define MSG_ST_GRP_GRP_NOT_NULL		-0x0103
#define MSG_ST_GRP_AC_NOT_NULL		-0x0104
#define MSG_ST_GRP_ACTIVE_CODE_NOT_NULL -0x0105
#define MSG_ST_GRP_APP_NOT_NULL		-0x0106
#define MSG_ST_GRP_PROP_NOT_NULL	-0x0107

#define MSG_ST_AC_ERR			-0x0200
#define MSG_ST_AC_NOT_FOUND		-0x0201
#define MSG_ST_AC_MAC_ERROR		-0x0202

#define MSG_ST_ACTV_ERR			-0x0300
#define MSG_ST_ACTV_TOO_MUCH		-0x0301
#define MSG_ST_ACTV_NOT_FOUND		-0x0302

#define MSG_ST_APP_ERR			-0x0400
#define MSG_ST_APP_NOT_FOUND		-0x0401
#define MSG_ST_APP_TYPE_ERROR		-0x0402
#define MSG_ST_APP_DUP_NICK		-0x0403
#define MSG_ST_APP_GRP_NOT_NULL		-0x0404
#define MSG_ST_APP_AC_NOT_NULL		-0x0405
#define MSG_ST_APP_FILE_MISS		-0x0406
#define MSG_ST_APP_FILE_COULD_NOT_OPEN	-0x0407

#define MSG_ST_PROP_ERR			-0x0500
#define MSG_ST_PROP_NOT_FOUND		-0x0501
#define MSG_ST_PROP_DUP_NICK		-0x0502
#define MSG_ST_PROP_GRP_NOT_NULL	-0x0503
#define MSG_ST_PROP_AC_NOT_NULL		-0x0504



/*
   msg for ac
 */
#define AC_MSG_BASE			0x0000
#define AC_MSG_REGISTER			0x0001
#define AC_MSG_LOGOUT			0x0002
#define AC_MSG_TRYUSING			0x0003
#define AC_MSG_HB			0x0004
#define AC_MSG_APPLIST			0x0005
#define AC_MSG_DOWNLOAD			0x0006


ems_int ems_sock_send(ems_sock *, ems_buffer *buf);
ems_int ems_sock_read(ems_sock *, ems_buffer *buf);
ems_int ems_pack_rsp(ems_uint tag, ems_int st, ems_cchar *ctx, ems_int len, ems_buffer *buf);
ems_int ems_pack_req(ems_uint tag, ems_cchar *ctx, ems_int len, ems_buffer *buf);
ems_int ems_sock_be_server(ems_sock *sock);;
ems_int ems_setsock_rw_timeout(ems_int sockfd, ems_int msecs);


#endif
