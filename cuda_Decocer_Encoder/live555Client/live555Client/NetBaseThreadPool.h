#ifndef _NetBaseThreadPool_H
#define _NetBaseThreadPool_H

#include <boost/lockfree/queue.hpp>

#define   MaxNetHandleQueueCount    512 
typedef map<NETHANDLE, NETHANDLE>   ClientProcessThreadMap;//固定客户端的线程序号 

class CNetBaseThreadPool
{
public:
	CNetBaseThreadPool(int nThreadCount);
   ~CNetBaseThreadPool();

   //插入任务ID 
   bool  InsertIntoTask(uint64_t nClientID);
   void  StopTask();

   pthread_mutex_t  pThreadPollLock;
private:
	volatile int nGetCurrentThreadOrder;
	void ProcessFunc();
	static void* OnProcessThread(void* lpVoid);

	volatile   uint64_t     nThreadProcessCount;
	std::mutex              threadLock;
	ClientProcessThreadMap  clientThreadMap;
    uint64_t                nTrueNetThreadPoolCount; 
    boost::lockfree::queue<uint64_t, boost::lockfree::capacity<2048>> m_NetHandleQueue[MaxNetHandleQueueCount];
    volatile bool         bExitProcessThreadFlag[MaxNetHandleQueueCount];
    volatile bool         bCreateThreadFlag;
	int                   hProcessHandle[MaxNetHandleQueueCount];
	pthread_t             hReadDataThread[MaxNetHandleQueueCount];
    volatile bool         bRunFlag;
};

#endif