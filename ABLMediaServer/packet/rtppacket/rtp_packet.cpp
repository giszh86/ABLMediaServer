#include "rtp_packet.h"
#include "session_manager_rtppacket.h"


 int32_t rtp_packet_start(rtp_packet_callback cb, void* userdata, uint32_t* h)
{
	if (!cb || !h)
	{
		return e_rtppkt_err_invalidparam;
	}

	*h = 0;

	rtp_session_ptr s = gblRtppacketMgrGet->malloc(cb, userdata);
	if (!s)
	{
		return e_rtppkt_err_mallocsessionerror;
	}

	if (!gblRtppacketMgrGet->push(s))
	{
		return e_rtppkt_err_managersessionerror;
	}

	*h = s->get_id();

	return e_rtppkt_err_noerror;

}

int32_t rtp_packet_stop(uint32_t h)
{
	if (!gblRtppacketMgrGet->pop(h))
	{
		return e_rtppkt_err_invalidsessionhandle;
	}

	return e_rtppkt_err_noerror;
}

 int32_t rtp_packet_input(_rtp_packet_input* input)
{
	if (!input || !input->data || (0 == input->datasize))
	{
		return e_rtppkt_err_invalidparam;
	}

	rtp_session_ptr s = gblRtppacketMgrGet->get(input->handle);

	if (!s)
	{
		return e_rtppkt_err_notfindsession;
	}

	return s->handle(input); 
}

 int32_t rtp_packet_setsessionopt(_rtp_packet_sessionopt* opt)
{
	if (!opt)
	{
		return e_rtppkt_err_invalidparam;
	}

	rtp_session_ptr s = gblRtppacketMgrGet->get(opt->handle);

	if (!s)
	{
		return e_rtppkt_err_notfindsession;
	}

	return s->set_option(opt);
}

 int32_t rtp_packet_resetsessionopt(_rtp_packet_sessionopt* opt)
{
	if (!opt)
	{
		return e_rtppkt_err_invalidparam;
	}

	rtp_session_ptr s = gblRtppacketMgrGet->get(opt->handle);

	if (!s)
	{
		return e_rtppkt_err_notfindsession;
	}

	return s->reset_option(opt);
}