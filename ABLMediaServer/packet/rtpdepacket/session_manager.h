#pragma once
#include <unordered_map>
#include <mutex>
#include "../../HSingleton.h"
#include "session_depacket.h"

class rtp_session_manager 
{
public:
	rtp_session_ptr malloc(rtp_depacket_callback cb, void* userdata);
	void free(rtp_session_ptr s);

	bool push(rtp_session_ptr s);
	bool pop(uint32_t h);
	rtp_session_ptr get(uint32_t h);

private:
	std::unordered_map<uint32_t, rtp_session_ptr> m_sessionMap;



	std::mutex m_sesMapMtx;


};


#define gblRtpDepacketMgrGet HSingletonTemplatePtr<rtp_session_manager>::Instance()
