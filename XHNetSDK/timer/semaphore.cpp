#include "semaphore.h"

void zhb::semaphore::wait()
{
    unique_lock<mutex> lck(m_mutex);
    //阻塞
    m_count.fetch_sub(1, std::memory_order_relaxed);
    if(m_count.load(std::memory_order_relaxed) < 0)
    {
        m_con.wait(lck, [this](){return m_wakeup.load(std::memory_order_relaxed) > 0;});
        m_wakeup.fetch_sub(1, std::memory_order_relaxed);
    }
}

bool zhb::semaphore::wait(int milliseconds, zhb::semaphore::function_type &&f)
{
    unique_lock<mutex> lck(m_mutex);
    auto msec = std::chrono::milliseconds(milliseconds);
    //阻塞
    m_count.fetch_sub(1, std::memory_order_relaxed);
    if(m_count.load(std::memory_order_relaxed) < 0)
    {
        bool b = false;
        if(nullptr == f)
        {
            b = m_con.wait_for(lck, msec, [this]()
            {
                return m_wakeup.load(std::memory_order_relaxed) > 0;
            });
        }
        else
        {
            b = m_con.wait_for(lck, msec, std::move(f));
        }

        m_wakeup.fetch_sub(1, std::memory_order_relaxed);

        return b;
    }

    return true;
}

void zhb::semaphore::signal_one()
{
    unique_lock<mutex> lck(m_mutex);
    m_count.fetch_add(1, std::memory_order_relaxed);
    if(m_count.load(std::memory_order_relaxed) <= 0)
    {
        m_wakeup.fetch_add(1, std::memory_order_relaxed);
        m_con.notify_one();
    }
}

void zhb::semaphore::signal_all()
{
    unique_lock<mutex> lck(m_mutex);
    m_count.fetch_add(1, std::memory_order_relaxed);
    if(m_count.load(std::memory_order_relaxed) <= 0)
    {
        m_wakeup.store(0, std::memory_order_relaxed);
        m_count.store(0, std::memory_order_relaxed);
        m_con.notify_all();
    }
}
