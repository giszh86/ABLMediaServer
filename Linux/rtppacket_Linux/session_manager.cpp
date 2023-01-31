#include <boost/make_shared.hpp>

#if (defined _WIN32 || defined _WIN64)

#include <boost/thread/lock_guard.hpp>

#endif

#include "session_manager.h"


rtp_session_ptr rtp_session_manager::malloc(rtp_packet_callback cb, void* userdata)
{
	rtp_session_ptr p;

	try
	{
		p = boost::make_shared<rtp_session_packet>(cb, userdata);
	}
	catch (const std::bad_alloc& /*e*/)
	{	
	}
	catch (...)
	{
	}
	
	return p;
}

void rtp_session_manager::free(rtp_session_ptr p)
{
}

bool rtp_session_manager::push(rtp_session_ptr s)
{
	if (!s)
	{
		return false;
	}

#if (defined _WIN32 || defined _WIN64)

	boost::lock_guard<boost::mutex> lg(m_sesmapmtx);

#else

	auto_lock::al_lock<auto_lock::al_spin> al(m_sesmapspin);

#endif

	std::pair<boost::unordered_map<uint32_t, rtp_session_ptr>::iterator, bool> ret = m_sessionmap.insert(std::make_pair(s->get_id(), s));

	return ret.second;
}

bool rtp_session_manager::pop(uint32_t h)
{
#if (defined _WIN32 || defined _WIN64)

	boost::lock_guard<boost::mutex> lg(m_sesmapmtx);

#else

	auto_lock::al_lock<auto_lock::al_spin> al(m_sesmapspin);

#endif

	boost::unordered_map<uint32_t, rtp_session_ptr>::iterator it = m_sessionmap.find(h);
	if (m_sessionmap.end() != it)
	{
		m_sessionmap.erase(it);

		return true;
	}

	return false;
}

rtp_session_ptr rtp_session_manager::get(uint32_t h)
{
#if (defined _WIN32 || defined _WIN64)

	boost::lock_guard<boost::mutex> lg(m_sesmapmtx);

#else

	auto_lock::al_lock<auto_lock::al_spin> al(m_sesmapspin);

#endif

	rtp_session_ptr s;

	boost::unordered_map<uint32_t, rtp_session_ptr>::iterator it = m_sessionmap.find(h);
	if (m_sessionmap.end() != it)
	{
		s = it->second;
	}

	return s;

}