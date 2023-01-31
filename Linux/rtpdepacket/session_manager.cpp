#include <boost/make_shared.hpp>

#if (defined _WIN32 || defined _WIN64)

#include <boost/thread/lock_guard.hpp>

#endif

#include "session_manager.h"

rtp_session_ptr rtp_session_manager::malloc(rtp_depacket_callback cb, void* userdata)
{
	rtp_session_ptr s;

	try
	{
		s = boost::make_shared<rtp_session>(cb, userdata);
	}
	catch (const std::bad_alloc& /*e*/)
	{
	}
	catch (...)
	{
	}

	return s;
}

void rtp_session_manager::free(rtp_session_ptr s)
{

}

bool rtp_session_manager::push(rtp_session_ptr s)
{
	if (!s)
	{
		return false;
	}

#if (defined _WIN32 || defined _WIN64)

	boost::lock_guard<boost::mutex> lg(m_sesMapMtx);

#else

	auto_lock::al_lock<auto_lock::al_spin> al(m_sesMapSpin);

#endif

	std::pair<boost::unordered_map<uint32_t, rtp_session_ptr>::iterator, bool> ret = m_sessionMap.insert(std::make_pair(s->get_id(), s));

	return ret.second;
}

bool rtp_session_manager::pop(uint32_t h)
{
#if (defined _WIN32 || defined _WIN64)

	boost::lock_guard<boost::mutex> lg(m_sesMapMtx);

#else

	auto_lock::al_lock<auto_lock::al_spin> al(m_sesMapSpin);

#endif

	boost::unordered_map<uint32_t, rtp_session_ptr>::iterator it = m_sessionMap.find(h);
	if (m_sessionMap.end() != it)
	{
		m_sessionMap.erase(it);

		return true;
	}

	return false;
}

rtp_session_ptr rtp_session_manager::get(uint32_t h)
{
#if (defined _WIN32 || defined _WIN64)

	boost::lock_guard<boost::mutex> lg(m_sesMapMtx);

#else

	auto_lock::al_lock<auto_lock::al_spin> al(m_sesMapSpin);

#endif

	rtp_session_ptr s;

	boost::unordered_map<uint32_t, rtp_session_ptr>::iterator it = m_sessionMap.find(h);
	if (m_sessionMap.end() != it)
	{
		s = it->second;
	}

	return s;
	
}