#pragma once
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <chrono>
#include <functional>
#include <future>

using std::mutex;
using std::unique_lock;
using std::atomic;
using std::condition_variable;
using namespace std::chrono;

namespace zhb {
class semaphore
{
public:
    semaphore() = default;
    semaphore(int count)
        :m_count(count)
        ,m_wakeup(0)
    {

    }

    ~semaphore()
    {

    }

    /*!
     * \brief wait 等待
     */
    void wait();

    using function_type = std::function<bool ()>;
    bool wait(int milliseconds, function_type &&f = nullptr);

#if 0
    template<typename Func, typename ...Args>
    void wait_for(Func &&f, Args &&...args)
    {
        using ftype = decltype (f(args...));
        auto newFunc = std::make_shared<std::packaged_task<ftype()>>(std::bind(std::forward<Func>(f), \
                                                                               std::forward<Args>(args)...));


        auto addFunc = [newFunc]()->bool
        {
            return (*newFunc)();
        };

        unique_lock<mutex> lck(m_mutex);
        m_count.fetch_sub(1, std::memory_order_relaxed);
        if(m_count.load(std::memory_order_relaxed) < 0)
        {
            m_con.wait(lck, addFunc);
        }
    }
#endif

    /*!
     * \brief signal_one 唤醒一个
     */
    void signal_one();

    /*!
     * \brief signal_all 唤醒所有
     */
    void signal_all();

private:
    atomic<int>         m_count{0};
    atomic<int>         m_wakeup{0};
    mutex               m_mutex;
    condition_variable  m_con;
};
}


