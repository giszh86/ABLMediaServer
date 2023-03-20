

#include "io_context_pool.h"
#include "libnet_error.h"
#include "data_define.h"



#ifdef USE_BOOST
#include <boost/make_shared.hpp>
#include <boost/function.hpp>
#include <boost/ref.hpp>
#else
#include <memory>
#include <functional>
#endif

io_context_pool::io_context_pool()
	: m_nextioc(0)
	, m_threads(1)
	, m_isinit(false)
	, m_periocthread(IOC_POOL_PREDEFINE_PER_IOC_THREAS)
{
	
	m_threads.executor();
}

io_context_pool::~io_context_pool()
{
	close();
}

int32_t io_context_pool::init(uint32_t iocnum, uint32_t periocthread)
{
	if (is_init())
	{
		return e_libnet_err_noerror;
	}

	io_context_ptr ioc;
	work_ptr wo;

	for (uint32_t i = 0; i < iocnum; )
	{
		try
		{
#ifdef USE_BOOST
			ioc = boost::make_shared<asio::io_context>();
			wo = boost::make_shared<asio::io_context::work>(boost::ref(*ioc));

#else
			ioc = std::make_shared<asio::io_context>();
			wo = std::make_shared<asio::io_context::work>(*ioc);

#endif
		m_iocontexts.push_back(ioc);
			m_works.push_back(wo);

			++i;
		}
#ifdef USE_BOOST
		catch (const std::bad_alloc& e)
		{
			(void)e;
			boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
			continue;
		}
		catch (...)
		{
			boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
			continue;
		}
#else
		catch (const std::bad_alloc& e)
		{
			(void)e;
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			continue;
		}
		catch (...)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			continue;
		}
#endif

	}

	if (0 == periocthread)
	{
		m_periocthread = IOC_POOL_PREDEFINE_PER_IOC_THREAS;
	}
	else if (periocthread > IOC_POOL_MAX_PER_IOC_THREAS)
	{
		m_periocthread = IOC_POOL_MAX_PER_IOC_THREAS;
	}
	else
	{
		m_periocthread = periocthread;
	}

	if (0 == m_iocontexts.size())
	{
		m_isinit = false;
		return e_libnet_err_nonioc;
	}
	else
	{
		m_isinit = true;
		return e_libnet_err_noerror;
	}

	return e_libnet_err_noerror;
}

int32_t io_context_pool::run()
{
	int32_t ret = e_libnet_err_nonioc;
	thread_ptr t;
#ifdef USE_BOOST
	boost::function<void(void)> f;
#else
	std::function<void(void)> f;
#endif


	for (std::vector<io_context_ptr>::size_type i = 0; i < m_iocontexts.size(); ++i)
	{
		for (uint32_t c = 0; c < m_periocthread; )
		{
			try
			{				
				asio::post(m_threads, [&]() { m_iocontexts[i]->run(); });
	
				num_threads++;

				ret = e_libnet_err_noerror;

				++c;
			}
			catch (const std::bad_alloc& e)
			{
				(void)e;
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				continue;
			}
			catch (...)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				continue;
			}
		}
	}

	return ret;
}

void io_context_pool::close()
{
	if (!is_init())
	{
		return;
	}

	m_works.clear();

	for (std::vector<io_context_ptr>::size_type i = 0; i < m_iocontexts.size(); ++i)
	{
		m_iocontexts[i]->stop();
	}

	m_iocontexts.clear();

	//m_threads.join();

	m_isinit = false;
}

asio::io_context& io_context_pool::get_io_context()
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_mutex);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_mutex);
#endif

	asio::io_context& ioc = *(m_iocontexts[m_nextioc]);

	++m_nextioc;

	if (m_nextioc == m_iocontexts.size())
	{
		m_nextioc = 0;
	}

	return ioc;
}
