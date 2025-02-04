#include <boost/bind.hpp>
#include "client_manager.h"
#include "libnet_error.h"
#include "identifier_generator.h"

client::client(boost::asio::io_context& ioc,
	NETHANDLE srvid,
	read_callback fnread,
	close_callback fnclose,
	bool autoread)
	: m_srvid(srvid)
	, m_id(generate_identifier())
	, m_socket(ioc)
	, m_fnread(fnread)
	, m_fnclose(fnclose)
	, m_fnconnect(NULL)
	, m_closeflag(false)
	, m_timer(ioc)
	, m_autoread(autoread)
	, m_inreading(false)
	, m_usrreadbuffer(NULL)
	, m_onwriting(false)
	, m_currwriteaddr(NULL)
	, m_currwritesize(0)
	
{
}

client::~client(void)
{
	recycle_identifier(m_id);
}

int32_t client::run()
{
	if (!m_autoread)
	{
		return e_libnet_err_climanualread;
	}

	boost::system::error_code ec;
	boost::asio::ip::tcp::no_delay no_delay_option(true);
	m_socket.set_option(no_delay_option, ec);

	//设置接收，发送缓冲区
	int  nRecvSize = 1024 * 1024 * 2;
	boost::asio::socket_base::send_buffer_size    SendSize_option(nRecvSize); //定义发送缓冲区大小
	boost::asio::socket_base::receive_buffer_size RecvSize_option(nRecvSize); //定义接收缓冲区大小
	m_socket.set_option(SendSize_option); //设置发送缓存区大小
	m_socket.set_option(RecvSize_option); //设置接收缓冲区大小

	//设置立即关闭
	struct linger so_linger ;
	so_linger.l_onoff = 0;
	so_linger.l_linger = 1;
	setsockopt(m_socket.native_handle(), SOL_SOCKET, SO_LINGER, (const char*)&so_linger, sizeof(so_linger)); //设置立即关闭

	m_socket.async_read_some(boost::asio::buffer(m_readbuff, CLIENT_MAX_RECV_BUFF_SIZE),
		boost::bind(&client::handle_read,
			shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));

	return e_libnet_err_noerror;
}

int32_t client::connect(int8_t* remoteip,
	uint16_t remoteport,
	int8_t* localip,
	uint16_t localport,
	bool blocked,
	connect_callback fnconnect,
	uint32_t timeout)
{
	boost::system::error_code err;
	boost::asio::ip::address remoteaddr = boost::asio::ip::address::from_string(reinterpret_cast<char*>(remoteip), err);
	if (err)
	{
		return e_libnet_err_cliinvalidip;
	}

	boost::asio::ip::tcp::endpoint srvep(remoteaddr, remoteport);

	//open socket
	if (!m_socket.is_open())
	{
		m_socket.open(remoteaddr.is_v4() ? boost::asio::ip::tcp::v4() : boost::asio::ip::tcp::v6(), err);
		if (err)
		{
			return e_libnet_err_cliopensock;
		}
	}

	//set callback function
	m_fnconnect = fnconnect;

	//bind local address
	if ((localip && (0 != strcmp(reinterpret_cast<char*>(localip), ""))) || (localport > 0))
	{
		boost::asio::ip::address localaddr;
		if ((localip && (0 != strcmp(reinterpret_cast<char*>(localip), ""))))
		{
			localaddr = boost::asio::ip::address::from_string(reinterpret_cast<char*>(localip), err);
			if (err)
			{
				close();
				return e_libnet_err_cliinvalidip;
			}
		}

		boost::asio::ip::tcp::endpoint localep(localaddr, localport);
		m_socket.bind(localep, err);
		if (err)
		{
			close();
			return e_libnet_err_clibind;
		}
	}

	//set option
	boost::asio::socket_base::reuse_address reuse_address_option(true);
	m_socket.set_option(reuse_address_option, err);
	if (err)
	{
		close();
		return e_libnet_err_clisetsockopt;
	}
	
	boost::asio::socket_base::send_buffer_size send_buffer_size_option(LISTEN_SEND_BUFF_SIZE);
	m_socket.set_option(send_buffer_size_option, err);
	if (err)
	{
		close();
		return e_libnet_err_clisetsockopt;
	}

	boost::asio::socket_base::receive_buffer_size recv_buffer_size_option(LISTEN_RECV_BUFF_SIZE);
	m_socket.set_option(recv_buffer_size_option, err);
	if (err)
	{
		close();
		return e_libnet_err_clisetsockopt;
	}
	

	//connect timeout
	if (timeout > 0)
	{
		m_timer.expires_from_now(boost::posix_time::milliseconds(timeout));
		m_timer.async_wait(boost::bind(&client::handle_connect_timeout, shared_from_this(), boost::asio::placeholders::error));
	}

	//connect
	if (blocked)
	{
		m_socket.connect(srvep, err);
		m_timer.cancel();
		if (!err)
		{
			run();
			return e_libnet_err_noerror;
		}
		else
		{
			close();
			return e_libnet_err_cliconnect;
		}
	}
	else //sync connect
	{
		m_socket.async_connect(srvep, boost::bind(&client::handle_connect, shared_from_this(), boost::asio::placeholders::error));
		return e_libnet_err_noerror;
	}
}

