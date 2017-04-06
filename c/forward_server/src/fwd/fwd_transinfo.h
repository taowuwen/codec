
#ifndef FWD_TRANSINFO_STATUS_HEADER___
#define FWD_TRANSINFO_STATUS_HEADER___


/*
for born time update
*/
dt_int fwd_trans_init(fwd_transinfo *trans);
dt_int fwd_trans_uninit(fwd_transinfo *trans);

/*
for speed and access time update
*/
dt_int fwd_trans_write(fwd_transinfo *trans, dt_int nbytes);
dt_int fwd_trans_read( fwd_transinfo *trans, dt_int nbytes);


/*
for modify time update
*/
dt_int fwd_trans_flush(fwd_transinfo *);



#endif
