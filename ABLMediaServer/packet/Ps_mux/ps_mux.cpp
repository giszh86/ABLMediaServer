#include "ps_mux.h"
#include "mux_manager.h"


int32_t ps_mux_start(_ps_mux_init* init)
{
	if (!init || !init->cb || !init->h)
	{
		return e_ps_mux_err_invalidparameter;
	}

	*(init->h) = 0;

	ps_mux_ptr p = gblPsmuxdMgrGet->malloc(reinterpret_cast<ps_mux_callback>(init->cb), init->userdata, init->alignmode,
																init->ttmode, init->ttincre);
	if (!p)
	{
		return e_ps_mux_err_mallocmuxerror;
	}

	if (!gblPsmuxdMgrGet->push(p))
	{
		return e_ps_mux_err_managermuxerror;
	}

	*(init->h) = p->get_id();

	return e_ps_mux_err_noerror;
}

int32_t ps_mux_stop(uint32_t h)
{
	if (!gblPsmuxdMgrGet->pop(h))
	{
		return e_ps_mux_err_invalidhandle;
	}

	return e_ps_mux_err_noerror;
}

int32_t ps_mux_input(_ps_mux_input* input)
{
	if (!input || !input->data || (0 == input->datasize))
	{
		return e_ps_mux_err_invalidparameter;
	}

	ps_mux_ptr p = gblPsmuxdMgrGet->get(input->handle);
	if (!p)
	{
		return e_ps_mux_err_invalidhandle;
	}

	return p->handle(input);
}