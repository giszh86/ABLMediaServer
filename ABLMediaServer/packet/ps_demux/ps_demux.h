#pragma once


#include <stdint.h>

enum e_ps_demux_error
{
	e_ps_dux_err_noerror = 0,

	e_ps_dux_err_invalidparameter,

	e_ps_dux_err_mallocdemuxerror,

	e_ps_dux_err_managerdemuxerror,

	e_ps_dux_err_invalidhandle,

	e_ps_dux_err_invaliddata,

	e_ps_dux_err_notfindpsheader,

	e_ps_dux_err_datalenlessthanpsheader,

	e_ps_dux_err_checkpsstartcodefail,

	e_ps_dux_err_invalidstreamid,

	e_ps_dux_err_notfindconsumer,

	e_ps_dux_err_invalidpsheader,

	e_ps_dux_err_invalidpssystemheader,

	e_ps_dux_err_invalidpsm,

	e_ps_dux_err_invalidpes,

	e_ps_dux_err_invalidptsdtsflag,

	e_ps_dux_err_invalidnonmediapes,
};

enum e_ps_demux_mode
{
	e_ps_dux_timestamp = 0,

	e_ps_dux_packet,
};

struct _ps_demux_cb
{
	uint32_t handle;
	int32_t streamid;
	int32_t streamtype;
	void* userdata;
	uint64_t pts;
	uint64_t dts;
	uint8_t* data;
	uint32_t datasize;
};

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

	typedef void (*ps_demux_callback)(_ps_demux_cb* cb);

	 int32_t ps_demux_start(ps_demux_callback cb, void* userdata, int32_t mode, uint32_t* h);

	 int32_t ps_demux_stop(uint32_t h);

	 int32_t ps_demux_input(uint32_t h, uint8_t* data, uint32_t datasize);

#ifdef __cplusplus
}
#endif

