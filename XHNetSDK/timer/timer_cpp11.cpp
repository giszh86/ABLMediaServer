#include "timer_cpp11.h"

zhb::zhb_timer_cpp11::timer_cpp11::~timer_cpp11()
{
    delete_all();
}

void zhb::zhb_timer_cpp11::timer_cpp11::delete_timer(zhb::zhb_timer_cpp11::u32 id)
{
    std::lock_guard<std::mutex> lck(m_task_mutex);
    auto it = m_tasks.find(id);
    if(it != m_tasks.end())
    {
        std::shared_ptr<task_unit> ptask{it->second};

        m_tasks.erase(it);

        //异步释放
        std::thread([ptask]()
        {
            ptask->stop();
        }).detach();
    }

}

void zhb::zhb_timer_cpp11::timer_cpp11::delete_all()
{
    std::lock_guard<std::mutex> lck(m_task_mutex);
    for(auto &m : m_tasks)
    {
        std::shared_ptr<task_unit> ptask{m.second};

        //异步释放
        std::thread([ptask]()
        {
            ptask->stop();
        }).detach();
    }

    m_tasks.clear();
}

zhb::zhb_timer_cpp11::task_unit::task_unit(zhb::zhb_timer_cpp11::u32 id, zhb::zhb_timer_cpp11::i32 interval, zhb::zhb_timer_cpp11::task_function func)
    :m_id(id)
    ,m_interval(interval)
    ,m_func(func)
    ,m_run_flag(true)
{
    //开启线程
    //start();
}

void zhb::zhb_timer_cpp11::task_unit::start()
{
    m_thread = std::thread(&task_unit::run, this);
}

void zhb::zhb_timer_cpp11::task_unit::stop()
{
    m_run_flag.store(false, std::memory_order_relaxed);
    m_semaphore.signal_one();
    if(m_thread.joinable())
    {
        m_thread.join();
    }

    printf("timer job stoped.\n");

}

void zhb::zhb_timer_cpp11::task_unit::run()
{
    while(m_run_flag.load(std::memory_order_relaxed))
    {
        bool b = m_semaphore.wait(m_interval, [this]()
        {
            return !m_run_flag.load(std::memory_order_relaxed);
        });

        if(b)
        {
            return;
        }

        m_func();
    }
}
