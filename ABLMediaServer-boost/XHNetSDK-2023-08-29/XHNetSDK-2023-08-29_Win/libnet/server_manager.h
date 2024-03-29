#ifndef _SERVER_MANAGER_H_
#define _SERVER_MANAGER_H_

#include <boost/unordered_map.hpp>
#include <boost/serialization/singleton.hpp>
#include "server.h"
#include "auto_lock.h"

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
	typedef boost::unordered_map<NETHANDLE, server_ptr>::iterator servermapiter;
	typedef boost::unordered_map<NETHANDLE, server_ptr>::const_iterator const_servermapiter;

private:
	boost::unordered_map<NETHANDLE, server_ptr> m_servers;
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_mutex m_mutex;
#else
	std::mutex          m_climtx;
#endif
};
typedef boost::serialization::singleton<server_manager> server_manager_singleton;

#endif
