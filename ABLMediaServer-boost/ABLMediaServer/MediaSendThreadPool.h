#ifndef _MediaSendThreadPool_H
#define _MediaSendThreadPool_H

#include <boost/lockfree/queue.hpp>
#include <condition_variable>

#define   MaxMediaSendThreadCount       2048 //Ԥ���������߳���  

//ÿ���߳��������Ŀͻ���ID  
struct ThreadContainClient
{
	int          nThreadOrder; //�߳���� 0 ��1��2��3��4 .... N  
	int          nTrueClientsCount; //��¼���߳����������Ŀͻ��˸��� 
	uint64_t     nClients[MaxMediaSendThreadCount];//��¼���̵߳Ŀͻ���ID���� ,Ԥ����4096�� ������ʵ���� nTrueClientsCount Ϊ׼ 
	int          nMaxClientArraySize; //����������������±� 

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

   //�ѿͻ��˼��뷢���̳߳� 
   bool    AddClientToThreadPool(uint64_t nClient);

   //�ѿͻ��˴��̳߳��Ƴ� 
   bool    DeleteClientToThreadPool(uint64_t nClient);

   void    MySleep(int nSleepTime);
private:
	std::mutex              threadLock;

	bool                    findClientAtArray(uint64_t nClient);
	ThreadContainClient     threadContainClient[MaxMediaSendThreadCount];//���Ԥ����4096 ���߳���
	volatile uint64_t       nTrueMaxNetThreadPoolCount; //ʵ�ʴ������֧�ֵķ����߳����� 
 	volatile uint64_t       nCreateThreadProcessCount; //�Ѿ������̵߳����� 
    volatile bool           bCreateThreadFlag;  //�Ƿ񴴽��߳���� 
    volatile bool           bExitProcessThreadFlag[MaxMediaSendThreadCount]; //��¼ÿһ���߳��Ƿ��˳� 
#ifdef OS_System_Windows
    HANDLE                  hProcessHandle[MaxMediaSendThreadCount];
#else
   pthread_t                hProcessHandle[MaxMediaSendThreadCount];
#endif 
	volatile bool           bRunFlag;

	std::condition_variable  cv[MaxMediaSendThreadCount];
	std::mutex               mtx[MaxMediaSendThreadCount];
};

#endif