#ifndef _SERVER_H_
#define _SERVER_H_


#include "client_manager.h"
#include "auto_lock.h"


#ifdef USE_BOOST
#include <boost/function.hpp>
#else
#include <functional>
#include <memory>
#endif


class server : public std::enable_shared_from_this<server>
{
public:
	server(asio::io_context& ioc,
		asio::ip::tcp::endpoint& ep,
		accept_callback fnaccept,
		read_callback fnread,
		close_callback fnclose,
		bool autoread);
	~server();

	int32_t run();
	void close();
	NETHANDLE get_id();

private:
	void start_accept();
	void handle_accept(client_ptr c, const boost::system::error_code& error);
	void disconnect_clients();

private:
	asio::ip::tcp::acceptor m_acceptor;
	asio::ip::tcp::endpoint m_endpoint;
	accept_callback m_fnaccept;
	read_callback m_fnread;
	close_callback m_fnclose;
	const bool m_autoread;
	NETHANDLE m_id;
};
#ifdef USE_BOOST
typedef boost::shared_ptr<server>  server_ptr;
#else
typedef std::shared_ptr<server>  server_ptr;
#endif



inline NETHANDLE server::get_id()
{
	return m_id;
}

inline void server::disconnect_clients()
{
	client_manager_singleton->pop_server_clients(get_id());
}

#endif