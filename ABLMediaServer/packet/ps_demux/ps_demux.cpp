#include "ps_demux.h"
#include "demux_manager.h"

int32_t ps_demux_start(ps_demux_callback cb, void* userdata, int32_t mode, uint32_t* h)
{
	if (!cb || !h)
	{
		return e_ps_dux_err_invalidparameter;
	}

	*h = 0;
	ps_demux_ptr p = gblDemuxdMgrGet->malloc(cb, userdata, mode);
	if (!p)
	{
		return e_ps_dux_err_mallocdemuxerror;
	}

	if (!gblDemuxdMgrGet->push(p))
	{
		return e_ps_dux_err_managerdemuxerror;
	}

	*h = p->get_id();

	return e_ps_dux_err_noerror;

}

int32_t ps_demux_stop(uint32_t h)
{
	if (!gblDemuxdMgrGet->pop(h))
	{
		return e_ps_dux_err_invalidhandle;
	}

	return e_ps_dux_err_noerror;
}

int32_t ps_demux_input(uint32_t h, uint8_t* data, uint32_t datasize)
{
	if (!data || (0 == datasize))
	{
		return e_ps_dux_err_invalidparameter;
	}

	ps_demux_ptr p = gblDemuxdMgrGet->get(h);
	if (!p)
	{
		return e_ps_dux_err_invalidhandle;
	}

	return p->handle(data, datasize);
}