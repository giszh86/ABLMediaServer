#pragma once 

#ifdef USE_BOOST
#include <boost/serialization/singleton.hpp>
#include <boost/unordered_map.hpp>
#include "udp_session.h"
#include "auto_lock.h"

class udp_session_manager
{
public:
	udp_session_manager();
	~udp_session_manager();

	bool push_udp_session(udp_session_ptr& s);
	bool pop_udp_session(NETHANDLE id);
	void pop_all_udp_sessions();
	udp_session_ptr get_udp_session(NETHANDLE id);

private:
	boost::unordered_map<NETHANDLE, udp_session_ptr> m_sessions;
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_mutex m_mutex;
#else
	std::mutex          m_climtx;
#endif
};

typedef boost::serialization::singleton<udp_session_manager> udp_session_manager_singleton;



#else

#include <map>
#include "udp_session.h"
#include "auto_lock.h"
#include "HSingleton.h"
class udp_session_manager
{
public:
	udp_session_manager();
	~udp_session_manager();

	bool push_udp_session(udp_session_ptr& s);
	bool pop_udp_session(NETHANDLE id);
	void pop_all_udp_sessions();
	udp_session_ptr get_udp_session(NETHANDLE id);

private:
	std::map<NETHANDLE, udp_session_ptr> m_sessions;
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_mutex m_mutex;
#else
	std::mutex          m_climtx;
#endif
};

#define udp_session_manager_singleton HSingletonTemplatePtr<udp_session_manager>::Instance()


#endif
