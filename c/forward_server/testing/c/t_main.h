
#ifndef TESTING_FWD_CM_HEADER____
#define TESTING_FWD_CM_HEADER____

#include "fwd.h"
#include "fwd_main.h"
#include "t_define.h"

dt_cchar *t_main_cm_server();
dt_int    t_main_cm_port();
struct sockaddr_in *t_main_cm_addr();

dt_int t_test(dt_uint flg);
dt_int t_main_hb();
dt_int t_main_retry();

off_t  t_main_file_size();


dt_int t_main_success_inc();
dt_int t_main_failed_inc();
dt_int t_main_fwd_times();
dt_cchar *t_main_fwd_args();


#endif
