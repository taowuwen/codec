
#ifndef FWD_UTILS_HEADER___
#define FWD_UTILS_HEADER___


dt_int fwd_cpucore();
dt_int fwd_pagesize();

dt_void fwd_printhex(dt_cchar *, dt_int len);
dt_cchar *fwd_public_addr();

#if 0
dt_cchar *fwd_public_addr_ipv6();
#endif

dt_int fwd_time_diff(struct timeval *t1, struct timeval *t2);



#endif
