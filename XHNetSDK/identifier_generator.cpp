
#include "identifier_generator.h"
#include "auto_lock.h"

#ifdef USE_BOOST
#include <boost/unordered_set.hpp>
boost::unordered_set<NETHANDLE> g_idset;
#else
#include <unordered_set>
std::unordered_set<NETHANDLE> g_idset;
#endif



#ifdef LIBNET_USE_CORE_SYNC_MUTEX
auto_lock::al_mutex g_idmtx;
#else
auto_lock::al_spin g_idmtx;
#endif

NETHANDLE generate_identifier()
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(g_idmtx);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(g_idmtx);
#endif

	static NETHANDLE s_id = 1;
	//boost::unordered_set<NETHANDLE>::iterator it;

	for (;;)
	{
		auto it = g_idset.find(s_id);
		if ((g_idset.end() == it) && (0 != s_id))
		{
			auto ret = g_idset.insert(s_id);
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

void  recycle_identifier(NETHANDLE id)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(g_idmtx);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(g_idmtx);
#endif

	auto it = g_idset.find(id);
	if (g_idset.end() != it)
	{
		g_idset.erase(it);
	}
}