void client::handle_write(const boost::system::error_code& ec, size_t transize)
{
	if (ec)
	{
		if (client_manager_singleton::get_mutable_instance().pop_client(get_id()))
		{
			if (m_fnclose)
			{
				m_fnclose(get_server_id(), get_id());
			}
		}

		return;
	}
	
	m_circularbuff.read_commit(m_currwritesize);
	m_currwriteaddr = NULL;
	m_currwritesize = 0;

#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_autowrmtx);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_autowrmtx);
#endif
	m_onwriting = write_packet();
}

void client::handle_read(const boost::system::error_code& ec, size_t transize)
{
	if (ec)
	{
		if (client_manager_singleton::get_mutable_instance().pop_client(get_id()))
		{
			if (m_fnclose)
			{
				m_fnclose(get_server_id(), get_id());
			}
		}

		return;
	}

	if (m_autoread)
	{
		if (m_fnread)
		{
			m_fnread(get_server_id(), get_id(), m_readbuff, static_cast<uint32_t>(transize), NULL);
		}

		m_socket.async_read_some(boost::asio::buffer(m_readbuff, CLIENT_MAX_RECV_BUFF_SIZE),
			boost::bind(&client::handle_read,
				shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
	}
	else
	{
		if (m_fnread)
		{
			m_fnread(get_server_id(), get_id(), m_usrreadbuffer, static_cast<uint32_t>(transize), NULL);
		}

		m_usrreadbuffer = NULL;
		m_inreading = false;
	}
}

void client::handle_connect(const boost::system::error_code& ec)
{
	m_timer.cancel();

	if (ec)
	{
		if (client_manager_singleton::get_mutable_instance().pop_client(get_id()))
		{
			if (m_fnconnect)
			{
				m_fnconnect(get_id(), 0);
			}
		}
	}
	else
	{
		if (m_fnconnect)
		{
			m_fnconnect(get_id(), 1);
		}

		run();	
	}
}

int32_t client::read(uint8_t* buffer,
	uint32_t* buffsize,
	bool blocked,
	bool certain)
{
#ifdef LIBNET_MULTI_THREAD_RECV
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_readmtx);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_readmtx);
#endif
#endif

	if (!buffer || !buffsize || (0 == *buffsize))
	{
		return  e_libnet_err_invalidparam;
	}

	if (m_autoread)
	{
		return e_libnet_err_cliautoread;
	}

	uint32_t readsize = 0;
	if (!m_socket.is_open())
	{
		return e_libnet_err_clisocknotopen;
	}

	if (blocked)
	{
		boost::system::error_code err;
		if (certain)
		{
			readsize = static_cast<uint32_t>(boost::asio::read(m_socket, boost::asio::buffer(buffer, *buffsize), err));
			if (err || (0 == readsize))
			{
				*buffsize = 0;
				client_manager_singleton::get_mutable_instance().pop_client(get_id());
				return e_libnet_err_clireaddata;
			}
			else
			{
				return e_libnet_err_noerror;
			}
		}
		else
		{
			readsize = static_cast<uint32_t>(m_socket.read_some(boost::asio::buffer(buffer, *buffsize), err));
			if (err || (0 == readsize))
			{
				*buffsize = 0;
				client_manager_singleton::get_mutable_instance().pop_client(get_id());
				return e_libnet_err_clireaddata;
			}
			else
			{
				*buffsize = readsize;
				return e_libnet_err_noerror;
			}
		}
	}
	else
	{
		if (m_inreading)
		{
			return e_libnet_err_cliprereadnotfinish;
		}

		m_inreading = true;
		m_usrreadbuffer = buffer;

		if (certain)
		{
			boost::asio::async_read(m_socket, boost::asio::buffer(m_usrreadbuffer, *buffsize),
				boost::bind(&client::handle_read,
					shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
		else
		{
			m_socket.async_read_some(boost::asio::buffer(m_usrreadbuffer, *buffsize),
				boost::bind(&client::handle_read,
					shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}

		return e_libnet_err_noerror;
	}
}

int32_t client::write(uint8_t* data,
	uint32_t datasize,
	bool blocked)
{
#ifdef LIBNET_MULTI_THREAD_SEND
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_writemtx);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_writemtx);
#endif
#endif

	int32_t ret = e_libnet_err_noerror;
	int32_t datasize2 = datasize;

	if (!data || (0 == datasize))
	{
		return e_libnet_err_invalidparam;
	}

	if (!m_socket.is_open())
	{
		return e_libnet_err_clisocknotopen;
	}

	if (blocked)
	{
		boost::system::error_code ec;
		unsigned long nSendPos = 0, nSendRet = 0 ;

		while (datasize2 > 0)
		{//改成循环发送
			nSendRet = boost::asio::write(m_socket, boost::asio::buffer(data + nSendPos, datasize2), ec);

			if (!ec)
			{//发送没有出错
 				if (nSendRet > 0)
				{
					nSendPos  += nSendRet;
					datasize2 -= nSendRet;
				}
			}
			else//发送出错，立即跳出循环，否则会死循环
				break;
		}
		if (!ec)
		{
			return e_libnet_err_noerror;
		}
		else
		{
			client_manager_singleton::get_mutable_instance().pop_client(get_id());
			return e_libnet_err_cliwritedata;
		}
	}
	else
	{
		if (!m_circularbuff.is_init() && 
			!m_circularbuff.init(CLIENT_MAX_SEND_BUFF_SIZE))
		{
			return e_libnet_err_cliinitswritebuff;
		}

		if (datasize != m_circularbuff.write(data, datasize))
		{
			ret = e_libnet_err_cliwritebufffull;
		}

#ifdef LIBNET_USE_CORE_SYNC_MUTEX
		auto_lock::al_lock<auto_lock::al_mutex> al(m_autowrmtx);
#else
		auto_lock::al_lock<auto_lock::al_spin> al(m_autowrmtx);
#endif
		if (!m_onwriting)
		{
			m_onwriting = write_packet();
		}
	}

	return ret;
}

bool client::write_packet()
{
	m_currwriteaddr = m_circularbuff.try_read(CLIENT_PER_SEND_PACKET_SIZE, m_currwritesize);
	if (m_currwriteaddr && (m_currwritesize > 0))
	{
		boost::asio::async_write(m_socket, boost::asio::buffer(m_currwriteaddr, m_currwritesize),
			boost::bind(&client::handle_write,
				shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
		
		return true;
	}
	
	return false;
}

void client::close()
{
	if (!m_closeflag.exchange(true))
	{
		m_fnconnect = NULL;
		m_fnread = NULL;
		m_timer.cancel();

		if (m_socket.is_open())
		{
			boost::system::error_code ec;
			m_socket.close(ec);
		}
	}
}

void client::handle_connect_timeout(const boost::system::error_code& ec)
{
	if (!ec)
	{
		if (m_socket.is_open())
		{
			boost::system::error_code ec;
			m_socket.close(ec);
		}
	}
}