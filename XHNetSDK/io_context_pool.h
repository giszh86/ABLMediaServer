#ifndef _IO_CONTEXT_POOL_H_
#define _IO_CONTEXT_POOL_H_ 

#include <vector>
#include <asio.hpp>
#include <asio/detail/thread_group.hpp>
#include <functional>
#include "auto_lock.h"
#ifdef USE_BOOST
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#else
#include <memory>
#include <thread>
#endif

#ifdef USE_BOOST
class io_context_pool : public boost::noncopyable
#else
class io_context_pool : public asio::detail::noncopyable
#endif
{

public:
	io_context_pool();
	

	~io_context_pool();

	uint32_t get_thread_count() const { return static_cast<uint32_t>(num_threads); }

	bool is_init();

	int32_t init(uint32_t iocnum, uint32_t periocthread);

	int32_t run();

	void close();

	asio::io_context& get_io_context();

private:
#ifdef USE_BOOST
	typedef boost::shared_ptr<asio::io_context> io_context_ptr;
	typedef boost::shared_ptr<asio::io_context::work> work_ptr;
#else
	typedef std::shared_ptr<asio::io_context> io_context_ptr;
	typedef std::shared_ptr<asio::io_context::work> work_ptr;
#endif



#ifdef USE_BOOST
	typedef boost::shared_ptr<boost::thread> thread_ptr;
#else
	typedef std::shared_ptr<std::thread> thread_ptr;
#endif

	std::vector<io_context_ptr> m_iocontexts;
	std::vector<work_ptr> m_works;

	asio::detail::thread_group m_threads;
	//std::vector<std::thread *> threads;
	//asio::thread_pool m_threads;
	std::size_t num_threads=0;

#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_mutex m_mutex;
#else
	auto_lock::al_spin m_mutex;
#endif
	uint32_t m_nextioc;
	uint32_t m_periocthread;
	bool m_isinit;
};

inline bool io_context_pool::is_init()
{
	return m_isinit;
}

#endif


