#include <boost/unordered_set.hpp>

#if (defined _WIN32 || defined _WIN64)
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#else
#include "auto_lock.h"
#endif

#include "common.h"

boost::unordered_set<uint32_t> g_identifier_setRtpDepacket;

#if (defined _WIN32 || defined _WIN64)

boost::mutex g_identifier_mutex;
#else

auto_lock::al_spin g_identifier_spinRtpDepacket;

#endif

uint32_t generate_identifier()
{
#if (defined _WIN32 || defined _WIN64)

	boost::lock_guard<boost::mutex> lg(g_identifier_mutex);

#else

	auto_lock::al_lock<auto_lock::al_spin> al(g_identifier_spinRtpDepacket);

#endif

	static uint32_t s_id = 1;
	boost::unordered_set<uint32_t>::iterator it;

	for (;;)
	{
		it = g_identifier_setRtpDepacket.find(s_id);
		if ((g_identifier_setRtpDepacket.end() == it) && (0 != s_id))
		{
			std::pair<boost::unordered_set<uint32_t>::iterator, bool> ret = g_identifier_setRtpDepacket.insert(s_id);
			if (ret.second)
			{
				break;	//useful
			}
		}
		else
		{
			++s_id;
		}
	}

	return s_id++;
}

void recycle_identifier(uint32_t id)
{
#if (defined _WIN32 || defined _WIN64)

	boost::lock_guard<boost::mutex> lg(g_identifier_mutex);

#else

	auto_lock::al_lock<auto_lock::al_spin> al(g_identifier_spinRtpDepacket);

#endif

	boost::unordered_set<uint32_t>::iterator it = g_identifier_setRtpDepacket.find(id);
	if (g_identifier_setRtpDepacket.end() != it)
	{
		g_identifier_setRtpDepacket.erase(it);
	}
}