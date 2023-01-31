#ifndef _AUTO_LOCK_H_
#define _AUTO_LOCK_H_ 

#if (defined _WIN32 || defined _WIN64)

#else

#include <stdexcept>
#include <pthread.h>

namespace auto_lock
{
	class al_spin
	{
	public:
		al_spin()
		{
			if(0 != pthread_spin_init(&m_spin, PTHREAD_PROCESS_PRIVATE))
			{
				throw std::runtime_error("pthread_spin_init error");
			}
		}

		~al_spin()
		{
			pthread_spin_destroy(&m_spin);
		}

		void lock()
		{
			if(0 != pthread_spin_lock(&m_spin))
			{
				throw std::runtime_error("pthread_spin_lock error");
			}
		}

		void unlock()
		{
			pthread_spin_unlock(&m_spin);
		}

	private:
		pthread_spinlock_t m_spin;
	};

	class al_mutex
	{
	public:
		al_mutex()
		{
			
			if (0 != pthread_mutex_init(&m_mtx, NULL))
			{
				throw std::runtime_error("pthread_mutex_init error");
			}
			
		}

		~al_mutex()
		{
			pthread_mutex_destroy(&m_mtx);
		}

		void lock()
		{
			if (0 != pthread_mutex_lock(&m_mtx))
			{
				throw std::runtime_error("pthread_mutex_lock error");
			}
		}

		void unlock()
		{
			pthread_mutex_unlock(&m_mtx);
		}

	private:
		pthread_mutex_t m_mtx;
	};


	template<typename T> class al_lock
	{
	public:
		al_lock(T& m)
			: m_mtx(m)
		{
			m_mtx.lock();
		}

		~al_lock()
		{
			m_mtx.unlock();
		}

	private:
		T& m_mtx;
	};
}

#endif


#endif