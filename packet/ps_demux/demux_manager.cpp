#include <boost/make_shared.hpp>

#if (defined _WIN32 || defined _WIN64)

#include <boost/thread/lock_guard.hpp>

#endif

#include "demux_manager.h"

ps_demux_ptr demux_manager::malloc(ps_demux_callback cb, void* userdata, int32_t mode)
{
	ps_demux_ptr p;

	try
	{
		p = boost::make_shared<ps_demux>(cb, userdata, mode);
	}
	catch (const std::bad_alloc& /*e*/)
	{
	}
	catch (...)
	{
	}

	return p;
}

void demux_manager::free()
{
}

bool demux_manager::push(ps_demux_ptr p)
{
	if (!p)
	{
		return false;
	}

#if (defined _WIN32 || defined _WIN64)

	boost::lock_guard<boost::mutex> lg(m_duxmtx);

#else

	auto_lock::al_lock<auto_lock::al_spin> al(m_duxspin);

#endif

	std::pair<boost::unordered_map<uint32_t, ps_demux_ptr>::iterator, bool> ret = m_duxmap.insert(std::make_pair(p->get_id(), p));

	return ret.second;
}

bool demux_manager::pop(uint32_t h)
{
#if (defined _WIN32 || defined _WIN64)

	boost::lock_guard<boost::mutex> lg(m_duxmtx);

#else

	auto_lock::al_lock<auto_lock::al_spin> al(m_duxspin);

#endif

	boost::unordered_map<uint32_t, ps_demux_ptr>::iterator it = m_duxmap.find(h);
	if (m_duxmap.end() != it)
	{
		m_duxmap.erase(it);

		return true;
	}

	return false;
}

ps_demux_ptr demux_manager::get(uint32_t h)
{
#if (defined _WIN32 || defined _WIN64)

	boost::lock_guard<boost::mutex> lg(m_duxmtx);

#else

	auto_lock::al_lock<auto_lock::al_spin> al(m_duxspin);

#endif

	ps_demux_ptr p;

	boost::unordered_map<uint32_t, ps_demux_ptr>::iterator it = m_duxmap.find(h);
	if (m_duxmap.end() != it)
	{
		p = it->second;
	}

	return p;
}