#ifndef _CLIENT_MANAGER_H_
#define _CLIENT_MANAGER_H_ 


#include "client.h"
#include "unordered_object_pool.h"
#ifdef USE_BOOST
#include <boost/unordered_map.hpp>
#include <boost/serialization/singleton.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#else
#include <map>
#include "HSingleton.h"
#include <memory>
#endif


#define CLIENT_POOL_OBJECT_COUNT 1000
#define CLIENT_POOL_MAX_KEEP_COUNT 100 

#ifdef USE_BOOST
typedef simple_pool::unordered_object_pool<client> client_pool;
typedef boost::shared_ptr<client_pool> client_pool_ptr;
#else
typedef simple_pool::unordered_object_pool<client> client_pool;
typedef std::shared_ptr<client_pool> client_pool_ptr;
#endif


class client_manager
{
public:
	client_manager(void);
	~client_manager(void);

	client_ptr malloc_client(asio::io_context& ioc,
		NETHANDLE srvid,
		read_callback fnread,
		close_callback fnclose,
		bool autoread);
	void free_client(client* cli);
	bool push_client(client_ptr& cli);
	bool pop_client(NETHANDLE id);
	void pop_server_clients(NETHANDLE srvid);
	void pop_all_clients();
	client_ptr get_client(NETHANDLE id);

private:
#ifdef USE_BOOST
	typedef boost::unordered_map<NETHANDLE, client_ptr>::iterator climapiter;
	typedef boost::unordered_map<NETHANDLE, client_ptr>::const_iterator const_climapiter;
	boost::unordered_map<NETHANDLE, client_ptr> m_clients;
#else
	typedef std::map<NETHANDLE, client_ptr>::iterator climapiter;
	typedef std::map<NETHANDLE, client_ptr>::const_iterator const_climapiter;
	std::map<NETHANDLE, client_ptr> m_clients;
#endif



private:
	client_pool m_pool;
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_mutex m_poolmtx;
#else
	auto_lock::al_spin m_poolmtx;
#endif


#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_mutex m_climtx;
#else
	auto_lock::al_spin m_climtx;
#endif
};

#ifdef USE_BOOST
typedef boost::serialization::singleton<client_manager> client_manager_singleton;
#else
#define client_manager_singleton HSingletonTemplatePtr<client_manager>::Instance()
#endif




#endif