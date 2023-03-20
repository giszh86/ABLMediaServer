#ifndef _SERVER_MANAGER_H_
#define _SERVER_MANAGER_H_


#include "server.h"
#include "auto_lock.h"

#ifdef USE_BOOST
#include <boost/unordered_map.hpp>
#include <boost/serialization/singleton.hpp>
#else
#include <map>

#endif
class server_manager
{
public:
	server_manager(void);
	~server_manager(void);

	bool push_server(server_ptr& s);
	bool pop_server(NETHANDLE id);
	void close_all_servers();
	server_ptr get_server(NETHANDLE id);

private:
	std::map<NETHANDLE, server_ptr> m_servers;
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_mutex m_mutex;
#else
	auto_lock::al_spin m_mutex;
#endif
};
#define server_manager_singleton HSingletonTemplatePtr<server_manager>::Instance()
//typedef boost::serialization::singleton<server_manager> server_manager_singleton;

#endif
