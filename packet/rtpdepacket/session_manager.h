#ifndef _RTP_PACKET_DEPACKET_SESSION_MANAGER_H_
#define _RTP_PACKET_DEPACKET_SESSION_MANAGER_H_

#include <stdint.h>
#include <boost/serialization/singleton.hpp>
#include <boost/unordered/unordered_map.hpp>

#if (defined _WIN32 || defined _WIN64)

#include <boost/thread/mutex.hpp>

#else

#include "auto_lock.h"

#endif

#include "session.h"

class rtp_session_manager : public boost::serialization::singleton<rtp_session_manager>
{
public:
	rtp_session_ptr malloc(rtp_depacket_callback cb, void* userdata);
	void free(rtp_session_ptr s);

	bool push(rtp_session_ptr s);
	bool pop(uint32_t h);
	rtp_session_ptr get(uint32_t h);

private:
	boost::unordered_map<uint32_t, rtp_session_ptr> m_sessionMap;

#if (defined _WIN32 || defined _WIN64)

	boost::mutex m_sesMapMtx;

#else

	auto_lock::al_spin m_sesMapSpin;

#endif
};

#endif