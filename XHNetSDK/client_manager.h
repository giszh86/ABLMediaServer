#ifndef _CLIENT_MANAGER_H_
#define _CLIENT_MANAGER_H_ 

#include "asio/detail/object_pool.hpp"
#include "client.h"
#include <iostream>
#include <memory>
#include <asio.hpp>
#include <asio/detail/object_pool.hpp>
#ifdef USE_BOOST
#include "unordered_object_pool.h"
#include <boost/unordered_map.hpp>
#include <boost/serialization/singleton.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#else
#include <map>
#include "HSingleton.h"
#include <memory>
#include "class_obj_pool.h"
#endif

#include "zzc_object_pool.h"

#define CLIENT_POOL_OBJECT_COUNT 1000
#define CLIENT_POOL_MAX_KEEP_COUNT 100 

#ifdef USE_BOOST
typedef simple_pool::unordered_object_pool<client> client_pool;
typedef boost::shared_ptr<client_pool> client_pool_ptr;
#else
typedef zzc::detail::object_pool<client> client_pool;
typedef std::shared_ptr<client> client_pool_ptr;
#endif



//class client_pool : public asio::detail::object_pool<client>
//{
//public:
//	using base_type = asio::detail::object_pool<std::shared_ptr<client>>;
//	using base_type::base_type;
//
//	template<typename... Args>
//	std::shared_ptr<client> alloc(asio::io_context& ioc, Args&&... args)
//	{
//		auto ptr = base_type::construct(std::forward<Args>(args)...);
//		ptr->socket().emplace(ioc);
//		return std::shared_ptr<client>(std::move(ptr), [this](client* p) { this->destroy(p); });
//	}
//
//	void destroy(client* p)
//	{
//	
//		if (p)
//		{
//			delete p;
//			p = nullptr;
//		}
//	
//	}
//public:
//	client* prev_;
//	client* next_;
//};

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
	client_pool m_pool;
#ifdef USE_BOOST
	typedef boost::unordered_map<NETHANDLE, client_ptr>::iterator climapiter;
	typedef boost::unordered_map<NETHANDLE, client_ptr>::const_iterator const_climapiter;
	boost::unordered_map<NETHANDLE, client_ptr> m_clients;
#else
	std::map<NETHANDLE, client_ptr> m_clients;
#endif



private:
	//client_pool m_pool;
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