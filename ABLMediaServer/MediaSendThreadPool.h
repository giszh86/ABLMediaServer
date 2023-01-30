#ifndef _MediaSendThreadPool_H
#define _MediaSendThreadPool_H

#include <boost/lockfree/queue.hpp>

#define   MaxMediaSendThreadCount       2048 //预分配最大的线程数  
typedef map<NETHANDLE, NETHANDLE>   ClientProcessThreadMap;//固定客户端的线程序号 

//每个线程所包含的客户端ID  
struct ThreadContainClient
{
	int          nThreadOrder; //线程序号 0 、1、2、3、4 .... N  
	int          nTrueClientsCount; //记录该线程真正包括的客户端个数 
	uint64_t     nClients[MaxMediaSendThreadCount];//记录该线程的客户端ID数组 ,预分配4096个 ，但是实际以 nTrueClientsCount 为准 
	int          nMaxClientArraySize; //存放在数组中最大的下标 

	ThreadContainClient()
	{
		nThreadOrder = 0;
		nTrueClientsCount = 0;
		nMaxClientArraySize = 0;
		for (int i = 0; i < MaxMediaSendThreadCount; i++)
 			nClients[i] = 0;
 	}
};

class CMediaSendThreadPool
{
public:
	CMediaSendThreadPool(int nMaxThreadCount);
   ~CMediaSendThreadPool();

   static void* OnProcessThread(void* lpVoid);
   void        ProcessFunc();

   //把客户端加入发送线程池 
   bool    AddClientToThreadPool(uint64_t nClient);

   //把客户端从线程池移除 
   bool    DeleteClientToThreadPool(uint64_t nClient);

   void    MySleep(int nSleepTime);
private:
	std::mutex              threadLock;

	bool                    findClientAtArray(uint64_t nClient);
	ThreadContainClient     threadContainClient[MaxMediaSendThreadCount];//最大预分配4096 个线程数
	volatile uint64_t       nTrueMaxNetThreadPoolCount; //实际打算最大支持的发送线程数量 
 	volatile uint64_t       nCreateThreadProcessCount; //已经创建线程的总数 
    volatile bool           bCreateThreadFlag;  //是否创建线程完毕 
    volatile bool           bExitProcessThreadFlag[MaxMediaSendThreadCount]; //记录每一个线程是否退出 
#ifdef OS_System_Windows
    HANDLE                  hProcessHandle[MaxMediaSendThreadCount];
#else
   pthread_t                hProcessHandle[MaxMediaSendThreadCount];
#endif 
	volatile bool           bRunFlag;
};

#endif