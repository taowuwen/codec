
#ifndef AUDIT_MSG_HEADER____NUM__
#define AUDIT_MSG_HEADER____NUM__

typedef enum _audit_msg_id_s {
	A_AUDIT_STOP  = 0,
	A_AUDIT_START = 1,
	A_AUDIT_LOG,
	A_AUDIT_NET_PKGS,
	A_AUDIT_FILE_FULL,
	A_AUDIT_FILE_TRUNCATE,
	A_AUDIT_FILE_UPLOAD_TRIGER,
	A_AUDIT_MAX
} audit_msgid;



/* for 16wifi */
#define MSG_E6WIFI_FTP		0x0501
#define MSG_E6WIFI_CITYCODE	0x0502

/*
   for others 
 */
#define MSG_USERINFO		0x0503
#define MSG_SETUSER		0x0504
#define MSG_DELUSER		0x0505
#define MSG_MODULE_CTRL		0x0506
#define MSG_MODULE_INFO		0x0507
#define MSG_MODULE_SET		0x0508
#define MSG_MODULE_GET		0x0509

/* for network update */
#define MSG_NETWORK_RELOAD	0x050a

#endif
