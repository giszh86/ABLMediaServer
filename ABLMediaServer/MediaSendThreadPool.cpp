/*
功能：
    自研的线程池功能，能以多个线程去执行一类实例，不同的任务
    这个线程池，当达到帧数据时会触发线程池 拷贝视频动作  	
日期    2021-03-29
作者    罗家兄弟
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "MediaSendThreadPool.h"

extern std::shared_ptr<CNetRevcBase> GetNetRevcBaseClient(NETHANDLE CltHandle);
extern bool                            DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern CMediaFifo                      pDisconnectBaseNetFifo; //清理断裂的链接 

CMediaSendThreadPool::CMediaSendThreadPool(int nMaxThreadCount)
{
	nTrueMaxNetThreadPoolCount = nMaxThreadCount;
	if (nMaxThreadCount > MaxMediaSendThreadCount)
		nTrueMaxNetThreadPoolCount = MaxMediaSendThreadCount;

	if (nMaxThreadCount <= 0)
		nTrueMaxNetThreadPoolCount = 128;

	bRunFlag = true;
	nCreateThreadProcessCount = 0;//已经创建的线程数量
	for (int i = 0; i < MaxMediaSendThreadCount; i++)
	{
		bExitProcessThreadFlag[i] = true;
		hProcessHandle[i] = NULL;
		threadContainClient[i].nThreadOrder = 0;
		threadContainClient[i].nTrueClientsCount = 0;
		threadContainClient[i].nMaxClientArraySize = 0;
		for (int j = 0; j < MaxMediaSendThreadCount; j++)
			threadContainClient[i].nClients[j] = 0;
	}
	WriteLog(Log_Debug, "CMediaSendThreadPool = %X 构造 ", this);
}

CMediaSendThreadPool::~CMediaSendThreadPool()
{
	bRunFlag = false;
	for (int i = 0; i < nTrueMaxNetThreadPoolCount; i++)
	{
		while (!bExitProcessThreadFlag[i])
	  		Sleep(50);
		if (hProcessHandle[i] != NULL)
		{
#ifdef      OS_System_Windows
			CloseHandle(hProcessHandle[i]);
#endif
		}
	}
	WriteLog(Log_Debug, "CMediaSendThreadPool = %X 析构 \r\n", this);
	malloc_trim(0);
}

//查找客户端是否存在发送线程中
bool CMediaSendThreadPool::findClientAtArray(uint64_t nClient)
{
	int i = 0;
	int j = 0;
	for ( i = 0; i < nCreateThreadProcessCount; i++)
	{
		for ( j= 0;j < threadContainClient[i].nMaxClientArraySize ;j++)
		{
			if (threadContainClient[i].nClients[j] == nClient)
			{
 				WriteLog(Log_Debug, "把客户端 nClient = %llu 已经存在媒体发送线程池中，不需要再加入 ,nThreadID = %d ", nClient, i);
				return true;
			}
		}
	}
	return false;
}

//把客户端加入发送线程池 
bool  CMediaSendThreadPool::AddClientToThreadPool(uint64_t nClient)
{
	std::lock_guard<std::mutex> lock(threadLock);
	
	//查找客户端是否存在发送线程中
	if (findClientAtArray(nClient))
		return false;

	//先查找空闲的线程，如果有空闲线程立即使用，不需要创建线程 
	for (int i = 0; i < nCreateThreadProcessCount; i++)
	{
		if (threadContainClient[i].nTrueClientsCount == 0)
		{//找到空闲的线程 
			threadContainClient[i].nClients[0] = nClient;
			threadContainClient[i].nTrueClientsCount = 1;
			threadContainClient[i].nMaxClientArraySize = 1;
			WriteLog(Log_Debug, "把客户端 nClient = %llu 成功加入到媒体发送线程池 ,nThreadID = %d ", nClient,i );
			return true;
		}
	}

	if (nCreateThreadProcessCount <= nTrueMaxNetThreadPoolCount)
	{//当前线程数量没有达到 打算的最大线程数量，就单独创建一个线程，进行发送
 		//如果找不到
		bCreateThreadFlag = false;
		unsigned long dwThread;
#ifdef OS_System_Windows
		hProcessHandle[nCreateThreadProcessCount] = ::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)OnProcessThread, (LPVOID)this, 0, &dwThread);
#else 
		pthread_create(&hProcessHandle[nCreateThreadProcessCount], NULL, OnProcessThread, (void*)this);
#endif
		while (bCreateThreadFlag == false)
			Sleep(5);

		threadContainClient[nCreateThreadProcessCount].nThreadOrder = nCreateThreadProcessCount; //当前线程序号
		threadContainClient[nCreateThreadProcessCount].nTrueClientsCount = 1;//当前线程所包含的客户端总数 
		threadContainClient[nCreateThreadProcessCount].nMaxClientArraySize = 1;
		threadContainClient[nCreateThreadProcessCount].nClients[0] = nClient;//存储该线程负责发送的客户端ID  

		WriteLog(Log_Debug, "把客户端 nClient = %llu 成功加入到媒体发送线程池 ,nThreadID = %d ", nClient, nCreateThreadProcessCount);

		nCreateThreadProcessCount ++;

		return true;
	}
	else
	{//当前创建的线程已经达到 打算创建的最大线程数，不能再创建线程，采用线程共享的方式发送视频 
		//查找出客户端数量最少的线程 
		int  nMinThreadContainClient = MaxMediaSendThreadCount;
		int  nMinThreadContainClientThread = 0 ;//记下客户端数量最少的线程ID 
 
		for (int i = 0; i < nCreateThreadProcessCount; i++)
		{
			if (threadContainClient[i].nTrueClientsCount < nMinThreadContainClient)
			{//找到空闲的线程 
				nMinThreadContainClient = threadContainClient[i].nTrueClientsCount;
				nMinThreadContainClientThread = i;
  			}
		}
 
		//已经找到客户端数量最少的线程ID
		if (nMinThreadContainClientThread >= 0 && nMinThreadContainClientThread < nCreateThreadProcessCount)
		{
			bool FillArrayPositionFlag = false; //是否已经成功占位
			int  nMaxArraySize = 1;//最大数组下标 

			for (int i = 0; i < nCreateThreadProcessCount; i++)
			{
				if (!FillArrayPositionFlag)
				{
				   if (threadContainClient[nMinThreadContainClientThread].nClients[i] == 0)
				   {
					 threadContainClient[nMinThreadContainClientThread].nClients[i] = nClient; //把ClientID 放置在空缺的位置上 
					 threadContainClient[nMinThreadContainClientThread].nTrueClientsCount += 1; //客户端总数加1  

					 FillArrayPositionFlag = true; //占位成功 
					 WriteLog(Log_Debug, "把客户端 nClient = %llu 成功加入到媒体发送线程池 ,nThreadID = %d ", nClient, i);
 				    }
			     }

				if (threadContainClient[nMinThreadContainClientThread].nClients[i] > 0)
				{
					nMaxArraySize = i + 1;
				}
  			 }

			threadContainClient[nMinThreadContainClientThread].nMaxClientArraySize = nMaxArraySize;//更新nClient 存储位置的最大数组下标 
			WriteLog(Log_Debug, "把客户端 nClient = %llu 成功加入到媒体发送线程池 ,nThreadID = %d , nMaxClientArraySize = %d ", nClient, nMinThreadContainClientThread,nMaxArraySize);
		}

 	   return true;
 	}
}

//把客户端从线程池移除 
bool  CMediaSendThreadPool::DeleteClientToThreadPool(uint64_t nClient)
{
	std::lock_guard<std::mutex> lock(threadLock);

	for (int i = 0; i < MaxMediaSendThreadCount; i++)
	{
		for (int j = 0; j < MaxMediaSendThreadCount; j++)
		{
			if (threadContainClient[i].nClients[j] == nClient)
			{
				threadContainClient[i].nClients[j] = 0;
				threadContainClient[i].nTrueClientsCount -= 1;

				if (threadContainClient[i].nTrueClientsCount < 0) //保护，防止意外 
					threadContainClient[i].nTrueClientsCount = 0;

				WriteLog(Log_Debug, "把客户端 nClient = %llu 从媒体发送线程池移除 ", nClient);
				return true;
			}
		}
	}
	return false;
}

void CMediaSendThreadPool::MySleep(int nSleepTime)
{
	if (nSleepTime <= 0)
		Sleep(5);
	else
		Sleep(nSleepTime);
}

void CMediaSendThreadPool::ProcessFunc()
{
	int nCurrentThreadID = nCreateThreadProcessCount;
 	bExitProcessThreadFlag[nCurrentThreadID] = false;
	bCreateThreadFlag = true; //创建线程完毕
	int i;
	int nVideoFrameCount, nAudioFrameCount;

	while (bRunFlag)
	{
		if (threadContainClient[nCurrentThreadID].nTrueClientsCount > 0)
		{
			for (i = 0; i < threadContainClient[nCurrentThreadID].nMaxClientArraySize; i++)
			{
				if (threadContainClient[nCurrentThreadID].nClients[i] > 0)
				{
		           std::shared_ptr<CNetRevcBase> pClient= GetNetRevcBaseClient(threadContainClient[nCurrentThreadID].nClients[i]);
				   if (pClient != NULL )
				   {
					   if (pClient->netBaseNetType == NetBaseNetType_HttpHLSClientRecv)
					   {//用于HLS视频，音频均匀塞入媒体源、以后还可以支持别的没有视频、音频发送的类
						   pClient->SendVideo();
						   pClient->SendAudio();
						   Sleep(5);
 					   }
					   else
					   {
 						   nVideoFrameCount = pClient->m_videoFifo.GetSize();
						   nAudioFrameCount = pClient->m_audioFifo.GetSize();
						   if (nVideoFrameCount == 0 && nAudioFrameCount == 0)
						   {
							   if (threadContainClient[nCurrentThreadID].nTrueClientsCount == 1)
							   {//如果该线程只有1个连接，可以等待5毫秒
								   Sleep(10);
								   continue;
							   }
							   else //如果该线程超过2个连接，本次循环不能等待时间 ，因为还需要发送下一个链接的数据 
							   {
								   Sleep(10); //防止死循环
								   continue;
							   }
						   }

						   for (i = 0; i < nVideoFrameCount; i++)
							   pClient->SendVideo();

						   for (i = 0; i < nAudioFrameCount; i++)
							   pClient->SendAudio();
					   }
				   }
				   else
				   {//找不到客户端 
					   WriteLog(Log_Debug, "找不到客户端 ,把该客户端 nClient = %llu 从媒体发送线程池移除 ", threadContainClient[nCurrentThreadID].nClients[i]);

					   threadContainClient[nCurrentThreadID].nClients[i] = 0; //把该通道设置为 0 ，
					   threadContainClient[nCurrentThreadID].nTrueClientsCount -= 1;//链接数减少1 

					   if (threadContainClient[nCurrentThreadID].nTrueClientsCount < 0)
						   threadContainClient[nCurrentThreadID].nTrueClientsCount = 0 ;
				   }
				}
 		    }
 		}else 
 		  Sleep(5);
 	}
	bExitProcessThreadFlag[nCurrentThreadID] = true;
}
 
void* CMediaSendThreadPool::OnProcessThread(void* lpVoid)
{
	int nRet = 0;
#ifndef OS_System_Windows
	pthread_detach(pthread_self()); //让子线程和主线程分离，当子线程退出时，自动释放子线程内存
#endif

	CMediaSendThreadPool* pThread = (CMediaSendThreadPool*)lpVoid;
	pThread->ProcessFunc();

#ifndef OS_System_Windows
	pthread_exit((void*)&nRet); //退出线程
#endif
	return  0;
}

