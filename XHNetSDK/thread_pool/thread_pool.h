#pragma once


#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <memory>
#include <functional>
#include <atomic>
#include <future>

#include <stdexcept>


#define MAX_THREAD  256


namespace netlib
{
	using ThreadTask = std::function<void(void)>;
	
	class ThreadPool
	{
	public:				

		ThreadPool(int threadNumber);

		~ThreadPool();


		//往任务队列里添加任务
		bool append(ThreadTask task, bool bPriority = false);

	
		template<typename Func, typename... Args>
		void appendArg(Func&& func, Args&&... args);

		ThreadTask get_one_task();
		//启动线程池
		bool start(void);

		//停止线程池
		bool stop(void);

		// 线程池是否在运行
		bool IsRunning();

		int getThreadNum() const;

		int getCompletedTaskCount() const;

		// 获取线程池实例
		static ThreadPool* GetInstance();

		static void Destory()
		{
			if (nullptr != s_pThreadPool)
			{
				delete s_pThreadPool;
				s_pThreadPool = nullptr;
			}
		};
public:

		static ThreadPool*		s_pThreadPool;
private:
		//线程所执行的工作函数
		void threadWork(void);

private:
		std::mutex m_mutex;                                        //互斥锁	
	
		std::atomic< bool> m_bRunning;                              //线程池是否在运行
		int m_nThreadNumber;                                       //线程数

		std::condition_variable_any m_condition_empty;             //当任务队列为空时的条件变量
		std::queue<ThreadTask> m_taskList;                          //任务队列
		
		std::vector<std::shared_ptr<std::thread>> m_vecThread;     //用来保存线程对象指针
		//空闲线程数量
		std::atomic<int>  m_idlThrNum;
		std::atomic<int> m_nCompletedTasks{ 0 };

	};


	struct ComparePriority
	{
		bool operator()(const std::pair<int, ThreadTask>& lhs, const std::pair<int, ThreadTask>& rhs) const
		{
			return lhs.first < rhs.first;
		}
	};



	class ThreadPriorityPool
	{
	public:
		ThreadPriorityPool(int threadNumber);
		~ThreadPriorityPool();

	
		bool append(ThreadTask task, int nPriority=0);

		template<typename Func, typename... Args>
		void appendArg(Func&& func, Args&&... args);

		ThreadTask get_one_task();

		bool start();
		bool stop();
		bool IsRunning();

		int getThreadNum() const;
		int getCompletedTaskCount() const;

		static ThreadPriorityPool* GetInstance();
		static void Destory()
		{
			if (nullptr != s_pThreadPool)
			{
				delete s_pThreadPool;
				s_pThreadPool = nullptr;
			}
		}

	private:
		void threadWork();

	private:
		std::mutex m_mutex;
		std::atomic<bool> m_bRunning;
		int m_nThreadNumber;
		std::condition_variable_any m_condition_empty;
		std::vector<std::shared_ptr<std::thread>> m_vecThread;
		std::priority_queue<std::pair<int, ThreadTask>, std::vector<std::pair<int, ThreadTask>>, ComparePriority> m_taskList;
		std::atomic<int> m_nCompletedTasks{ 0 };
		static ThreadPriorityPool* s_pThreadPool;
	};


}

#define GSThreadPool_AddFun(x)	netlib::ThreadPool::GetInstance()->append(x)

#define GSThreadPool	netlib::ThreadPool::GetInstance()




