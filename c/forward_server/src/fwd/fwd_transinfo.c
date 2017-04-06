
#include "fwd_main.h"
#include "fwd_transinfo.h"


dt_int fwd_trans_init(fwd_transinfo *trans)
{
	struct timeval tv;
	fwd_time       *tm;

	dt_assert(trans && "invalid arg");

	if (!trans)
		return FWD_ERR;

	memset(trans, 0, sizeof(fwd_transinfo));

	gettimeofday(&tv, NULL);
	tm = &trans->time;

	memcpy(&tm->access, &tv, sizeof(tv));
	memcpy(&tm->modify, &tv, sizeof(tv));
	memcpy(&tm->born,   &tv, sizeof(tv));

	return FWD_OK;
}

dt_int fwd_trans_uninit(fwd_transinfo *trans)
{
	return fwd_trans_init(trans);
}

/*
we need microseconds not miliseconds
*/
static off_t 
trans_time_diff(struct timeval *t1, struct timeval *t2)
{
	return
		(t1->tv_sec - t2->tv_sec) * 1000000 +
		(t1->tv_usec - t2->tv_usec);
}

static dt_void 
fwd_trans_update(fwd_transinfo *trans, dt_int nbytes)
{
	fwd_time         *tm;
	struct timeval    tv;
	off_t             diff;

	gettimeofday(&tv, NULL);

	tm = &trans->time;

	diff = trans_time_diff(&tv, &tm->access);
	dt_assert(diff >= 0);

	if (diff > 0)
		trans->real_time_speed = (nbytes * 1000000) / diff;

	memcpy(&tm->access, &tv, sizeof(tv));

	diff = trans_time_diff(&tv, &tm->modify);

	trans->total_bytes += nbytes;

	dt_assert(diff >= 0);

	if (diff > 0) {
		trans->average_speed = (trans->total_bytes * 1000000) / diff;
		trans->read_speed    = (trans->total_read  * 1000000) / diff;
		trans->write_speed   = (trans->total_write * 1000000) / diff;
	}
}

dt_int fwd_trans_write(fwd_transinfo *trans, dt_int nbytes)
{
	dt_assert(trans != NULL && nbytes >= 0);

	if (!trans)
		return FWD_ERR;

	trans->total_write += nbytes;

	fwd_trans_update(trans, nbytes);
	return FWD_OK;
}

dt_int fwd_trans_read( fwd_transinfo *trans, dt_int nbytes)
{
	dt_assert(trans != NULL && nbytes >= 0);

	if (!trans)
		return FWD_ERR;

	trans->total_read += nbytes;

	fwd_trans_update(trans, nbytes);

	return FWD_OK;
}


dt_int fwd_trans_flush(fwd_transinfo *trans)
{
	fwd_time       *tm;

	dt_assert(trans);

	trans->real_time_speed = 0;
	trans->total_bytes     = 0;
	trans->total_read      = 0;
	trans->read_speed      = 0;
	trans->write_speed     = 0;

	tm = &trans->time;

	gettimeofday(&tm->modify, NULL);

	return FWD_OK;
}

