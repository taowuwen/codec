
#ifndef FWD_CM_WORKER_TESTING____H_
#define FWD_CM_WORKER_TESTING____H_



dt_int  t_worker_init(t_worker *worker);
dt_void t_worker_uninit(t_worker *worker);

dt_int  t_worker_start(t_worker *worker, dt_int spawn, dt_int sess);
dt_int  t_worker_stop(t_worker *worker);

#define worker_event(worker)	(&(worker)->event)



#endif
