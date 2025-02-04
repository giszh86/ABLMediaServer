#include <boost/make_shared.hpp>
#include "client_manager.h"

struct client_deletor
{
	void operator()(client* cli)
	{
		if (cli)
		{
			client_manager_singleton::get_mutable_instance().free_client(cli);
		}
	}
};

client_manager::client_manager(void)
	: m_pool(CLIENT_POOL_OBJECT_COUNT, CLIENT_POOL_OBJECT_COUNT)
{
}

client_manager::~client_manager(void)
{
	pop_all_clients();
}

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

	return cli;
}

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
	auto_lock::al_lock<auto_lock::al_spin> al(m_climtx);
#endif

	if (cli)
	{
		std::pair<const_climapiter, bool> ret = m_clients.insert(std::make_pair(cli->get_id(), cli));
		return ret.second;
	}

	return false;
}

bool client_manager::pop_client(NETHANDLE id)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_climtx);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_climtx);
#endif

	const_climapiter iter = m_clients.find(id);
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
	auto_lock::al_lock<auto_lock::al_spin> al(m_climtx);
#endif

	for (const_climapiter iter = m_clients.begin(); m_clients.end() != iter;)
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
	auto_lock::al_lock<auto_lock::al_spin> al(m_climtx);
#endif

	for (const_climapiter iter = m_clients.begin(); iter != m_clients.end(); ++iter)
	{
		if (iter->second)
		{
			iter->second->close();
		}
	}

	m_clients.clear();
}

client_ptr client_manager::get_client(NETHANDLE id)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_climtx);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_climtx);
#endif

	client_ptr cli;
	const_climapiter iter = m_clients.find(id);
	if (m_clients.end() != iter)
	{
		cli = iter->second;
	}

	return cli;
}