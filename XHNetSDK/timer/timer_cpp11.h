#pragma once
#include <map>
#include <mutex>
#include <memory>
#include <future>
#include <thread>
#include <functional>
#include "semaphore.h"

namespace zhb
{
	namespace zhb_timer_cpp11 {

		using u32 = unsigned int;
		using i32 = int;
		using u64 = unsigned long long;
		using i64 = long long;

		using task_function = std::function<void()>;

		//定时器事件信息
		class task_unit
		{
		public:
			explicit task_unit(u32 id, i32 interval = 0, task_function func = nullptr);

			/*!
			 * \brief start 当前任务启动线程
			 */
			void start();

			/*!
			 * \brief stop 任务释放
			 */
			void stop();

		private:
			/*!
			 * \brief run 线程运行函数
			 */
			void run();

			task_unit(const task_unit& r) = delete;

			task_unit& operator=(const task_unit& r) = delete;
		private:
			u32                 m_id{ 0 };               //任务id（唯一）
			i32                 m_interval{ 0 };         //定时器间隔时间(ms)
			task_function       m_func{ nullptr };        //任务执行函数
			std::thread         m_thread;      //线程
			zhb::semaphore      m_semaphore;   //信号量，控制任务执行的时间
			std::atomic<bool>   m_run_flag{ true };       //线程运行标识
		};

		/*!
		 * \brief The timer_cpp11 class 定时器控制类
		 */
		class timer_cpp11
		{
		public:
			timer_cpp11() = default;

			~timer_cpp11();

			/*!
			 * \brief start_timer 添加计数器任务
			 * \param id    任务id
			 * \param interval_ms   间隔时间，毫秒
			 * \param f
			 * \param args
			 * \return
			 */
			template<class F, class ...ARGS>
			bool start_timer(u32 id, i32 interval_ms, F&& f, ARGS &&...args)
			{
				std::lock_guard<std::mutex> lck(m_task_mutex);
				//check if exist
				if (m_tasks.find(id) != m_tasks.end())
				{
					return false;
				}

				//封装函数
				using function_type = decltype(f(args...));
				auto task1 = std::make_shared<std::packaged_task<function_type()>>(std::bind(std::forward<F>(f), \
					std::forward<ARGS>(args)...));
				auto task = [task1]()->void
				{
					(*task1)();
					(*task1).reset(); //重置std::future的状态
				};

				std::shared_ptr<task_unit> ptask(new task_unit(id, interval_ms, task));
				auto ret = m_tasks.emplace(id, ptask);

				if (true == ret.second)
				{
					//执行线程
					ptask->start();
				}

				return ret.second;
			}

			/*!
			 * \brief delete_timer 删除定时器任务
			 * \param id 对应的定时器id
			 */
			void delete_timer(u32 id);

			/*!
			 * \brief delete_all 清空定时器
			 */
			void delete_all();

		private:
			std::mutex                                  m_task_mutex;
			std::map<u32, std::shared_ptr<task_unit>>    m_tasks;
		};

		/*!线程管理*/
		class timer_thread
		{
		private:
			std::thread m_thread;
		};

	}

	using timer = zhb_timer_cpp11::timer_cpp11;
}


