
#include "client_manager.h"

#ifdef USE_BOOST
#include <boost/make_shared.hpp>
#else
#include <memory>
#endif

struct client_deletor
{
	void operator()(client* cli)
	{
		if (cli)
		{
			client_manager_singleton->free_client(cli);
		}
	}
};

#ifdef USE_BOOST

client_manager::client_manager(void)
	: m_pool()
{
}
#else
client_manager::client_manager(void)
{
}

#endif


client_manager::~client_manager(void)
{
	pop_all_clients();
}
#ifdef USE_BOOST

client_ptr client_manager::malloc_client(boost::asio::io_context& ioc,
	NETHANDLE srvid,
	read_callback fnread,
	close_callback fnclose,
	bool autoread)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_poolmtx);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_poolmtx);
#endif

	client_ptr cli;
	cli.reset(m_pool.construct(ioc, srvid, fnread, fnclose, autoread), client_deletor());
	//cli.reset(m_pool.allocA(ioc), client_deletor());
	//cli->init(srvid, fnread, fnclose, autoread);
	return cli;
}
#else
client_ptr client_manager::malloc_client(asio::io_context& ioc,
	NETHANDLE srvid,
	read_callback fnread,
	close_callback fnclose,
	bool autoread)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_poolmtx);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_poolmtx);
#endif

	client_ptr cli;
	cli.reset(m_pool.construct(ioc, srvid, fnread, fnclose, autoread), client_deletor());
	//cli.reset(m_pool.allocA(ioc), client_deletor());
	//cli->init(srvid, fnread, fnclose, autoread);
	return cli;
}

#endif


void client_manager::free_client(client* cli)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_poolmtx);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_poolmtx);
#endif
	
	m_pool.destroy(cli);

}

bool client_manager::push_client(client_ptr& cli)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_climtx);
#else
	std::lock_guard<std::mutex> lock(m_climtx) ;
#endif

	if (cli)
	{
		auto ret = m_clients.insert(std::make_pair(cli->get_id(), cli));
		return ret.second;
	}

	return false;
}

bool client_manager::pop_client(NETHANDLE id)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_climtx);
#else
	std::lock_guard<std::mutex> lock(m_climtx);
#endif

	auto iter = m_clients.find(id);
	if (m_clients.end() != iter)
	{
		if (iter->second)
		{
			iter->second->close();
		}

		m_clients.erase(iter);

		return true;
	}

	return false;
}

void client_manager::pop_server_clients(NETHANDLE srvid)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_climtx);
#else
	std::lock_guard<std::mutex> lock(m_climtx);
#endif

	for (auto  iter = m_clients.begin(); m_clients.end() != iter;)
	{
		if (iter->second && (iter->second->get_server_id() == srvid))
		{
			iter->second->close();
			m_clients.erase(iter++);
		}
		else
		{
			++iter;
		}
	}
}

void client_manager::pop_all_clients()
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_climtx);
#else
	std::lock_guard<std::mutex> lock(m_climtx);
#endif

	for (auto iter: m_clients)
	{
		if (iter.second)
		{
			iter.second->close();
		}
	}

	m_clients.clear();
}

client_ptr client_manager::get_client(NETHANDLE id)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_climtx);
#else
	std::lock_guard<std::mutex> lock(m_climtx);
#endif

	client_ptr cli = nullptr  ;
	auto  iter = m_clients.find(id);
	if (m_clients.end() != iter)
	{
		cli = iter->second;
	}

	return cli;
}