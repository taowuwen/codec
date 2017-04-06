

#ifndef FWD_WITH_CM_HEADER___
#define FWD_WITH_CM_HEADER___


#define MSG_CM_REG		0x1501
#define MSG_CM_TICKET_NEW	0x1503
#define MSG_CM_TICKET_FIRED	0x1505
#define MSG_CM_HEARTBEAT	0x1506

dt_int fwd_cm_init(fwd_cm *);
dt_int fwd_cm_uninit(fwd_cm *);

dt_int fwd_cm_start(fwd_cm *);
dt_int fwd_cm_stop(fwd_cm *);

dt_int fwd_cm_ticket_fired(fwd_cm *, dt_cchar *ticket);



#endif
