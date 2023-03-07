/*
功能：
    自研的线程池功能，能以多个线程去执行一类实例，不同的任务
    这个线程池，当网络数据达到时触发线程执行 	
日期    2021-03-29
作者    罗家兄弟
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetBaseThreadPool.h"

extern boost::shared_ptr<CNetRevcBase> GetNetRevcBaseClient(NETHANDLE CltHandle);

CNetBaseThreadPool::CNetBaseThreadPool(int nThreadCount)
{
	nThreadProcessCount = 0;
 	nTrueNetThreadPoolCount = nThreadCount;
	if (nThreadCount > MaxNetHandleQueueCount)
		nTrueNetThreadPoolCount = MaxNetHandleQueueCount;

	if (nThreadCount <= 0)
		nTrueNetThreadPoolCount = 64 ;

	pthread_mutex_init(&pThreadPollLock, NULL);

 	bRunFlag = true;
	nGetCurrentThreadOrder = 0;
	for (int i = 0; i < nTrueNetThreadPoolCount; i++)
	{
		bCreateThreadFlag = false;
 		hProcessHandle[i] = pthread_create(&hReadDataThread[i], NULL, OnProcessThread, (void*)this);;
		usleep(20 * OneMicroSecondTime);
	}
	Rtsp_WriteLog(Log_Debug, "CNetBaseThreadPool= %X, 构造 ", this);
}

CNetBaseThreadPool::~CNetBaseThreadPool()
{
	StopTask();

	pthread_mutex_destroy(&pThreadPollLock);
	Rtsp_WriteLog(Log_Debug, "CNetBaseThreadPool %X 析构 \r\n", this );
}

void  CNetBaseThreadPool::StopTask()
{
	if (bRunFlag)
	{
		bRunFlag = false;
		usleep(100 * OneMicroSecondTime);
		for (int i = 0; i < nTrueNetThreadPoolCount; i++)
		{
			while (!bExitProcessThreadFlag[i])
				usleep(50 * OneMicroSecondTime);
 		}
	}
}

void CNetBaseThreadPool::ProcessFunc()
{
	pthread_mutex_lock(&pThreadPollLock);
 		int nCurrentThreadID = nGetCurrentThreadOrder;
		Rtsp_WriteLog(Log_Debug, "CNetBaseThreadPool = %X nCurrentThreadID = %d ", this, nCurrentThreadID);
 		bExitProcessThreadFlag[nCurrentThreadID] = false;
	    bCreateThreadFlag = true; //创建线程完毕
		nGetCurrentThreadOrder ++;
	pthread_mutex_unlock(&pThreadPollLock);
	uint64_t nClientID;

	while (bRunFlag)
	{
		if (m_NetHandleQueue[nCurrentThreadID].pop(nClientID))
		{
			boost::shared_ptr<CNetRevcBase> pClient = GetNetRevcBaseClient(nClientID);
			if (pClient != NULL)
			{
				pClient->ProcessNetData();//任务执行
			}
		}else
			usleep(5 * OneMicroSecondTime);
 	}
	bExitProcessThreadFlag[nCurrentThreadID] = true;
}

void* CNetBaseThreadPool::OnProcessThread(void* lpVoid)
{
	pthread_detach(pthread_self()); //让子线程和主线程分离，当子线程退出时，自动释放子线程内存
    int nRet2 = 0 ;

	CNetBaseThreadPool* pThread = (CNetBaseThreadPool*)lpVoid;
	pThread->ProcessFunc();
	
	pthread_exit((void*)&nRet2); //退出线程
	return  0;
}

bool CNetBaseThreadPool::InsertIntoTask(uint64_t nClientID)
{
	std::lock_guard<std::mutex> lock(threadLock);

	int               nThreadThread = 0;
	ClientProcessThreadMap::iterator it;

	it = clientThreadMap.find(nClientID);
	if (it != clientThreadMap.end())
	{//找到 
		nThreadThread = (*it).second;
	}
	else
	{//尚未加入过
		nThreadThread = nThreadProcessCount % nTrueNetThreadPoolCount;
 		clientThreadMap.insert(ClientProcessThreadMap::value_type(nClientID, nThreadThread));
		nThreadProcessCount  ++;
	}

	m_NetHandleQueue[nThreadThread].push(nClientID);

	return true;
}
