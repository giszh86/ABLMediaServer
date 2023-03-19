#ifndef _UDP_SESSION_MANAGER_H_
#define _UDP_SESSION_MANAGER_H_


#include "udp_session.h"
#include "auto_lock.h"

#ifdef USE_BOOST
#include <boost/serialization/singleton.hpp>
#include <boost/unordered_map.hpp>
#else
#include <unordered_map>
#include "HSingleton.h"
#endif
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
#ifdef USE_BOOST
	boost::unordered_map<NETHANDLE, udp_session_ptr> m_sessions;
#else
	std::map<NETHANDLE, udp_session_ptr> m_sessions;
#endif

#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_mutex m_mutex;
#else
	auto_lock::al_spin m_mutex;
#endif
};

#ifdef USE_BOOST
typedef boost::serialization::singleton<udp_session_manager> udp_session_manager_singleton;

#else
#define udp_session_manager_singleton HSingletonTemplatePtr<udp_session_manager>::Instance()
#endif


#endif

